#!/usr/bin/env bash
set -euo pipefail
here=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:?private output required}
case "$out" in /mnt/w/Daywater/research-temp/nflstreet-disc-20261015|/mnt/w/Daywater/research-temp/nflstreet-disc-20261015/*) ;; *) exit 2 ;; esac
mkdir -p "$out"
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
nod=${DAYWATER_NOD:-/mnt/w/Daywater/research-temp/nflstreet-live-20261013/build/_deps/nod_prebuilt-src}
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation \
 -I "$nod/include" "$here/disc_files_check.c" "$here/disc_files.c" "$here/archive.c" "$here/lzh1.c" \
 "$nod/lib/nod.lib" -o "$out/disc-files-check.exe"
cp "$nod/bin/nod.dll" "$out/nod.dll"
if [[ ${2:-} == --live ]]; then
 aurora=/mnt/w/Daywater/tools/aurora
 sdl=/mnt/w/Daywater/research-temp/nflstreet-live-20261013/build/_deps/sdl3_prebuilt-src
 "$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation \
  -ffp-contract=off -DAURORA -DTARGET_PC=1 -I "$aurora/include" -I "$sdl/include" -I "$here" \
  -c "$here/../../native/live/live_main.c" -o "$out/live_main.obj"
fi
