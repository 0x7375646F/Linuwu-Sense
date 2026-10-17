"""Command-line interface for the Linuwu-Sense kernel module."""

from __future__ import annotations

import argparse
import json
import sys
from typing import Callable

from . import profiles
from .controller import FOUR_ZONE_MODES, STATUS_ATTRIBUTES, Controller
from .errors import LinuwuError
from .thermal import Thermal

__version__ = "0.1.0"

PROG = "linusexcli"

_Handler = Callable[[Controller, argparse.Namespace], int]

#: Human-friendly labels for ``status``.
_STATUS_LABELS = {
    "fan_speed": "Fan speed",
    "battery_limiter": "Battery limiter",
    "battery_calibration": "Battery calibration",
    "backlight_timeout": "Backlight timeout",
    "boot_animation_sound": "Boot animation",
    "lcd_override": "LCD override",
    "usb_charging": "USB charging",
    "four_zone_mode": "Four-zone mode",
    "per_zone_mode": "Per-zone mode",
    "thermal_profile": "Thermal profile",
    "turbo_state": "Turbo mode",
}

_STATUS_FLAGS = ("battery_limiter", "backlight_timeout", "boot_animation_sound", "lcd_override")


def _on_off(value: str) -> bool:
    lowered = value.lower()
    if lowered in ("on", "1", "true", "yes", "enable", "enabled"):
        return True
    if lowered in ("off", "0", "false", "no", "disable", "disabled"):
        return False
    raise argparse.ArgumentTypeError("expected 'on' or 'off'")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog=PROG,
        description=(
            "Control the Linuwu-Sense kernel module on Acer Predator/Nitro "
            "laptops: fans, battery, keyboard RGB and thermal profiles."
        ),
        epilog=(
            "Run '%(prog)s status' to see what this machine exposes. Writes go "
            "straight to sysfs; use sudo or the linuwu_sense group if denied."
        ),
    )
    parser.add_argument("--version", action="version", version=f"%(prog)s {__version__}")
    subparsers = parser.add_subparsers(dest="command", metavar="<command>")

    status = subparsers.add_parser("status", help="show the current state of every control")
    status.add_argument("--json", action="store_true", help="emit machine-readable JSON")
    status.set_defaults(handler=_cmd_status)

    fan = subparsers.add_parser("fan", help="set CPU/GPU fan speed (0 = auto, 1-100)")
    fan.add_argument("cpu", type=int, metavar="CPU", help="CPU fan speed, 0-100")
    fan.add_argument("gpu", type=int, metavar="GPU", help="GPU fan speed, 0-100")
    fan.set_defaults(handler=_cmd_fan)

    battery = subparsers.add_parser("battery", help="battery limiter and calibration")
    battery_sub = battery.add_subparsers(dest="battery_command", metavar="<setting>", required=True)
    limiter = battery_sub.add_parser("limiter", help="enable/disable the 80%% charge limit")
    limiter.add_argument("state", type=_on_off, metavar="{on,off}")
    limiter.set_defaults(handler=_cmd_battery_limiter)
    calibration = battery_sub.add_parser("calibration", help="start/stop battery calibration")
    calibration.add_argument("state", choices=("start", "stop"))
    calibration.set_defaults(handler=_cmd_battery_calibration)

    backlight = subparsers.add_parser("backlight", help="30-second keyboard backlight timeout")
    backlight.add_argument("state", type=_on_off, metavar="{on,off}")
    backlight.set_defaults(handler=_cmd_backlight)

    boot = subparsers.add_parser("boot-animation", help="custom boot animation and sound")
    boot.add_argument("state", type=_on_off, metavar="{on,off}")
    boot.set_defaults(handler=_cmd_boot_animation)

    lcd = subparsers.add_parser("lcd-override", help="LCD override (lower latency, less ghosting)")
    lcd.add_argument("state", type=_on_off, metavar="{on,off}")
    lcd.set_defaults(handler=_cmd_lcd_override)

    usb = subparsers.add_parser("usb-charging", help="off-state USB charging cut-off level")
    usb.add_argument("level", type=int, choices=(0, 10, 20, 30), metavar="LEVEL")
    usb.set_defaults(handler=_cmd_usb_charging)

    thermal = subparsers.add_parser("thermal", help="ACPI platform (thermal) profile")
    thermal.add_argument("profile", nargs="?", help="profile to switch to; omit to show current")
    thermal.set_defaults(handler=_cmd_thermal)

    turbo = subparsers.add_parser("turbo", help="show the turbo hotkey state (read-only)")
    turbo.set_defaults(handler=_cmd_turbo)

    rgb = subparsers.add_parser("rgb", help="four-zone keyboard RGB")
    rgb_sub = rgb.add_subparsers(dest="rgb_command", metavar="<mode>", required=True)
    four = rgb_sub.add_parser("four-zone", help="animated lighting effect")
    four.add_argument("mode", type=int, help="0-7: " + ", ".join(
        f"{number}={name}" for number, name in FOUR_ZONE_MODES.items()))
    four.add_argument("speed", type=int, metavar="SPEED", help="0-9")
    four.add_argument("brightness", type=int, metavar="BRIGHTNESS", help="0-100")
    four.add_argument("direction", type=int, metavar="DIRECTION", help="0 ignored, 1 right->left, 2 left->right")
    four.add_argument("red", type=int, metavar="RED", help="0-255")
    four.add_argument("green", type=int, metavar="GREEN", help="0-255")
    four.add_argument("blue", type=int, metavar="BLUE", help="0-255")
    four.set_defaults(handler=_cmd_four_zone)

    per = rgb_sub.add_parser("per-zone", help="static colour for each zone")
    per.add_argument("zones", nargs=4, metavar="HEX", help="four 6-digit hex colours (RRGGBB)")
    per.add_argument("brightness", type=int, metavar="BRIGHTNESS", help="0-100")
    per.set_defaults(handler=_cmd_per_zone)

    profile = subparsers.add_parser("profile", help="manage saved settings profiles")
    profile_sub = profile.add_subparsers(dest="profile_command", metavar="<action>", required=True)
    save = profile_sub.add_parser("save", help="save the current settings")
    save.add_argument("name")
    save.set_defaults(handler=_cmd_profile_save)
    load = profile_sub.add_parser("load", help="restore a saved profile")
    load.add_argument("name")
    load.set_defaults(handler=_cmd_profile_load)
    profile_sub.add_parser("list", help="list saved profiles").set_defaults(handler=_cmd_profile_list)

    return parser


