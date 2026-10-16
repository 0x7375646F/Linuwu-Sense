#!/usr/bin/env python3
"""
Penguin Sense: a minimal, read-only system info window (CPU/GPU temps, any
fan RPM hwmon exposes -- including linuwu_sense's own "acer" hwmon once
fan_speed_read is enabled). Styled to match osd/linuwu_turbo_osd.py.

No fan control, no RGB, no hotkey integration -- just launch it:
    python3 osd/penguin_sense.py
"""
import glob
import os
import subprocess
import sys

import gi

gi.require_version("Gtk", "3.0")
from gi.repository import GLib, Gtk  # noqa: E402

REFRESH_SECONDS = 2

# Same palette as linuwu_turbo_osd.py -- keep in sync if that one changes.
BG = "rgba(45, 45, 45, 0.88)"
GREEN = "#8BC34A"
GREY_TEXT = "#CFCFCF"

CSS = f"""
#ps-box {{ background-color: {BG}; padding: 12px 20px; }}
#ps-title {{ color: {GREEN}; font-size: 14px; font-weight: 800; letter-spacing: 2px; }}
#ps-label {{ color: {GREY_TEXT}; font-size: 12px; }}
#ps-value {{ color: {GREEN}; font-size: 12px; font-weight: 700; }}
"""


def read_hwmon_sensors(root="/sys/class/hwmon"):
    """Return [(name, value_str), ...] for every temp*_input and fan*_input
    under every hwmon chip. Pure stdlib, works for any driver (coretemp,
    k10temp, amdgpu, nouveau, linuwu_sense's "acer" hwmon, ...)."""
    sensors = []
    for chip_dir in sorted(glob.glob(os.path.join(root, "hwmon*"))):
        try:
            with open(os.path.join(chip_dir, "name")) as f:
                chip = f.read().strip()
        except OSError:
            continue

        for path in sorted(glob.glob(os.path.join(chip_dir, "temp*_input"))):
            idx = os.path.basename(path)[len("temp"):-len("_input")]
            label = _read_label(chip_dir, f"temp{idx}_label") or chip
            value = _read_int(path)
            if value is not None:
                sensors.append((f"{chip}/{label}", f"{value / 1000:.0f}°C"))

        for path in sorted(glob.glob(os.path.join(chip_dir, "fan*_input"))):
            idx = os.path.basename(path)[len("fan"):-len("_input")]
            label = _read_label(chip_dir, f"fan{idx}_label") or f"{chip} fan{idx}"
            value = _read_int(path)
            if value is not None:
                sensors.append((label, f"{value} RPM"))

    return sensors


def _read_label(chip_dir, filename):
    path = os.path.join(chip_dir, filename)
    try:
        with open(path) as f:
            return f.read().strip()
    except OSError:
        return None


def _read_int(path):
    try:
        with open(path) as f:
            return int(f.read())
    except (OSError, ValueError):
        return None


def read_nvidia_gpu_temp():
    """nvidia-smi is already installed alongside the proprietary driver on
    these laptops; hwmon usually doesn't expose the discrete GPU at all."""
    try:
        out = subprocess.run(
            ["nvidia-smi", "--query-gpu=temperature.gpu", "--format=csv,noheader"],
            capture_output=True, text=True, timeout=1,
        )
    except (OSError, subprocess.SubprocessError):
        return None
    if out.returncode != 0 or not out.stdout.strip():
        return None
    return f"{out.stdout.strip().splitlines()[0]}°C"


def read_all_sensors():
    sensors = read_hwmon_sensors()
    gpu_temp = read_nvidia_gpu_temp()
    if gpu_temp is not None:
        sensors.insert(0, ("nvidia/gpu", gpu_temp))
    return sensors


class PenguinSense(Gtk.Window):
    def __init__(self):
        super().__init__(title="Penguin Sense")
        self.set_default_size(280, 0)
        self.set_resizable(False)

        css = Gtk.CssProvider()
        css.load_from_data(CSS.encode())
        Gtk.StyleContext.add_provider_for_screen(
            self.get_screen(), css, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)

        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        box.set_name("ps-box")
        title = Gtk.Label(label="PENGUIN SENSE")
        title.set_name("ps-title")
        box.pack_start(title, False, False, 0)

        self._grid = Gtk.Grid(row_spacing=4, column_spacing=16)
        box.pack_start(self._grid, False, False, 0)
        self.add(box)

        self.refresh()
        GLib.timeout_add_seconds(REFRESH_SECONDS, self._on_tick)
        self.connect("destroy", Gtk.main_quit)

    def _on_tick(self):
        self.refresh()
        return True

    def refresh(self):
        for child in list(self._grid.get_children()):
            self._grid.remove(child)

        sensors = read_all_sensors()
        if not sensors:
            sensors = [("no sensors found", "-")]

        for row, (name, value) in enumerate(sensors):
            name_label = Gtk.Label(label=name, xalign=0)
            name_label.set_name("ps-label")
            value_label = Gtk.Label(label=value, xalign=1)
            value_label.set_name("ps-value")
            self._grid.attach(name_label, 0, row, 1, 1)
            self._grid.attach(value_label, 1, row, 1, 1)

        self._grid.show_all()


def _selftest():
    import tempfile

    with tempfile.TemporaryDirectory() as root:
        chip = os.path.join(root, "hwmon0")
        os.makedirs(chip)
        with open(os.path.join(chip, "name"), "w") as f:
            f.write("coretemp\n")
        with open(os.path.join(chip, "temp1_input"), "w") as f:
            f.write("45000\n")
        with open(os.path.join(chip, "fan1_input"), "w") as f:
            f.write("2400\n")

        sensors = dict(read_hwmon_sensors(root))
        assert sensors["coretemp/coretemp"] == "45°C", sensors
        assert sensors["coretemp fan1"] == "2400 RPM", sensors
    print("selftest OK")


def main():
    if "--selftest" in sys.argv:
        _selftest()
        return
    win = PenguinSense()
    win.show_all()
    Gtk.main()


if __name__ == "__main__":
    main()
