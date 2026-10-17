"""sysfs interface to the ``linuwu_sense`` kernel module.

The module exposes its controls under one of two device directories
(``predator_sense`` or ``nitro_sense``) plus an optional ``four_zoned_kb``
directory and a ``turbo_sense`` directory. Everything lives below the acer-wmi
platform directory created by the driver::

    /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/

Set ``LINUWU_SYSFS_ROOT`` (or pass ``root=``) to point at a different location,
which is what the tests use to run unprivileged against a fake tree.
"""

from __future__ import annotations

import os
from pathlib import Path

from .errors import (
    AttributeUnavailableError,
    LinuwuError,
    ModuleNotLoadedError,
    PermissionDeniedError,
    ValidationError,
)

#: Default acer-wmi platform directory created by the module.
DEFAULT_SYSFS_ROOT = Path("/sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi")

#: Environment variable overriding :data:`DEFAULT_SYSFS_ROOT`.
SYSFS_ROOT_ENV = "LINUWU_SYSFS_ROOT"

SENSE_DIRS = ("predator_sense", "nitro_sense")
FOUR_ZONE_DIR = "four_zoned_kb"
TURBO_DIR = "turbo_sense"
TURBO_STATE_FILE = "turbo_state"

#: USB charging levels accepted by the driver.
USB_CHARGING_LEVELS = (0, 10, 20, 30)

#: four-zone keyboard lighting effects.
FOUR_ZONE_MODES = {
    0: "static",
    1: "breathing",
    2: "neon",
    3: "wave",
    4: "shifting",
    5: "zoom",
    6: "meteor",
    7: "twinkling",
}

#: Sense attributes shown by ``status``, in display order.
STATUS_ATTRIBUTES = (
    "fan_speed",
    "battery_limiter",
    "battery_calibration",
    "backlight_timeout",
    "boot_animation_sound",
    "lcd_override",
    "usb_charging",
)

#: four_zoned_kb attributes shown by ``status``.
FOUR_ZONE_ATTRIBUTES = ("four_zone_mode", "per_zone_mode")

_HEX_DIGITS = set("0123456789abcdefABCDEF")


