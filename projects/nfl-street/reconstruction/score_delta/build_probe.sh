#!/usr/bin/env bash
set -euo pipefail

source_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
reconstruction_root=$(cd -- "$source_root/.." && pwd)
repo_root=$(cd -- "$reconstruction_root/../../.." && pwd)
dol_path=${1:-"$repo_root/cache/extracted/813c4511ba3135059a13de424555647e8252254b/boot.dol"}
output_dir=${2:-/mnt/w/Daywater/research-temp/nflstreet-score-delta-probe}
gcc_cc1=${GCC_CC1:-/mnt/w/Daywater/tools/gcc-powerpc64-fc43/usr/libexec/gcc/powerpc64-linux-gnu/15/cc1}
zig_binary=${ZIG_BINARY:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}

python "$reconstruction_root/verify_score_targets.py" "$dol_path" delta
mkdir -p -- "$output_dir"

"$gcc_cc1" -quiet -m32 -mcpu=750 -mmultiple -meabi -mno-always-hint \
    -ffreestanding -nostdinc -O2 -fomit-frame-pointer \
    -fno-optimize-sibling-calls -fno-reorder-blocks \
    -fno-reorder-blocks-and-partition -fno-schedule-insns \
    -fno-schedule-insns2 -fno-asynchronous-unwind-tables \
    -fno-unwind-tables "$source_root/score_delta_gcc_probe.c" \
    -o "$output_dir/score_delta_gcc_probe.s"

"$zig_binary" cc -target powerpc-freestanding -mcpu=750 -c \
    "$output_dir/score_delta_gcc_probe.s" \
    -o "$output_dir/score_delta_gcc_probe.o"

"$zig_binary" cc -target powerpc-freestanding -mcpu=750 -nostdlib \
    -Wl,-e,add_score_for_side -Wl,-T,"$source_root/score_delta_probe.ld" \
    "$output_dir/score_delta_gcc_probe.o" \
    -o "$output_dir/score_delta_gcc_probe.elf"

if readelf -r "$output_dir/score_delta_gcc_probe.elf" | rg -q 'Relocation section'; then
    echo 'The linked candidate still has relocations' >&2
    exit 1
fi
if ! readelf -SW "$output_dir/score_delta_gcc_probe.elf" | rg -q '\.text[[:space:]]+PROGBITS[[:space:]]+8002676c'; then
    echo 'The linked candidate .text has the wrong address' >&2
    exit 1
fi
if ! readelf -h "$output_dir/score_delta_gcc_probe.elf" | rg -q 'Entry point address:[[:space:]]+0x8002676c'; then
    echo 'The linked candidate has the wrong entry point' >&2
    exit 1
fi

"$zig_binary" objcopy --only-section=.text -O binary \
    "$output_dir/score_delta_gcc_probe.elf" \
    "$output_dir/score_delta_gcc_probe.bin"

python - "$output_dir/score_delta_gcc_probe.bin" <<'PY'
from pathlib import Path
import sys

candidate = Path(sys.argv[1]).read_bytes()
print(f"Linked GCC probe: {len(candidate)} bytes; DOL target: 124 bytes")
if len(candidate) == 124:
    print("Length matches; run verify_score_targets.py with --candidate-raw to compare bytes")
else:
    print("No byte match: candidate length differs from the DOL target")
PY
