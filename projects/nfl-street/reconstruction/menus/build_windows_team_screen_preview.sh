#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:-/mnt/w/Daywater/research-temp/nflstreet-team-screen-preview-20261011}
case "$out" in /mnt/w/Daywater/*) ;; *) echo 'Private output must be beneath /mnt/w/Daywater' >&2; exit 2 ;; esac
mkdir -p "$out"
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
headers=${DAYWATER_MODERNGEKKO_HEADERS:-/mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/include}
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
sources=(team_player_body_binding team_player_selected_pair team_effects team_player_resolve team_player_manifest team_player_selected_head team_confirm team_transition team_devices team_player_spawn team_player_labels_callback team_player_remove team_player_labels team_identity team_weights team_bootstrap_weights team_bootstrap_static team_bootstrap team_formation team_lineup team_screen team_player_window team_player_window_callback team_activation team_preview team_actor team_format team_callback team_classes team_start team_catalog team_selection team_carousel options_screen options coroutine events construct text_construct image_construct main_menu dispatcher focus rate script screen text text_lines font)
paths=()
for source in "${sources[@]}"; do
  if [[ "$source" == script ]]; then paths+=("${DAYWATER_UI_SCRIPT_SOURCE:-$root/script.c}")
  else paths+=("$root/$source.c"); fi
done
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off \
  -I "$root" -I "$root/../native_assets" -I "$headers" \
  "$root/team_screen_preview.c" "${paths[@]}" "$root/../native_assets/texture.c" "$root/../native_assets/model.c" "$root/../native_assets/mesh_render.c" "$root/../native_assets/archive.c" "$root/../native_assets/lzh1.c" \
  -o "$out/team-screen-preview.exe" -lgdi32 -luser32
echo "Built $out/team-screen-preview.exe"
