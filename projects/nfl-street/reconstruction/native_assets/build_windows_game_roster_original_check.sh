#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:?private output required}
case "$out" in /mnt/w/Daywater/*) ;; *) exit 2 ;; esac
mkdir -p "$out"
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation \
 -I /mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/include "$root/game_roster_original_check.c" "$root/actor_model_init.c" "$root/player_record.c" "$root/game_roster.c" \
 "$root/gear_texture.c" "$root/headwear_state.c" "$root/archive.c" "$root/lzh1.c" "$root/../menus/team_catalog.c" \
 -o "$out/game-roster-original-check.exe"
