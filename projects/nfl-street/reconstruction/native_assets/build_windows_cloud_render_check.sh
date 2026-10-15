#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:?private output required}
case "$out" in /mnt/w/Daywater/*) ;; *) exit 2 ;; esac
mkdir -p "$out"
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation \
 "$root/cloud_render_check.c" "$root/cloud_draw.c" "$root/crowd_draw.c" "$root/player_draw.c" "$root/scenery_draw.c" "$root/marks_draw.c" "$root/hud_draw.c" "$root/matrix_stack.c" "$root/uis_draw.c" "$root/ball_draw.c" "$root/scene_frame.c" "$root/light_rig.c" "$root/camera_slots.c" "$root/name_tag_project.c" "$root/camera_projection.c" "$root/effects_draw.c" "$root/actor_scale.c" "$root/actor_palettes.c" "$root/cluster_palette.c" "$root/model_view.c" "$root/gx_texobj.c" "$root/draw_engine.c" "$root/texture.c" -o "$out/cloud-render-check.exe"
