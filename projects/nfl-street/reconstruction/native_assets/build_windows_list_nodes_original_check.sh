#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:?private output required}
case "$out" in /mnt/w/Daywater/research-temp/*) ;; *) exit 2 ;; esac
mkdir -p "$out"
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/home/dan/.cache/daywater-zig-cache}
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation \
 -I /mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/include \
 "$root/list_nodes_original_check.c" "$root/list_nodes.c" "$root/scene_owner.c" "$root/scene_insert.c" "$root/registered_pool.c" "$root/object_pool.c" "$root/actor_factory.c" \
 -o "$out/list-nodes-original-check.exe"
