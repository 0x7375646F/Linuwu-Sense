# Arch Linux packaging (AUR)

DKMS package for this repository: `linuwu-sense-an515-55-dkms`.

Ships the **driver only** — the `linuwu_sense` module plus its systemd units,
`modules-load.d` / `modprobe.d` / `sysusers.d` / `tmpfiles.d` drop-ins. The GUI,
Turbo OSD and profile notifier are not part of this package.

## Files

| File | Purpose |
| --- | --- |
| `PKGBUILD` | Build recipe; installs the DKMS tree to `/usr/src/linuwu_sense-$pkgver` |
| `.SRCINFO` | Generated metadata (`makepkg --printsrcinfo > .SRCINFO`) |
| `linuwu-sense.install` | Adds/removes the module, prints the group hint |
| `linuwu-sense-setup.service` | Re-applies sysfs permissions after the module loads |

## Build and install

```bash
cd aur
sudo pacman -S --needed base-devel dkms linux-headers
makepkg -si
```

Then join the group once (sysfs controls are group-owned by `linuwu_sense`):

```bash
sudo usermod -aG linuwu_sense "$USER"
```

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
