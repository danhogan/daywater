#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:?private output required}
case "$out" in /mnt/w/Daywater/research-temp/*) ;; *) exit 2 ;; esac
mkdir -p "$out"
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/mnt/w/Daywater/research-temp/nflstreet-disc-constants-20261017/zig-cache}
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
nod=${DAYWATER_NOD:-/mnt/w/Daywater/research-temp/nflstreet-live-20261013/build/_deps/nod_prebuilt-src}
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation \
 -I "$nod/include" "$root/disc_constants_original_check.c" "$root/dol_sections.c" "$root/disc_dol.c" \
 "$nod/lib/nod.lib" -o "$out/disc-constants-original-check.exe"
cp "$nod/bin/nod.dll" "$out/nod.dll"
