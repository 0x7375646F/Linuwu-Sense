"""Save and restore named settings profiles.

Profiles are plain JSON snapshots written to
``~/.config/linuwu-sense/saved_profiles/<name>.json`` — the same location and
format used by the original ``Linuwu-Sense.py``, so the two stay
interchangeable. Set ``LINUWU_PROFILE_DIR`` to override the directory.
"""

from __future__ import annotations

import json
import os
from pathlib import Path

from .controller import Controller
from .errors import LinuwuError, ProfileNotFoundError

#: Environment variable overriding the profile directory.
PROFILE_DIR_ENV = "LINUWU_PROFILE_DIR"

#: Default profile location (shared with the legacy control script).
DEFAULT_PROFILE_DIR = Path.home() / ".config" / "linuwu-sense" / "saved_profiles"

#: Sense attributes restored from a profile. ``battery_calibration`` is
#: deliberately excluded: loading a profile must not kick off a calibration.
_RESTORE_FLAGS = (
    "backlight_timeout",
    "boot_animation_sound",
    "lcd_override",
)


def profile_dir() -> Path:
    override = os.environ.get(PROFILE_DIR_ENV)
    return Path(override) if override else DEFAULT_PROFILE_DIR


def profile_path(name: str) -> Path:
    return profile_dir() / f"{name}.json"


def list_profiles() -> list[str]:
    """Return the sorted names of saved profiles (without extension)."""
    directory = profile_dir()
    if not directory.is_dir():
        return []
    return sorted(path.stem for path in directory.glob("*.json"))


def save_profile(controller: Controller, name: str) -> Path:
    """Write the controller's current status to ``<name>.json``."""
    status = controller.get_status()
    directory = profile_dir()
    directory.mkdir(parents=True, exist_ok=True)
    path = directory / f"{name}.json"
    path.write_text(json.dumps(status, indent=2) + "\n")
    return path


def load_profile(controller: Controller, name: str) -> list[str]:
    """Apply a saved profile and return the list of settings that were applied."""
    path = profile_path(name)
    if not path.is_file():
        raise ProfileNotFoundError(f"profile '{name}' not found")
    try:
        data = json.loads(path.read_text())
    except (OSError, ValueError) as exc:
        raise LinuwuError(f"could not read profile '{name}': {exc}") from exc

    applied: list[str] = []

    if "fan_speed" in data:
        cpu, gpu = (int(part) for part in str(data["fan_speed"]).split(","))
        controller.set_fan_speed(cpu, gpu)
        applied.append("fan_speed")

    if "battery_limiter" in data:
        controller.set_battery_limiter(str(data["battery_limiter"]) == "1")
        applied.append("battery_limiter")

    if "usb_charging" in data:
        controller.set_usb_charging(int(data["usb_charging"]))
        applied.append("usb_charging")

    for key in _RESTORE_FLAGS:
        if key in data:
            _apply_flag(controller, key, data[key])
            applied.append(key)

    # Keyboard settings are skipped on machines without the four-zone interface
    # so a profile saved on a Predator still loads on a Nitro that lacks RGB.
    if controller.four_zone_dir is not None:
        if "four_zone_mode" in data:
            parts = (int(part) for part in str(data["four_zone_mode"]).split(","))
            values = list(parts)
            if len(values) != 7:
                raise LinuwuError("invalid four_zone_mode value in profile")
            controller.set_four_zone_mode(*values)
            applied.append("four_zone_mode")

        if "per_zone_mode" in data:
            parts = str(data["per_zone_mode"]).split(",")
            if len(parts) != 5:
                raise LinuwuError("invalid per_zone_mode value in profile")
            controller.set_per_zone_mode(parts[:4], int(parts[4]))
            applied.append("per_zone_mode")

    return applied


def _apply_flag(controller: Controller, key: str, value: object) -> None:
    enabled = str(value) == "1"
    {
        "backlight_timeout": controller.set_backlight_timeout,
        "boot_animation_sound": controller.set_boot_animation_sound,
        "lcd_override": controller.set_lcd_override,
    }[key](enabled)
