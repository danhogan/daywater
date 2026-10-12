#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:-/mnt/w/Daywater/research-temp/nflstreet-ball-frame-20261010}
mkdir -p "$out"
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
headers=${DAYWATER_MODERNGEKKO_HEADERS:-/mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/include}
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
for mode in attached airborne; do
 "$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/native_assets/ball_frame_${mode}_original_check.c" "$root/ball/ball_frame.c" "$root/ball/ball_flight.c" "$root/ball/ball_state.c" "$root/ball/ball_orientation.c" "$root/ball/ball_attachment.c" -lm -o "$out/ball-frame-$mode-original-check.exe"
done
