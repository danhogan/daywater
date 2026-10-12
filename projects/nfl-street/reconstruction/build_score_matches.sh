#!/usr/bin/env bash
set -euo pipefail

source_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(cd -- "$source_root/../../.." && pwd)
dol_path=${1:-"$repo_root/cache/extracted/813c4511ba3135059a13de424555647e8252254b/boot.dol"}
output_dir=${2:-/mnt/w/Daywater/research-temp/nflstreet-score-matches}
gcc_cc1=${GCC_CC1:-/mnt/w/Daywater/tools/gcc-powerpc64-fc43/usr/libexec/gcc/powerpc64-linux-gnu/15/cc1}
zig_binary=${ZIG_BINARY:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}

mkdir -p -- "$output_dir"
bash "$source_root/build_clear_match.sh" "$dol_path" "$output_dir"

"$gcc_cc1" -quiet -m32 -mcpu=750 -mno-always-hint \
    -ffreestanding -nostdinc -O2 -fomit-frame-pointer \
    "$source_root/score_reader/score_reader_gcc_match.c" \
    -o "$output_dir/score_reader_gcc_match.s"

"$zig_binary" cc -target powerpc-freestanding -mcpu=750 -c \
    "$output_dir/score_reader_gcc_match.s" \
    -o "$output_dir/score_reader_gcc_match.o"

relocations=$(readelf -r "$output_dir/score_reader_gcc_match.o")
if rg -q '\.rela?\.text' <<< "$relocations"; then
    echo 'The reader candidate .text has unresolved relocations' >&2
    exit 1
fi

"$zig_binary" objcopy --only-section=.text -O binary \
    "$output_dir/score_reader_gcc_match.o" \
    "$output_dir/score_reader_gcc_match.bin"

python "$source_root/verify_score_targets.py" "$dol_path" reader \
    --candidate-raw "$output_dir/score_reader_gcc_match.bin"

"$gcc_cc1" -quiet -m32 -mcpu=750 -mno-always-hint \
    -ffreestanding -nostdinc -O2 -fomit-frame-pointer \
    -fno-reorder-blocks -fno-reorder-blocks-and-partition \
    -fno-if-conversion -fno-schedule-insns -fno-schedule-insns2 \
    "$source_root/score_writer/score_writer_gcc_match.c" \
    -o "$output_dir/score_writer_gcc_match.s"

"$zig_binary" cc -target powerpc-freestanding -mcpu=750 -c \
    "$output_dir/score_writer_gcc_match.s" \
    -o "$output_dir/score_writer_gcc_match.o"

relocations=$(readelf -r "$output_dir/score_writer_gcc_match.o")
if rg -q '\.rela?\.text' <<< "$relocations"; then
    echo 'The writer candidate .text has unresolved relocations' >&2
    exit 1
fi

"$zig_binary" objcopy --only-section=.text -O binary \
    "$output_dir/score_writer_gcc_match.o" \
    "$output_dir/score_writer_gcc_match.bin"

python "$source_root/verify_score_targets.py" "$dol_path" writer \
    --candidate-raw "$output_dir/score_writer_gcc_match.bin"
