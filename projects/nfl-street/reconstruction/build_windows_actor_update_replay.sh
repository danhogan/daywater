#!/usr/bin/env bash
set -euo pipefail

source_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
zig_binary=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
output_path=${1:-/mnt/w/Daywater/research-temp/nflstreet-actor-update-replay.exe}
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}

"$zig_binary" cc -target x86_64-windows-gnu -std=c11 -O2 \
    -Wall -Wextra -Werror -ffp-contract=off \
    -DNFLSTREET_PREPARATION_NO_MAIN -DNFLSTREET_COMMAND_NO_MAIN \
    -DNFLSTREET_KINEMATICS_NO_MAIN \
    "$source_root/command_to_movement/actor_update_replay.c" \
    "$source_root/command_to_movement/preparation_replay.c" \
    "$source_root/command_to_movement/command_to_movement_replay.c" \
    "$source_root/kinematics/kinematics_replay.c" -lm -o "$output_path"

printf '%s\n' "$output_path"
