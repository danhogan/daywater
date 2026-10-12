#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
out=${1:-/mnt/w/Daywater/research-temp/nflstreet-stream-setup-replay.exe}
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror \
 "$root/actor_target_input/stream_setup_replay.c" "$root/actor_target_input/command_timing.c" \
 "$root/actor_target_input/pair_requests.c" \
 "$root/actor_target_input/event_signals.c" "$root/actor_target_input/target_selection.c" -lm -o "$out"
printf '%s\n' "$out"
