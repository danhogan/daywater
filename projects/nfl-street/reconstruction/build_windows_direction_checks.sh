#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
out=${1:-/mnt/w/Daywater/research-temp/nflstreet-directions-20261009}
headers=${DAYWATER_MODERNGEKKO_HEADERS:-/mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/include}
mkdir -p "$out"
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
# Includes full movement original-stage comparison and native lifecycle check.
bash "$root/build_windows_braking_checks.sh" "$out"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror \
 -ffp-contract=off -I "$headers" "$root/native_assets/stream_original_check.c" \
 "$root/actor_target_input/command_timing.c" "$root/actor_target_input/event_signals.c" \
 "$root/actor_target_input/target_selection.c" -lm -o "$out/stream-original-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror \
 -ffp-contract=off -I "$headers" "$root/native_assets/ball_original_check.c" \
 "$root/ball/ball_state.c" -lm -o "$out/ball-original-check.exe"
