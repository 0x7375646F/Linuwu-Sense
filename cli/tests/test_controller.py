"""Unit tests for the sysfs controller."""

from __future__ import annotations

import os
from pathlib import Path

import pytest

from linusexcli.controller import Controller
from linusexcli.errors import (
    AttributeUnavailableError,
    ModuleNotLoadedError,
    PermissionDeniedError,
    ValidationError,
)


def test_detects_predator_interface(fake_sysfs: Path) -> None:
    controller = Controller()
    assert controller.model == "predator_sense"
    assert controller.four_zone_dir is not None
    assert controller.turbo_dir is not None


def test_missing_module_raises(tmp_path: Path) -> None:
    controller = Controller(root=tmp_path / "empty")
    with pytest.raises(ModuleNotLoadedError):
        controller.require_sense()
    with pytest.raises(ModuleNotLoadedError):
        controller.get_status()


def test_set_fan_speed(fake_sysfs: Path) -> None:
    controller = Controller()
    controller.set_fan_speed(50, 70)
    assert (fake_sysfs / "predator_sense" / "fan_speed").read_text() == "50,70"


@pytest.mark.parametrize("cpu,gpu", [(-1, 0), (0, 101), (150, 20)])
def test_fan_speed_validation(fake_sysfs: Path, cpu: int, gpu: int) -> None:
    with pytest.raises(ValidationError):
        Controller().set_fan_speed(cpu, gpu)


def test_usb_charging_validation(fake_sysfs: Path) -> None:
    with pytest.raises(ValidationError):
        Controller().set_usb_charging(15)


def test_boolean_controls(fake_sysfs: Path) -> None:
    controller = Controller()
    controller.set_battery_limiter(True)
    controller.set_lcd_override(True)
    assert (fake_sysfs / "predator_sense" / "battery_limiter").read_text() == "1"
    assert (fake_sysfs / "predator_sense" / "lcd_override").read_text() == "1"


def test_four_zone_mode(fake_sysfs: Path) -> None:
    Controller().set_four_zone_mode(1, 4, 100, 1, 255, 0, 255)
    written = (fake_sysfs / "four_zoned_kb" / "four_zone_mode").read_text()
    assert written == "1,4,100,1,255,0,255"


def test_four_zone_validation(fake_sysfs: Path) -> None:
    with pytest.raises(ValidationError):
        Controller().set_four_zone_mode(9, 0, 100, 0, 0, 0, 0)


def test_per_zone_mode(fake_sysfs: Path) -> None:
    Controller().set_per_zone_mode(["4287f5", "ff5733", "33ff57", "ff33a6"], 80)
    written = (fake_sysfs / "four_zoned_kb" / "per_zone_mode").read_text()
    assert written == "4287f5,ff5733,33ff57,ff33a6,80"


@pytest.mark.parametrize("colour", ["12345", "gggggg", "zzzzzz"])
def test_per_zone_hex_validation(fake_sysfs: Path, colour: str) -> None:
    with pytest.raises(ValidationError):
        Controller().set_per_zone_mode([colour, "000000", "000000", "000000"], 100)


def test_turbo_state(fake_sysfs: Path) -> None:
    assert Controller().read_turbo_state() == "0"


def test_turbo_missing(tmp_path: Path) -> None:
    base = tmp_path / "acer-wmi"
    (base / "predator_sense").mkdir(parents=True)
    with pytest.raises(AttributeUnavailableError):
        Controller(root=base).read_turbo_state()


def test_status_reads_seeded_values(fake_sysfs: Path) -> None:
    status = Controller().get_status()
    assert status["fan_speed"] == "0,0"
    assert status["boot_animation_sound"] == "1"
    assert status["four_zone_mode"] == "0,0,100,0,255,255,255"


@pytest.mark.skipif(os.geteuid() == 0, reason="root bypasses file permissions")
def test_write_permission_denied(fake_sysfs: Path) -> None:
    target = fake_sysfs / "predator_sense" / "fan_speed"
    target.chmod(0o000)
    try:
        with pytest.raises(PermissionDeniedError):
            Controller().set_fan_speed(10, 10)
    finally:
        target.chmod(0o644)
