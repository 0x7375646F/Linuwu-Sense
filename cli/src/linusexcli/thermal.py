"""ACPI platform (thermal) profile support.

The thermal profiles are exposed by the kernel's generic ``platform_profile``
interface, not by the linuwu_sense module::

    /sys/firmware/acpi/platform_profile
    /sys/firmware/acpi/platform_profile_choices

Set ``LINUWU_ACPI_ROOT`` (or pass ``root=``) to point elsewhere; the tests use
this to run against a fake tree.
"""

from __future__ import annotations

import os
from pathlib import Path

from .errors import AttributeUnavailableError, LinuwuError, PermissionDeniedError

DEFAULT_ACPI_ROOT = Path("/sys/firmware/acpi")

#: Environment variable overriding :data:`DEFAULT_ACPI_ROOT`.
ACPI_ROOT_ENV = "LINUWU_ACPI_ROOT"

PROFILE_FILE = "platform_profile"
CHOICES_FILE = "platform_profile_choices"


class Thermal:
    """Read and switch the ACPI platform profile."""

    def __init__(self, root: str | os.PathLike[str] | None = None) -> None:
        if root is None:
            root = os.environ.get(ACPI_ROOT_ENV) or DEFAULT_ACPI_ROOT
        self.root = Path(root)

    @property
    def profile_path(self) -> Path:
        return self.root / PROFILE_FILE

    @property
    def choices_path(self) -> Path:
        return self.root / CHOICES_FILE

    def current(self) -> str | None:
        """Return the active profile, or ``None`` when unsupported."""
        try:
            return self.profile_path.read_text().strip()
        except OSError:
            return None

    def choices(self) -> list[str]:
        """Return the supported profiles (empty list when unsupported)."""
        try:
            return self.choices_path.read_text().split()
        except OSError:
            return []

    def set(self, profile: str) -> None:
        """Switch to ``profile``, rejecting names the firmware does not offer."""
        available = self.choices()
        if available and profile not in available:
            raise LinuwuError(
                f"unknown thermal profile '{profile}' "
                f"(supported: {', '.join(available)})"
            )
        if not self.profile_path.exists():
            raise AttributeUnavailableError(
                "platform profile is not available on this machine "
                f"({self.profile_path})"
            )
        try:
            with self.profile_path.open("w") as handle:
                handle.write(profile)
        except PermissionError as exc:
            raise PermissionDeniedError(
                f"permission denied writing {self.profile_path}; run with sudo"
            ) from exc
        except OSError as exc:
            raise LinuwuError(f"failed to write {self.profile_path}: {exc}") from exc
