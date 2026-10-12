#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:-/mnt/w/Daywater/research-temp/nflstreet-team-start-args-20261011}
case "$out" in /mnt/w/Daywater/*) ;; *) echo 'Private output must be beneath /mnt/w/Daywater' >&2; exit 2 ;; esac
mkdir -p "$out"
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror \
  -I /mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/include \
  "$root/team_start_args_original_check.c" "$root/team_start_catalog_service.c" "$root/team_catalog.c" \
  "$root/font.c" "$root/../native_assets/texture.c" \
  -o "$out/team-start-args-original-check.exe"
