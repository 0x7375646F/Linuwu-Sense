"""Shared fixtures: a fake sysfs/acpi tree so tests need neither hardware nor root."""

from __future__ import annotations

from pathlib import Path

import pytest

#: Seed values written into the fake predator_sense directory.
SENSE_FILES = {
    "fan_speed": "0,0",
    "battery_limiter": "0",
    "battery_calibration": "0",
    "backlight_timeout": "0",
    "boot_animation_sound": "1",
    "lcd_override": "0",
    "usb_charging": "20",
}


@pytest.fixture
def fake_sysfs(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> Path:
    """A populated acer-wmi tree; points the CLI at it via LINUWU_SYSFS_ROOT."""
    base = tmp_path / "acer-wmi"

    sense = base / "predator_sense"
    sense.mkdir(parents=True)
    for name, value in SENSE_FILES.items():
        (sense / name).write_text(value + "\n")

    keyboard = base / "four_zoned_kb"
    keyboard.mkdir()
    (keyboard / "four_zone_mode").write_text("0,0,100,0,255,255,255\n")
    (keyboard / "per_zone_mode").write_text("4287f5,4287f5,4287f5,4287f5,100\n")

    turbo = base / "turbo_sense"
    turbo.mkdir()
    (turbo / "turbo_state").write_text("0\n")

    monkeypatch.setenv("LINUWU_SYSFS_ROOT", str(base))
    # Keep thermal detection hermetic: point at a path that does not exist.
    monkeypatch.setenv("LINUWU_ACPI_ROOT", str(tmp_path / "no-acpi"))
    return base


@pytest.fixture
def fake_acpi(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> Path:
    """A fake /sys/firmware/acpi tree with platform-profile support."""
    root = tmp_path / "acpi"
    root.mkdir()
    (root / "platform_profile").write_text("balanced\n")
    (root / "platform_profile_choices").write_text("low-power balanced performance\n")
    monkeypatch.setenv("LINUWU_ACPI_ROOT", str(root))
    return root


@pytest.fixture
def profile_dir(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> Path:
    """A throwaway saved-profile directory."""
    directory = tmp_path / "profiles"
    monkeypatch.setenv("LINUWU_PROFILE_DIR", str(directory))
    return directory
