#!/bin/sh
# DKMS post_build hook (see dkms.conf's POST_BUILD). Runs as root, with cwd
# set to the dkms build tree (so ./src/linuwu_sense.ko is the freshly built
# module). Signs it with the MOK keys from module_signing_readme, if present.
# Safe to run on systems without Secure Boot / without keys: it just skips.
set -e

kernelver="$1"
ko="src/linuwu_sense.ko"

real_user="${SUDO_USER:-$(logname 2>/dev/null || true)}"
real_home=$(getent passwd "$real_user" 2>/dev/null | cut -d: -f6)
[ -n "$real_home" ] || real_home="$HOME"

priv="$real_home/module-signing/MOK.priv"
der="$real_home/module-signing/MOK.der"

if [ ! -f "$priv" ] || [ ! -f "$der" ]; then
    echo "MOK keys not found in $real_home/module-signing/. Skipping module signing (common for non-Secure Boot)."
    exit 0
fi

sign_tool="/lib/modules/$kernelver/build/scripts/sign-file"
if [ ! -x "$sign_tool" ]; then
    sign_tool="/usr/src/linux-headers-$kernelver/scripts/sign-file"
fi
if [ ! -x "$sign_tool" ]; then
    echo "ERROR: sign-file tool not found, but MOK keys exist." >&2
    exit 1
fi

echo "Signing module $ko using $sign_tool"
"$sign_tool" sha256 "$priv" "$der" "$ko"
