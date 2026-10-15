#!/usr/bin/env bash
set -euo pipefail
here=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:-/tmp/nflstreet-check-80188bb0}
mem=${2:-/mnt/w/Daywater/research-temp/position-neutral-14.mem1}
module=${3:-/mnt/w/Daywater/research-temp/nflstreet-moderngekko-modules/GNNE69/a065ae2f4c192124266a56171aa5eed4e2dccb586d32670c5dab2e2ed3ad980c-c4dcfb815f5c2267/gGNNE69_recomp.so}
include=/mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/include
cc -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$include" \
  "$here/check_80188bb0_module.c" "$here/quaternion_kernels.c" -ldl -lm -o "$out"
"$out" "$mem" "$module"
