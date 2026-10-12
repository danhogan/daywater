#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:-/mnt/w/Daywater/research-temp/nflstreet-team-labels-20261010}
case "$out" in /mnt/w/Daywater/*) ;; *) echo 'Private output must be beneath /mnt/w/Daywater' >&2; exit 2 ;; esac
mkdir -p "$out"
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
headers=${DAYWATER_MODERNGEKKO_HEADERS:-/mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/include}
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off \
  -I "$headers" -I "$root" \
  "$root/team_labels_check.c" "$root/team_callback.c" "$root/team_start.c" \
  "$root/team_selection.c" "$root/team_catalog.c" "$root/team_carousel.c" \
  -o "$out/team-labels-check.exe"
