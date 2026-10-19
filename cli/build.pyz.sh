#!/usr/bin/env bash
# Build linusexcli.pyz — a single-file zipapp with a standard-library-only
# runtime, so it needs no packages and nothing to install at run time.
#
# Usage:  bash build.pyz.sh [output.pyz]
set -euo pipefail

cd -- "$(dirname -- "$0")"

python="${PYTHON:-python3}"
out="${1:-linusexcli.pyz}"

staging="$(mktemp -d)"
trap 'rm -rf "$staging"' EXIT

cp -r src/linusexcli "$staging/linusexcli"
find "$staging" -type d -name __pycache__ -prune -exec rm -rf {} +
find "$staging" -type f \( -name '*.pyc' -o -name '*.pyo' \) -delete

"$python" -m zipapp "$staging" \
    --main 'linusexcli.cli:run' \
    --python '/usr/bin/env python3' \
    --compress \
    --output "$out"

chmod +x "$out"
echo "built $out ($(du -h "$out" | cut -f1))"
