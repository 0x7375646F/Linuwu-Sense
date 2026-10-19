"""Tests for saved-profile save/load/list."""

from __future__ import annotations

from pathlib import Path

import pytest

from linusexcli import profiles
from linusexcli.cli import main
from linusexcli.controller import Controller
from linusexcli.errors import ProfileNotFoundError


def test_save_and_list(fake_sysfs: Path, profile_dir: Path) -> None:
    path = profiles.save_profile(Controller(), "gaming")
    assert path == profile_dir / "gaming.json"
    assert profile_dir.is_dir()
    assert profiles.list_profiles() == ["gaming"]


def test_list_empty(profile_dir: Path) -> None:
    assert profiles.list_profiles() == []


def test_round_trip_restores_settings(fake_sysfs: Path, profile_dir: Path) -> None:
    controller = Controller()
    controller.set_fan_speed(70, 80)
    controller.set_battery_limiter(True)
    profiles.save_profile(controller, "work")

    # Change everything, then load the profile back.
    controller.set_fan_speed(0, 0)
    controller.set_battery_limiter(False)
    applied = profiles.load_profile(controller, "work")

    assert "fan_speed" in applied
    assert controller.get_status()["fan_speed"] == "70,80"
    assert controller.get_status()["battery_limiter"] == "1"


def test_load_missing_profile(fake_sysfs: Path, profile_dir: Path) -> None:
    with pytest.raises(ProfileNotFoundError):
        profiles.load_profile(Controller(), "nope")


def test_profile_cli_round_trip(fake_sysfs: Path, profile_dir: Path, capsys: pytest.CaptureFixture[str]) -> None:
    assert main(["fan", "42", "24"]) == 0
    assert main(["profile", "save", "custom"]) == 0
    assert main(["profile", "list"]) == 0
    assert "custom" in capsys.readouterr().out

    # Overwrite, then load the saved profile.
    assert main(["fan", "1", "1"]) == 0
    assert main(["profile", "load", "custom"]) == 0
    capsys.readouterr()
    assert (fake_sysfs / "predator_sense" / "fan_speed").read_text() == "42,24"
