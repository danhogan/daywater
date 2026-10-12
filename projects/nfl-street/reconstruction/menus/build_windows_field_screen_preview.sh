#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:-/mnt/w/Daywater/research-temp/nflstreet-field-screen-preview-20261011}
case "$out" in /mnt/w/Daywater/*) ;; *) echo 'Private output must be beneath /mnt/w/Daywater' >&2; exit 2 ;; esac
mkdir -p "$out"
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off \
  -I "$root" -I "$root/../native_assets" \
  "$root/field_screen_preview.c" "$root/team_classes.c" "$root/construct.c" \
  "$root/screen.c" "$root/text_construct.c" "$root/text.c" "$root/text_lines.c" \
  "$root/image_construct.c" "$root/font.c" "$root/../native_assets/texture.c" \
  -o "$out/field-screen-preview.exe" -lgdi32 -luser32
echo "Built $out/field-screen-preview.exe"
