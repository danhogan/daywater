#!/usr/bin/env bash
set -euo pipefail

source_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(cd -- "$source_root/../../.." && pwd)
dol_path=${1:-"$repo_root/cache/extracted/813c4511ba3135059a13de424555647e8252254b/boot.dol"}
output_dir=${2:-/mnt/w/Daywater/research-temp/nflstreet-clear-match}
zig_binary=${ZIG_BINARY:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}

mkdir -p -- "$output_dir"
"$zig_binary" cc -target powerpc-freestanding -mcpu=750 -std=c89 \
    -O2 -g0 -fomit-frame-pointer -c \
    "$source_root/score_writer/clear_unknown_40_clang_match.c" \
    -o "$output_dir/clear_unknown_40_clang_match.o"

relocations=$(readelf -r "$output_dir/clear_unknown_40_clang_match.o")
if rg -q '\.rela?\.text' <<< "$relocations"; then
    echo 'The candidate .text has unresolved relocations' >&2
    exit 1
fi

"$zig_binary" objcopy --only-section=.text -O binary \
    "$output_dir/clear_unknown_40_clang_match.o" \
    "$output_dir/clear_unknown_40_clang_match.bin"

python "$source_root/verify_score_targets.py" "$dol_path" clear \
    --candidate-raw "$output_dir/clear_unknown_40_clang_match.bin"
