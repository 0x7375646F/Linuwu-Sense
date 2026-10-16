# Arch Linux packaging (AUR)

DKMS package for this repository: `linuwu-sense-an515-55-dkms`.

Ships the **driver only** — the `linuwu_sense` module plus its systemd units and
`modules-load.d` / `modprobe.d` / `tmpfiles.d` drop-ins. The GUI, Turbo OSD and
profile notifier are not part of this package.

## Files

| File | Purpose |
| --- | --- |
| `PKGBUILD` | Build recipe; installs the DKMS tree to `/usr/src/linuwu_sense-$pkgver` |
| `.SRCINFO` | Generated metadata (`makepkg --printsrcinfo > .SRCINFO`) |
| `linuwu-sense.install` | Applies sysfs permissions and enables the units |
| `linuwu-sense-setup.service` | Re-applies sysfs permissions after the module loads |

## Build and install

```bash
cd aur
sudo pacman -S --needed base-devel dkms linux-headers
makepkg -si
```

No further setup: the module is registered with DKMS (and rebuilds on kernel
upgrades), loads at boot, and its controls are available to every user straight
away.

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

The recipe clones this repo at `#tag=v$pkgver`, so **`pkgver` and the git tag
must match**. To cut a release:

1. Bump `PACKAGE_VERSION` in `dkms.conf` to the new version (DKMS requires the
   tree to be `/usr/src/linuwu_sense-<PACKAGE_VERSION>`).
2. `git tag v<version> && git push origin v<version>`.
3. Set `pkgver` in `PKGBUILD`, reset `pkgrel=1`, and refresh `.SRCINFO`:
   `makepkg --printsrcinfo > .SRCINFO`.

`package()` also rewrites `PACKAGE_VERSION` to `$pkgver` inside the installed
`dkms.conf`, so the DKMS tree name and the config can never drift apart.

## Validate

```bash
namcap PKGBUILD
makepkg -sf
```
