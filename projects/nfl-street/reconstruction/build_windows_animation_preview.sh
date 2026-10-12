#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
out=${1:-/mnt/w/Daywater/research-temp/nflstreet-animation-20261008}
mkdir -p "$out"
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror \
 "$root/native_assets/animation_preview.c" "$root/native_assets/pose.c" "$root/native_assets/player.c" \
 "$root/native_assets/field_render.c" "$root/native_assets/sky.c" "$root/native_assets/model.c" \
 "$root/native_assets/texture.c" -lm -o "$out/animation-preview.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror \
 "$root/native_assets/animation_window.c" -lgdi32 -luser32 -o "$out/animation-window.exe"
printf '%s\n' "$out"
