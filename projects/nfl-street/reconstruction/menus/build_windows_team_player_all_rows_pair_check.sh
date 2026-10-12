#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
out=${1:-/mnt/w/Daywater/research-temp/nflstreet-team-player-loader/all-rows}
case "$out" in /mnt/w/Daywater/*) ;; *) echo 'Private output must be beneath /mnt/w/Daywater' >&2; exit 2 ;; esac
mkdir -p "$out"
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off \
  "$root/menus/team_player_all_rows_pair_check.c" \
  "$root/menus/team_player_resolve.c" "$root/menus/team_player_manifest.c" \
  "$root/menus/team_identity.c" "$root/menus/team_catalog.c" \
  "$root/menus/team_player_body_binding.c" \
  "$root/menus/team_player_selected_pair.c" \
  "$root/menus/team_player_selected_head.c" \
  "$root/native_assets/model.c" "$root/native_assets/mesh_render.c" \
  "$root/native_assets/archive.c" "$root/native_assets/lzh1.c" \
  "$root/native_assets/texture.c" -lm \
  -o "$out/team-player-all-rows-pair-check.exe"
