#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:?private output required}
src=${2:-$root} # a scratch copy of the player-marker sources (mutation tests); shared files always come from here
case "$out" in /mnt/w/Daywater/*) ;; *) exit 2 ;; esac
mkdir -p "$out"
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation -ffp-contract=off -Wno-unused-function -I /mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/include -I "$root" \
 "$src/player_markers_original_check.c" "$src/select_rings.c" "$src/camera_slots.c" "$src/name_tag_project.c" "$root/matrix_stack.c" "$root/draw_engine.c" \
 "$root/hud_draw.c" "$root/scenery_draw.c" "$root/uis_draw.c" "$root/ball_draw.c" "$root/gx_texobj.c" "$root/marks_draw.c" \
 -o "$out/player-markers-original-check.exe"
