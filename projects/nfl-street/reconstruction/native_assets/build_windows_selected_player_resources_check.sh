#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:?private output required}
case "$out" in /mnt/w/Daywater/*) ;; *) exit 2 ;; esac
mkdir -p "$out"
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation -ffp-contract=off \
 "$root/selected_player_resources_check.c" "$root/selected_player_resources.c" "$root/face_resource_pair.c" "$root/face_postattach.c" "$root/model_postattach.c" "$root/skeleton_resource.c" \
 "$root/body_resource_pool.c" "$root/model_view.c" "$root/model_relocate.c" "$root/model_chunk_relocate.c" "$root/archive.c" "$root/lzh1.c" \
 "$root/../menus/team_player_body_binding.c" "$root/../menus/team_player_resolve.c" "$root/../menus/team_player_manifest.c" \
 "$root/../menus/team_catalog.c" "$root/../menus/team_identity.c" -o "$out/selected-player-resources-check.exe"
