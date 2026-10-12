#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:?private W output directory required}
case "$out" in /mnt/w/Daywater/*) ;; *) exit 2 ;; esac
mkdir -p "$out"
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
headers=${DAYWATER_MODERNGEKKO_HEADERS:-/mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/include}
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off \
 -I "$headers" "$root/model_postattach_original_check.c" "$root/model_postattach.c" \
 "$root/actor_model_init.c" "$root/face_resource_pair.c" "$root/model_relocate.c" \
 "$root/model_chunk_relocate.c" "$root/model_view.c" "$root/archive.c" "$root/lzh1.c" "$root/skeleton_resource.c" \
 -o "$out/model-postattach-original-check.exe"
