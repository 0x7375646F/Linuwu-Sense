# linusexcli

A small command-line tool to drive the **Linuwu-Sense** kernel module on Acer
Predator/Nitro laptops: fan speeds, battery limiter/calibration, keyboard RGB,
and the ACPI thermal profile.

The runtime is **standard-library only**. `uv` is used only as a development
toolchain (environment, lockfile, tests); the shipped artifact is a single-file
zipapp built with `python -m zipapp`.

## Layout

| Path | Purpose |
| --- | --- |
| `src/linusexcli/` | Package source (stdlib only) |
| `tests/` | `pytest` suite, runs unprivileged against a fake sysfs tree |
| `build.pyz.sh` | Builds `linusexcli.pyz` (single-file zipapp) |
| `pyproject.toml` | Project metadata + console-script entry point |

## Development with uv

`uv` is **dev-only**; keep `[project].dependencies` empty so the artifact stays
stdlib-only.

```bash
uv sync            # create/refresh .venv from uv.lock
uv run pytest      # run the tests
uv run linusexcli --help
```

Commit `uv.lock`; never commit `.venv/` (gitignored).

## Build the zipapp

```bash
bash build.pyz.sh              # -> linusexcli.pyz
python linusexcli.pyz --help
```

`build.pyz.sh` is the single source of truth for the artifact, and is exactly
what the AUR package (`../aur/linusexcli/`) invokes.

## Usage

```text
linusexcli status [--json]
linusexcli fan CPU GPU                       # 0 = auto, 1-100
linusexcli battery limiter on|off
linusexcli battery calibration start|stop
linusexcli backlight on|off
linusexcli boot-animation on|off
linusexcli lcd-override on|off
linusexcli usb-charging 0|10|20|30
linusexcli thermal                           # show current + supported profiles
linusexcli thermal <profile>                 # switch platform profile
linusexcli turbo                             # read-only turbo hotkey state
linusexcli rgb four-zone MODE SPEED BRIGHTNESS DIRECTION R G B
linusexcli rgb per-zone Z1 Z2 Z3 Z4 BRIGHTNESS
linusexcli profile save|load|list [NAME]
```

Examples:

```bash
linusexcli fan 50 70
linusexcli battery limiter on
linusexcli rgb four-zone 1 4 100 1 255 0 255      # breathing purple
linusexcli rgb per-zone 4287f5 4287f5 4287f5 4287f5 100
linusexcli thermal performance
linusexcli profile save gaming
linusexcli profile load gaming
```

### Notes

- The tool auto-detects `predator_sense` vs `nitro_sense`; commands fail with a
  clear message when the module is not loaded.
- Writes go straight to sysfs. If a write is refused, run with `sudo` or add
  your user to the `linuwu_sense` group (the `make install` path); the AUR
  package makes the attributes world-writable instead.
- Profiles are stored at `~/.config/linuwu-sense/saved_profiles/*.json`, the
  same location and format as the legacy `Linuwu-Sense.py`, so they are
  interchangeable.
- `thermal` and `turbo` degrade gracefully when the firmware does not expose
  them.

### Environment overrides (tests / unusual layouts)

| Variable | Overrides |
| --- | --- |
| `LINUWU_SYSFS_ROOT` | acer-wmi sysfs directory |
| `LINUWU_ACPI_ROOT` | `/sys/firmware/acpi` (thermal profiles) |
| `LINUWU_PROFILE_DIR` | saved-profile directory |
