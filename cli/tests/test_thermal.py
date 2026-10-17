"""Tests for the ACPI platform-profile helpers."""

from __future__ import annotations

from pathlib import Path

import pytest

from linusexcli.cli import main
from linusexcli.errors import LinuwuError
from linusexcli.thermal import Thermal


def test_current_and_choices(fake_acpi: Path) -> None:
    thermal = Thermal()
    assert thermal.current() == "balanced"
    assert thermal.choices() == ["low-power", "balanced", "performance"]


def test_set_profile(fake_acpi: Path) -> None:
    Thermal().set("performance")
    assert (fake_acpi / "platform_profile").read_text() == "performance"


def test_set_unknown_profile(fake_acpi: Path) -> None:
    with pytest.raises(LinuwuError):
        Thermal().set("bogus")


def test_thermal_cli_show(fake_acpi: Path, capsys: pytest.CaptureFixture[str]) -> None:
    assert main(["thermal"]) == 0
    out = capsys.readouterr().out
    assert "balanced" in out
    assert "performance" in out


def test_thermal_cli_set(fake_acpi: Path) -> None:
    assert main(["thermal", "performance"]) == 0
    assert (fake_acpi / "platform_profile").read_text() == "performance"


def test_thermal_unavailable(tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]) -> None:
    monkeypatch.setenv("LINUWU_ACPI_ROOT", str(tmp_path / "empty"))
    assert main(["thermal"]) == 1
    assert "not available" in capsys.readouterr().err
