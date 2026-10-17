"""End-to-end tests for the command-line entry point."""

from __future__ import annotations

import json
from pathlib import Path

import pytest

from linusexcli.cli import main


def test_no_command_prints_help(capsys: pytest.CaptureFixture[str]) -> None:
    assert main([]) == 0
    assert "usage:" in capsys.readouterr().out


def test_unknown_command_exits_2() -> None:
    with pytest.raises(SystemExit) as excinfo:
        main(["definitely-not-a-command"])
    assert excinfo.value.code == 2


def test_status_json(fake_sysfs: Path, capsys: pytest.CaptureFixture[str]) -> None:
    assert main(["status", "--json"]) == 0
    data = json.loads(capsys.readouterr().out)
    assert data["fan_speed"] == "0,0"
    assert data["turbo_state"] == "0"
    assert "thermal_profile" not in data


def test_fan_sets_value(fake_sysfs: Path, capsys: pytest.CaptureFixture[str]) -> None:
    assert main(["fan", "55", "65"]) == 0
    assert (fake_sysfs / "predator_sense" / "fan_speed").read_text() == "55,65"
    assert "CPU=55%" in capsys.readouterr().out


def test_fan_rejects_out_of_range(fake_sysfs: Path, capsys: pytest.CaptureFixture[str]) -> None:
    assert main(["fan", "200", "10"]) == 1
    assert "error:" in capsys.readouterr().err


def test_battery_limiter(fake_sysfs: Path) -> None:
    assert main(["battery", "limiter", "on"]) == 0
    assert (fake_sysfs / "predator_sense" / "battery_limiter").read_text() == "1"


def test_battery_calibration(fake_sysfs: Path) -> None:
    assert main(["battery", "calibration", "start"]) == 0
    assert (fake_sysfs / "predator_sense" / "battery_calibration").read_text() == "1"


def test_usb_charging(fake_sysfs: Path) -> None:
    assert main(["usb-charging", "30"]) == 0
    assert (fake_sysfs / "predator_sense" / "usb_charging").read_text() == "30"


def test_rgb_four_zone(fake_sysfs: Path) -> None:
    assert main(["rgb", "four-zone", "1", "4", "100", "1", "255", "0", "255"]) == 0
    written = (fake_sysfs / "four_zoned_kb" / "four_zone_mode").read_text()
    assert written == "1,4,100,1,255,0,255"


def test_rgb_per_zone(fake_sysfs: Path) -> None:
    assert main(["rgb", "per-zone", "4287f5", "ff5733", "33ff57", "ff33a6", "90"]) == 0
    written = (fake_sysfs / "four_zoned_kb" / "per_zone_mode").read_text()
    assert written == "4287f5,ff5733,33ff57,ff33a6,90"


def test_missing_module_is_reported(tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]) -> None:
    monkeypatch.setenv("LINUWU_SYSFS_ROOT", str(tmp_path / "nothing"))
    assert main(["status"]) == 1
    assert "not detected" in capsys.readouterr().err
