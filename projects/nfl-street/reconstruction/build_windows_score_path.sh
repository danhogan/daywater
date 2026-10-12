#!/usr/bin/env bash
set -euo pipefail

source_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
zig_binary=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
output_path=${1:-/mnt/w/Daywater/research-temp/nflstreet-score-path-check.exe}
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}

"$zig_binary" cc -target x86_64-windows-gnu -std=c89 -O2 \
    "$source_root/score_reader/score_reader.c" \
    "$source_root/score_writer/score_writer.c" \
    "$source_root/score_writer/clear_unknown_40.c" \
    "$source_root/score_delta/score_delta.c" \
    "$source_root/score_record/score_record.c" \
    "$source_root/score_table/score_table.c" \
    "$source_root/../native/fnig_store.c" \
    "$source_root/tests/test_score_delta.c" \
    -o "$output_path"

printf '%s\n' "$output_path"
