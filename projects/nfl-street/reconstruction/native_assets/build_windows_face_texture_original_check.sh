#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:?private output required}
case "$out" in /mnt/w/Daywater/*) ;; *) exit 2 ;; esac
mkdir -p "$out"
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation \
 -I /mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/include "$root/face_texture_original_check.c" "$root/actor_model_init.c" "$root/face_texture.c" "$root/texture_apply.c" "$root/texture.c" "$root/archive.c" "$root/lzh1.c" \
 "$root/../menus/team_player_resolve.c" "$root/../menus/team_player_manifest.c" "$root/../menus/team_catalog.c" "$root/../menus/team_identity.c" \
 -o "$out/face-texture-original-check.exe"