class Controller:
    """Read and write the Linuwu-Sense sysfs controls."""

    def __init__(self, root: str | os.PathLike[str] | None = None) -> None:
        if root is None:
            root = os.environ.get(SYSFS_ROOT_ENV) or DEFAULT_SYSFS_ROOT
        self.root = Path(root)

    # -- discovery -------------------------------------------------------
    @property
    def sense_dir(self) -> Path | None:
        """The detected ``predator_sense``/``nitro_sense`` directory, if any."""
        for name in SENSE_DIRS:
            candidate = self.root / name
            if candidate.is_dir():
                return candidate
        return None

    @property
    def four_zone_dir(self) -> Path | None:
        """The ``four_zoned_kb`` directory, if this machine has one."""
        candidate = self.root / FOUR_ZONE_DIR
        return candidate if candidate.is_dir() else None

    @property
    def turbo_dir(self) -> Path | None:
        """The ``turbo_sense`` directory, if this machine has one."""
        candidate = self.root / TURBO_DIR
        return candidate if candidate.is_dir() else None

    @property
    def model(self) -> str | None:
        """``"predator_sense"``/``"nitro_sense"``, or ``None`` when absent."""
        directory = self.sense_dir
        return directory.name if directory is not None else None

    def require_sense(self) -> Path:
        """Return the sense directory or raise :class:`ModuleNotLoadedError`."""
        directory = self.sense_dir
        if directory is None:
            raise ModuleNotLoadedError(
                "Linuwu-Sense module not detected or not loaded "
                f"(looked for predator_sense/nitro_sense under {self.root})"
            )
        return directory

    def require_four_zone(self) -> Path:
        """Return the keyboard directory or raise if the machine lacks it."""
        directory = self.four_zone_dir
        if directory is None:
            raise AttributeUnavailableError(
                "four-zone keyboard RGB is not available on this machine"
            )
        return directory

    # -- low-level sysfs access -----------------------------------------
    @staticmethod
    def _read(directory: Path, attribute: str) -> str | None:
        try:
            return (directory / attribute).read_text().strip()
        except (OSError, ValueError):
            return None

    @staticmethod
    def _write(directory: Path, attribute: str, value: str) -> None:
        path = directory / attribute
        try:
            # sysfs accepts an optional trailing newline; write the bare value
            # to match the existing Linuwu-Sense.py behaviour.
            with path.open("w") as handle:
                handle.write(value)
        except PermissionError as exc:
            raise PermissionDeniedError(
                f"permission denied writing {path}; run with sudo or add your "
                "user to the linuwu_sense group"
            ) from exc
        except FileNotFoundError as exc:
            raise AttributeUnavailableError(
                f"'{attribute}' is not available on this machine ({path})"
            ) from exc
        except OSError as exc:
            raise LinuwuError(f"failed to write {path}: {exc}") from exc

    # -- status ----------------------------------------------------------
    def get_status(self) -> dict[str, str]:
        """Return every sense/keyboard attribute this machine exposes."""
        directory = self.require_sense()
        status: dict[str, str] = {}
        for attribute in STATUS_ATTRIBUTES:
            value = self._read(directory, attribute)
            if value is not None:
                status[attribute] = value

        keyboard = self.four_zone_dir
        if keyboard is not None:
            for attribute in FOUR_ZONE_ATTRIBUTES:
                value = self._read(keyboard, attribute)
                if value is not None:
                    status[attribute] = value
        return status

    # -- fan -------------------------------------------------------------
    def set_fan_speed(self, cpu: int, gpu: int) -> None:
        """Set CPU and GPU fan speeds (0 = auto, 1-100 = fixed percentage)."""
        for label, value in (("CPU", cpu), ("GPU", gpu)):
            if not 0 <= value <= 100:
                raise ValidationError(f"{label} fan speed must be between 0 and 100")
        self._write(self.require_sense(), "fan_speed", f"{cpu},{gpu}")

    # -- battery ---------------------------------------------------------
    def set_battery_limiter(self, enabled: bool) -> None:
        """Enable/disable the 80% charging limit."""
        self._write(self.require_sense(), "battery_limiter", _flag(enabled))

    def set_battery_calibration(self, start: bool) -> None:
        """Start/stop battery calibration."""
        self._write(self.require_sense(), "battery_calibration", _flag(start))

    # -- miscellaneous ---------------------------------------------------
    def set_backlight_timeout(self, enabled: bool) -> None:
        """Enable/disable the 30-second keyboard backlight timeout."""
        self._write(self.require_sense(), "backlight_timeout", _flag(enabled))

    def set_boot_animation_sound(self, enabled: bool) -> None:
        """Enable/disable the custom boot animation and sound."""
        self._write(self.require_sense(), "boot_animation_sound", _flag(enabled))

    def set_lcd_override(self, enabled: bool) -> None:
        """Enable/disable the LCD override (lower latency, less ghosting)."""
        self._write(self.require_sense(), "lcd_override", _flag(enabled))

    def set_usb_charging(self, level: int) -> None:
        """Set the off-state USB charging cut-off (0 disables it)."""
        if level not in USB_CHARGING_LEVELS:
            allowed = ", ".join(str(value) for value in USB_CHARGING_LEVELS)
            raise ValidationError(f"USB charging level must be one of: {allowed}")
        self._write(self.require_sense(), "usb_charging", str(level))

    # -- keyboard RGB ----------------------------------------------------
    def set_four_zone_mode(
        self,
        mode: int,
        speed: int,
        brightness: int,
        direction: int,
        red: int,
        green: int,
        blue: int,
    ) -> None:
        """Set an animated four-zone lighting effect."""
        if mode not in FOUR_ZONE_MODES:
            raise ValidationError("mode must be between 0 and 7")
        if not 0 <= speed <= 9:
            raise ValidationError("speed must be between 0 and 9")
        if not 0 <= brightness <= 100:
            raise ValidationError("brightness must be between 0 and 100")
        if not 0 <= direction <= 2:
            raise ValidationError(
                "direction must be 0 (ignored), 1 (right to left) or 2 (left to right)"
            )
        for label, value in (("red", red), ("green", green), ("blue", blue)):
            if not 0 <= value <= 255:
                raise ValidationError(f"{label} must be between 0 and 255")

        value = f"{mode},{speed},{brightness},{direction},{red},{green},{blue}"
        self._write(self.require_four_zone(), "four_zone_mode", value)

    def set_per_zone_mode(self, zones: list[str] | tuple[str, ...], brightness: int) -> None:
        """Set a static colour for each of the four keyboard zones."""
        if len(zones) != 4:
            raise ValidationError("expected exactly four zone colours")
        if not 0 <= brightness <= 100:
            raise ValidationError("brightness must be between 0 and 100")
        for index, colour in enumerate(zones, 1):
            if len(colour) != 6 or any(char not in _HEX_DIGITS for char in colour):
                raise ValidationError(
                    f"zone {index} colour must be a 6-digit hex value like '4287f5'"
                )
        self._write(
            self.require_four_zone(),
            "per_zone_mode",
            f"{','.join(zones)},{brightness}",
        )

    # -- turbo -----------------------------------------------------------
    def read_turbo_state(self) -> str | None:
        """Return the read-only turbo hotkey state, if this machine has one."""
        directory = self.turbo_dir
        if directory is None:
            raise AttributeUnavailableError(
                "turbo_state is not available on this machine"
            )
        return self._read(directory, TURBO_STATE_FILE)


def _flag(value: bool) -> str:
    return "1" if value else "0"
