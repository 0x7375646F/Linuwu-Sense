"""Exception types for the linusexcli package.

Every expected failure (bad input, missing hardware, refused write) is raised as
a :class:`LinuwuError` subclass so the CLI can turn it into a friendly message
and a non-zero exit code instead of a traceback.
"""

from __future__ import annotations


class LinuwuError(Exception):
    """Base class for all expected CLI failures."""


class ModuleNotLoadedError(LinuwuError):
    """The linuwu_sense sysfs interface could not be found."""


class AttributeUnavailableError(LinuwuError):
    """A requested sysfs attribute is not exposed by this machine."""


class PermissionDeniedError(LinuwuError):
    """A sysfs write was refused (needs root or the linuwu_sense group)."""


class ValidationError(LinuwuError):
    """A value passed on the command line is out of range or malformed."""


class ProfileNotFoundError(LinuwuError):
    """A requested saved profile does not exist."""
