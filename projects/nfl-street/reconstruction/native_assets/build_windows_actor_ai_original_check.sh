#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:?private output required}
case "$out" in /mnt/w/Daywater/research-temp/nflstreet-ai-20261015|/mnt/w/Daywater/research-temp/nflstreet-ai-20261015/*) ;; *) exit 2 ;; esac
mkdir -p "$out"
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation -ffp-contract=off \
 -I /mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/include \
 "$root/actor_ai_original_check.c" "$root/actor_ai.c" "$root/gekko_quaternion/quaternion_kernels.c" \
 -o "$out/actor-ai-original-check.exe"
