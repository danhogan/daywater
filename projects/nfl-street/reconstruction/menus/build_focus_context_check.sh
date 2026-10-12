#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
OUT="${1:-/tmp/nflstreet-focus-context-check}"
cc -std=c11 -Wall -Wextra -Werror -Wno-misleading-indentation -O2 \
  "$HERE/focus_context_check.c" "$HERE/focus.c" "$HERE/screen.c" "$HERE/script.c" \
  -lm -o "$OUT"
"$OUT"
