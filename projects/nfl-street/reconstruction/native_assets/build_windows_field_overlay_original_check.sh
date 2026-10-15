#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:?private output required}
case "$out" in /mnt/w/Daywater/research-temp/nflstreet-frame14-20261015*) ;; *) exit 2 ;; esac
mkdir -p "$out"
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
src=${FIELD_OVERLAY_SOURCE:-$root/field_overlay.c}
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation -ffp-contract=off -Wno-unused-function -I /mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/include -I "$root" \
 "$root/field_overlay_original_check.c" "$src" "$root/scenery_draw.c" "$root/marks_draw.c" "$root/matrix_stack.c" "$root/draw_engine.c" \
 -o "$out/field-overlay-original-check.exe"
