#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:?private output required}
case "$out" in /mnt/w/Daywater/*) ;; *) exit 2 ;; esac
mkdir -p "$out"
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
"$zig" cc -target x86_64-windows-gnu -std=c11 -O1 -Wall -Wextra -Werror -Wno-misleading-indentation -Wno-unused-function -Wno-unused-label -ffp-contract=off ${CB_CFLAGS:-} \
 -I /mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/include \
 "$root/channel_callbacks_original_check.c" "$root/channel_callbacks.c" "$root/actor_channels.c" "$root/anim_stream.c" "$root/actor_pose.c" "$root/matrix_stack.c" "$root/draw_engine.c" \
 -o "$out/channel-callbacks-original-check.exe"
