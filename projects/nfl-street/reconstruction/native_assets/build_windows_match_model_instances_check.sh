#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:?private output required}
case "$out" in /mnt/w/Daywater/*) ;; *) exit 2 ;; esac
mkdir -p "$out"
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation -ffp-contract=off \
 "$root/match_model_instances_check.c" "$root/match_model_instances.c" "$root/model_instance.c" "$root/match_attachment_palettes.c" "$root/attachment_palette_scale.c" "$root/match_body_palettes.c" "$root/cluster_palette.c" "$root/match_bodies.c" "$root/body_postattach.c" "$root/match_skeletons.c" "$root/match_visuals.c" "$root/quick_match_session.c" "$root/quick_match_roster.c" "$root/../menus/quick_match_selection.c" "$root/selected_player_visual.c" "$root/player_surface.c" "$root/player_record.c" "$root/game_roster.c" "$root/slot_textures.c" "$root/slot_materials.c" "$root/palette_recolor.c" "$root/gear_texture.c" "$root/selected_appearance_assets.c" "$root/face_texture.c" "$root/texture_apply.c" "$root/texture.c" "$root/selected_player_resources.c" "$root/hair_resources.c" "$root/hair_postattach.c" "$root/headwear_state.c" "$root/selected_player_geometry.c" "$root/face_resource_pair.c" "$root/face_postattach.c" "$root/model_postattach.c" "$root/skeleton_resource.c" "$root/skeleton_startup.c" \
 "$root/body_resource_pool.c" "$root/model_view.c" "$root/model_relocate.c" "$root/model_chunk_relocate.c" "$root/archive.c" "$root/lzh1.c" \
 "$root/../menus/team_player_body_binding.c" "$root/../menus/team_player_resolve.c" "$root/../menus/team_player_manifest.c" \
 "$root/../menus/team_catalog.c" "$root/../menus/team_identity.c" -o "$out/match-model-instances-check.exe"
