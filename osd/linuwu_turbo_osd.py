#!/usr/bin/env python3
"""
Shows a small transient OSD popup whenever the Predator turbo hotkey is
toggled, by blocking on poll() against the linuwu_sense driver's
turbo_sense/turbo_state sysfs file (which the driver wakes with
sysfs_notify() on every toggle -- see acer_toggle_turbo() in
src/linuwu_sense.c). No root privileges needed; the file is 0444.
"""
import os
import sys

import gi

gi.require_version("Gtk", "3.0")
gi.require_version("Gdk", "3.0")
from gi.repository import Gdk, GLib, Gtk  # noqa: E402

SYSFS_PATH = "/sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/turbo_sense/turbo_state"
DISPLAY_SECONDS = 3

BG = "rgba(45, 45, 45, 0.88)"      # Mint-grey panel
GREEN = "#8BC34A"                  # Mint green accent
GREY_TEXT = "#CFCFCF"


class TurboOSD:
    def __init__(self):
        self._window = None
        self._hide_id = None

    def show(self, turbo_on):
        self._dismiss()

        accent = GREEN if turbo_on else GREY_TEXT
        title = "TURBO" if turbo_on else "AUTO"
        subtitle = "Fans at maximum" if turbo_on else "Fans back to normal"

        win = Gtk.Window(type=Gtk.WindowType.POPUP)
        win.set_decorated(False)
        win.set_skip_taskbar_hint(True)
        win.set_skip_pager_hint(True)
        win.set_keep_above(True)
        win.set_accept_focus(False)
        win.set_type_hint(Gdk.WindowTypeHint.NOTIFICATION)
        win.set_app_paintable(True)

        screen = win.get_screen()
        visual = screen.get_rgba_visual()
        if visual and screen.is_composited():
            win.set_visual(visual)

        css = Gtk.CssProvider()
        css.load_from_data(f"""
            #osd-box {{
                background-color: {BG};
                border: 2px solid {accent};
                border-radius: 12px;
                padding: 10px 22px;
            }}
            #osd-title {{
                color: {accent};
                font-size: 18px;
                font-weight: 800;
                letter-spacing: 2px;
            }}
            #osd-subtitle {{
                color: {GREY_TEXT};
                font-size: 11px;
            }}
        """.encode())

        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=2)
        box.set_name("osd-box")
        ctx = box.get_style_context()
        ctx.add_provider(css, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)

        title_label = Gtk.Label(label=title)
        title_label.set_name("osd-title")
        title_label.get_style_context().add_provider(css, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)

        subtitle_label = Gtk.Label(label=subtitle)
        subtitle_label.set_name("osd-subtitle")
        subtitle_label.get_style_context().add_provider(css, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)

        box.pack_start(title_label, False, False, 0)
        box.pack_start(subtitle_label, False, False, 0)
        win.add(box)

        win.show_all()
        win.realize()

        display = screen.get_display()
        monitor = display.get_primary_monitor() or display.get_monitor(0)
        geo = monitor.get_geometry()
        w, h = win.get_size()
        win.move(geo.x + (geo.width - w) // 2, geo.y + 24)

        self._window = win
        self._hide_id = GLib.timeout_add_seconds(DISPLAY_SECONDS, self._dismiss)

    def _dismiss(self):
        if self._hide_id is not None:
            GLib.source_remove(self._hide_id)
            self._hide_id = None
        if self._window is not None:
            self._window.destroy()
            self._window = None
        return False


def watch(osd: TurboOSD):
    # If linuwu_sense is reloaded (kernel upgrade, dkms rebuild, manual
    # rmmod/insmod) while we're running, our fd points at a sysfs node
    # that no longer exists. Rather than try to detect and reconnect to
    # the *new* node in-process, we exit and let the systemd unit's
    # Restart=on-failure bring up a fresh process, which reopens
    # SYSFS_PATH from scratch -- so it self-heals even if the module
    # isn't back yet (RestartSec keeps retrying).
    try:
        fd = os.open(SYSFS_PATH, os.O_RDONLY)

        def read_value():
            os.lseek(fd, 0, os.SEEK_SET)
            return os.read(fd, 16).strip()

        last_value = read_value()
    except OSError as exc:
        print(f"linuwu-turbo-osd: cannot open {SYSFS_PATH}: {exc}", file=sys.stderr)
        sys.exit(1)

    def on_ready(source, condition):
        # sysfs_notify() wakes pollers with IO_PRI *and* IO_ERR together --
        # that IO_ERR is not an error, it's just how kernfs signals "the
        # value changed" (see kernfs_fop_poll()). Do not treat it as a sign
        # the node is gone; only a failed read (ENODEV once the attribute
        # group is actually removed) means that.
        nonlocal last_value
        try:
            value = read_value()
        except OSError as exc:
            print(f"linuwu-turbo-osd: lost {SYSFS_PATH} ({exc}); exiting for systemd to restart us",
                  file=sys.stderr)
            os._exit(1)
        if value != last_value:
            last_value = value
            osd.show(value == b"1")
        return True

    GLib.io_add_watch(fd, GLib.IO_PRI | GLib.IO_ERR | GLib.IO_HUP, on_ready)


def main():
    osd = TurboOSD()
    watch(osd)
    Gtk.main()


if __name__ == "__main__":
    main()