# -- handlers ------------------------------------------------------------
def _cmd_status(controller: Controller, args: argparse.Namespace) -> int:
    status: dict[str, str] = controller.get_status()

    thermal = Thermal()
    current_profile = thermal.current()
    if current_profile is not None:
        status["thermal_profile"] = current_profile

    try:
        turbo_state = controller.read_turbo_state()
    except LinuwuError:
        turbo_state = None
    if turbo_state is not None:
        status["turbo_state"] = turbo_state

    if args.json:
        print(json.dumps(status, indent=2))
        return 0

    print("Linuwu-Sense status")
    print("=" * 19)
    if controller.model:
        print(f"{'Model interface':<20}{controller.model}")
    for key in STATUS_ATTRIBUTES + ("four_zone_mode", "per_zone_mode", "thermal_profile", "turbo_state"):
        if key in status:
            print(f"{_STATUS_LABELS[key]:<20}{_format_value(key, status[key])}")
    return 0


def _cmd_fan(controller: Controller, args: argparse.Namespace) -> int:
    controller.set_fan_speed(args.cpu, args.gpu)
    print(f"fan speed set: CPU={args.cpu}%, GPU={args.gpu}%")
    return 0


def _cmd_battery_limiter(controller: Controller, args: argparse.Namespace) -> int:
    controller.set_battery_limiter(args.state)
    print(f"battery limiter {'enabled' if args.state else 'disabled'}")
    return 0


def _cmd_battery_calibration(controller: Controller, args: argparse.Namespace) -> int:
    start = args.state == "start"
    controller.set_battery_calibration(start)
    print(f"battery calibration {'started' if start else 'stopped'}")
    return 0


def _cmd_backlight(controller: Controller, args: argparse.Namespace) -> int:
    controller.set_backlight_timeout(args.state)
    print(f"keyboard backlight timeout {'enabled' if args.state else 'disabled'}")
    return 0


