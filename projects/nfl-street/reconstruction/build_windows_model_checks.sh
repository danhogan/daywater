#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
out=${1:-/mnt/w/Daywater/research-temp/nflstreet-model-checks.exe}
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror \
 "$root/native_assets/model_checks.c" "$root/native_assets/model.c" "$root/native_assets/mesh_render.c" \
 "$root/native_assets/texture.c" -lm -o "$out"
printf '%s\n' "$out"
