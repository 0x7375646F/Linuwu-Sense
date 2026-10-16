# Arch Linux packaging (AUR)

This directory holds **two independent packages**, each with its own recipe,
version line and release cycle. Build either one, or both:

| Package | Directory | What it is |
| --- | --- | --- |
| `linuwu-sense-an515-55-dkms` | `aur/` | The DKMS kernel module (this directory) |
| `linusexcli` | `aur/linusexcli/` | A standalone CLI to control the driver |

The CLI has no build-time dependency on the module — it only talks to the
driver's sysfs files at runtime — so it declares the module as an `optdepends`.
`makepkg -si` on the CLI never drags in `dkms`, and installing the module never
drags in the CLI.

## Module package: `linuwu-sense-an515-55-dkms`

Ships the **driver only** — the `linuwu_sense` module plus its systemd units and
`modules-load.d` / `modprobe.d` / `tmpfiles.d` drop-ins. The GUI, Turbo OSD and
profile notifier are not part of this package.

### Files

| File | Purpose |
| --- | --- |
| `PKGBUILD` | Build recipe; installs the DKMS tree to `/usr/src/linuwu_sense-$pkgver` |
| `.SRCINFO` | Generated metadata (`makepkg --printsrcinfo > .SRCINFO`) |
| `linuwu-sense.install` | Applies sysfs permissions and enables the units |
| `linuwu-sense-setup.service` | Re-applies sysfs permissions after the module loads |

### Build and install

```bash
cd aur
sudo pacman -S --needed base-devel dkms linux-headers
makepkg -si
```

No further setup: the module is registered with DKMS (and rebuilds on kernel
upgrades), loads at boot, and its controls are available to every user straight
away.

## CLI package: `linusexcli`

A standard-library-only zipapp built from the repo's `cli/` with
`cli/build.pyz.sh` and installed as `/usr/bin/linusexcli`. It needs only
`python` at runtime.

### Files

| File | Purpose |
| --- | --- |
| `linusexcli/PKGBUILD` | Build recipe; runs `cli/build.pyz.sh` and installs the zipapp |
| `linusexcli/.SRCINFO` | Generated metadata (`makepkg --printsrcinfo > .SRCINFO`) |

### Build and install

```bash
cd aur/linusexcli
sudo pacman -S --needed base-devel python
makepkg -si
```

The driver is optional: the CLI builds and installs on its own, and reports a
clear error if the module is not loaded. Install `linuwu-sense-an515-55-dkms`
(above) to actually control the hardware.

## Permissions

The driver is treated as a whole-machine fix, so the sysfs attributes it exposes
are installed **world-writable** (`0666`) via `tmpfiles.d`. This deliberately
skips the `linuwu_sense` group used by the manual `make install`, so all local
users can adjust fan speed, RGB and the battery limiter with no `usermod`, no
relogin, and no per-user setup.

Trade-off: any local process can also write those files. That is accepted here
because there is no per-user notion of "who owns the fans"; drop to `0660` with
a group if you want to restrict it.

## Versioning

Both recipes clone this repo at `#tag=v$pkgver`, so **each `pkgver` must have a
matching git tag** — but the two packages release independently:

- **Module** (`linuwu-sense-an515-55-dkms`) tracks the DKMS version.
- **CLI** (`linusexcli`) has its own version line (currently `0.1.0`), kept in
  sync with `cli/pyproject.toml`.

To cut a module release:

1. Bump `PACKAGE_VERSION` in `dkms.conf` to the new version (DKMS requires the
   tree to be `/usr/src/linuwu_sense-<PACKAGE_VERSION>`).
2. `git tag v<version> && git push origin v<version>`.
3. Set `pkgver` in `PKGBUILD`, reset `pkgrel=1`, and refresh `.SRCINFO`:
   `makepkg --printsrcinfo > .SRCINFO`.

To cut a CLI release: bump `version` in `cli/pyproject.toml`, tag the repo
`v<version>`, set `pkgver` in `aur/linusexcli/PKGBUILD`, reset `pkgrel=1`, and
refresh `aur/linusexcli/.SRCINFO`.

`package()` also rewrites `PACKAGE_VERSION` to `$pkgver` inside the module's
installed `dkms.conf`, so the DKMS tree name and the config can never drift
apart.

## Validate

```bash
namcap PKGBUILD                    # module
namcap linusexcli/PKGBUILD         # CLI
makepkg -sf                        # module
(cd linusexcli && makepkg -sf)     # CLI
```