def _cmd_boot_animation(controller: Controller, args: argparse.Namespace) -> int:
    controller.set_boot_animation_sound(args.state)
    print(f"boot animation {'enabled' if args.state else 'disabled'}")
    return 0


def _cmd_lcd_override(controller: Controller, args: argparse.Namespace) -> int:
    controller.set_lcd_override(args.state)
    print(f"LCD override {'enabled' if args.state else 'disabled'}")
    return 0


def _cmd_usb_charging(controller: Controller, args: argparse.Namespace) -> int:
    controller.set_usb_charging(args.level)
    if args.level == 0:
        print("USB charging disabled")
    else:
        print(f"USB charging enabled until battery reaches {args.level}%")
    return 0


def _cmd_thermal(controller: Controller, args: argparse.Namespace) -> int:
    thermal = Thermal()
    if args.profile is None:
        current = thermal.current()
        if current is None:
            raise LinuwuError("platform profile is not available on this machine")
        print(f"thermal profile: {current}")
        choices = thermal.choices()
        if choices:
            print("available: " + ", ".join(choices))
        return 0
    thermal.set(args.profile)
    print(f"thermal profile set to {args.profile}")
    return 0


def _cmd_turbo(controller: Controller, args: argparse.Namespace) -> int:
    state = controller.read_turbo_state()
    if state is None:
        raise LinuwuError("turbo_state is not available on this machine")
    print(f"turbo mode: {'on' if state == '1' else 'off'}")
    return 0


def _cmd_four_zone(controller: Controller, args: argparse.Namespace) -> int:
    controller.set_four_zone_mode(
        args.mode, args.speed, args.brightness, args.direction, args.red, args.green, args.blue
    )
    mode_name = FOUR_ZONE_MODES.get(args.mode, str(args.mode))
    print(f"four-zone mode set: {mode_name}, speed={args.speed}, brightness={args.brightness}%")
    return 0


def _cmd_per_zone(controller: Controller, args: argparse.Namespace) -> int:
    controller.set_per_zone_mode(args.zones, args.brightness)
    print(f"per-zone colours set: {', '.join(args.zones)} at brightness {args.brightness}%")
    return 0


def _cmd_profile_save(controller: Controller, args: argparse.Namespace) -> int:
    path = profiles.save_profile(controller, args.name)
    print(f"profile '{args.name}' saved to {path}")
    return 0


def _cmd_profile_load(controller: Controller, args: argparse.Namespace) -> int:
    applied = profiles.load_profile(controller, args.name)
    if applied:
        print(f"profile '{args.name}' loaded: {', '.join(applied)}")
    else:
        print(f"profile '{args.name}' loaded (nothing to apply)")
    return 0


def _cmd_profile_list(controller: Controller, args: argparse.Namespace) -> int:
    names = profiles.list_profiles()
    if not names:
        print("no saved profiles")
        return 0
    for name in names:
        print(name)
    return 0


# -- formatting ----------------------------------------------------------
def _format_value(key: str, value: str) -> str:
    if key in _STATUS_FLAGS:
        return "enabled" if value == "1" else "disabled"
    if key == "battery_calibration":
        return "active" if value == "1" else "inactive"
    if key == "usb_charging":
        return "disabled" if value == "0" else f"until {value}%"
    if key == "turbo_state":
        return "on" if value == "1" else "off"
    if key == "fan_speed":
        cpu, _, gpu = value.partition(",")
        return f"CPU {_fan_value(cpu)}, GPU {_fan_value(gpu)}"
    return value


def _fan_value(value: str) -> str:
    return "auto" if value == "0" else f"{value}%"


# -- entry points --------------------------------------------------------
def main(argv: list[str] | None = None) -> int:
    """Parse ``argv``, run the requested command and return an exit code."""
    parser = build_parser()
    args = parser.parse_args(argv)

    handler: _Handler | None = getattr(args, "handler", None)
    if handler is None:
        parser.print_help()
        return 0

    try:
        return handler(Controller(), args)
    except LinuwuError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        print("interrupted", file=sys.stderr)
        return 130


def run() -> None:
    """Zipapp entry point: run ``main`` and propagate its exit code."""
    raise SystemExit(main())
