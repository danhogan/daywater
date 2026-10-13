#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:-/mnt/w/Daywater/research-temp/nflstreet-team-field-picker-join-20261011}
case "$out" in /mnt/w/Daywater/*) ;; *) echo 'Private output must be beneath /mnt/w/Daywater' >&2; exit 2 ;; esac
mkdir -p "$out"
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
headers=${DAYWATER_MODERNGEKKO_HEADERS:-/mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/include}
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
flags=(-target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" -I "$root" -I "$root/../native_assets")
"$zig" cc "${flags[@]}" -Dmain=nflstreet_field_preview_standalone_main -c \
  "$root/field_screen_preview.c" -o "$out/field_screen_preview.o"
"$zig" cc "${flags[@]}" -Dmain=nflstreet_field_source_probe_main -c \
  "$root/field_screen_probe.c" -o "$out/field_screen_runtime.o"
sources=(team_confirm team_transition team_devices team_player_spawn team_player_labels_callback team_player_remove team_player_labels team_identity team_weights team_bootstrap_weights team_bootstrap_static team_bootstrap team_formation team_lineup team_screen team_player_window team_player_window_callback team_activation team_preview team_actor team_format team_callback team_classes team_start team_catalog team_selection team_carousel options_screen options coroutine events construct text_construct text text_lines font image_construct main_menu dispatcher focus rate script screen field_catalog field_bootstrap field_availability field_stat_text field_cache quick_match_selection)
paths=()
for source in "${sources[@]}"; do
  if [[ "$source" == script ]]; then paths+=("${DAYWATER_UI_SCRIPT_SOURCE:-$root/script.c}")
  else paths+=("$root/$source.c"); fi
done
"$zig" cc "${flags[@]}" "$root/team_field_picker_join_probe.c" \
  "${paths[@]}" "$out/field_screen_runtime.o" "$out/field_screen_preview.o" \
  "$root/../native_assets/texture.c" "$root/../native_assets/quick_match_session.c" \
  "$root/../native_assets/quick_match_roster.c" "$root/../native_assets/game_roster.c" -lgdi32 -luser32 \
  -o "$out/team-field-picker-join.exe"
echo "Built $out/team-field-picker-join.exe"
