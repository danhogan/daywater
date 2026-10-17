#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:?private output required}
case "$out" in /mnt/w/Daywater/research-temp/*) ;; *) exit 2 ;; esac
mkdir -p "$out"
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/home/dan/.cache/daywater-zig-cache}
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
nod=${DAYWATER_NOD:-/mnt/w/Daywater/research-temp/nflstreet-live-20261013/build/_deps/nod_prebuilt-src}
# Scratch mutant build uses only its changed adapter; shared consumers stay pinned.
shared=${DAYWATER_GEAR_SHARED:-$root}
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation \
 -I "$shared" -I "$nod/include" "$root/disc_gear_catalog_check.c" "$root/disc_gear_catalog.c" \
 "$shared/dol_sections.c" "$shared/disc_dol.c" "$shared/gear_texture.c" "$shared/headwear_state.c" \
 "$shared/archive.c" "$shared/lzh1.c" "$shared/../menus/team_catalog.c" "$nod/lib/nod.lib" \
 -o "$out/disc-gear-catalog-check.exe"
cp "$nod/bin/nod.dll" "$out/nod.dll"
