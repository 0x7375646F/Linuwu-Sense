"""linusexcli — command-line control for the Linuwu-Sense kernel module.

The runtime is standard-library only; ``uv`` is a development toolchain that
builds the environment and the single-file zipapp artifact.
"""

from .cli import __version__, main, run

__all__ = ["__version__", "main", "run"]
