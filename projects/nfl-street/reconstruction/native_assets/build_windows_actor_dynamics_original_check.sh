#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:?private output required}
case "$out" in /mnt/w/Daywater/research-temp/nflstreet-actor-dynamics-20261014|/mnt/w/Daywater/research-temp/nflstreet-actor-dynamics-20261014/*) ;; *) exit 2 ;; esac
source_file=${2:-$root/actor_dynamics.c}
mkdir -p "$out"
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
kinds=${3:-hand head all}
for kind in $kinds; do
  case "$kind" in
    all) check=actor_dynamics_original_check.c; name=actor-dynamics-original-check.exe ;;
    *) check=actor_dynamics_${kind}_original_check.c; name=actor-dynamics-${kind}-original-check.exe ;;
  esac
  "$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation \
    -ffp-contract=off -Wno-unused-function -I "$root" \
    -I /mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/include \
    "$root/$check" "$source_file" "$root/actor_dynamics_math.c" "$root/matrix_stack.c" "$root/draw_engine.c" \
    -lm -o "$out/$name"
done
