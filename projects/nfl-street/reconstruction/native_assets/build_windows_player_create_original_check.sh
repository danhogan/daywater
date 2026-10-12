#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:?private output directory required}
case "$out" in /mnt/w/Daywater/*) ;; *) exit 2 ;; esac
mkdir -p "$out"
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
headers=${DAYWATER_MODERNGEKKO_HEADERS:-/mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/include}
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off \
 -I "$headers" "$root/player_create_original_check.c" "$root/player_startup.c" \
 "$root/player_appearance_init.c" "$root/player_constructor.c" "$root/actor_model_init.c" \
 "$root/actor_tail_init.c" "$root/actor_factory.c" -o "$out/player-create-original-check.exe"
