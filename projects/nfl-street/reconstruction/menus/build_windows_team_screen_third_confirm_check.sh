#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:-/mnt/w/Daywater/research-temp/nflstreet-team-third-confirm-20261011}
case "$out" in /mnt/w/Daywater/*) ;; *) exit 2 ;; esac
mkdir -p "$out"
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
headers=${DAYWATER_MODERNGEKKO_HEADERS:-/mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/include}
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
sources=(team_confirm team_transition team_devices team_player_spawn team_player_labels_callback team_player_remove team_player_labels team_identity team_weights team_bootstrap_weights team_bootstrap_static team_bootstrap team_formation team_lineup team_screen team_player_window team_player_window_callback team_activation team_preview team_actor team_format team_callback team_classes team_start team_catalog team_selection team_carousel options_screen options coroutine events construct text_construct image_construct main_menu dispatcher focus rate script screen text text_lines font)
paths=();for source in "${sources[@]}";do paths+=("$root/$source.c");done
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off \
  -I "$headers" -I "$root" "$root/team_screen_third_confirm_original_check.c" "${paths[@]}" \
  "$root/../native_assets/texture.c" -o "$out/team-screen-third-confirm-original-check.exe"
