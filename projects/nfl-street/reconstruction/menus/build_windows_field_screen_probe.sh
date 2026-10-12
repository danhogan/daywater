#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:-/mnt/w/Daywater/research-temp/nflstreet-field-screen-probe-20261011}
case "$out" in /mnt/w/Daywater/*) ;; *) echo 'Private output must be beneath /mnt/w/Daywater' >&2; exit 2 ;; esac
mkdir -p "$out"
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
headers=${DAYWATER_MODERNGEKKO_HEADERS:-/mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/include}
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off \
  -I "$headers" -I "$root" -I "$root/../native_assets" \
  -Dmain=nflstreet_field_screen_preview_standalone_main \
  -c "$root/field_screen_preview.c" -o "$out/field_screen_preview.o"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off \
  -I "$headers" -I "$root" -I "$root/../native_assets" \
  "$root/field_screen_probe.c" "$root/field_catalog.c" "$root/field_bootstrap.c" \
  "$root/field_availability.c" "$root/field_stat_text.c" "$root/field_cache.c" \
  "$root/team_catalog.c" "$root/screen.c" "$root/construct.c" "$root/events.c" \
  "$root/script.c" "$root/text_construct.c" "$root/text.c" "$root/text_lines.c" \
  "$root/font.c" "$root/image_construct.c" "$root/main_menu.c" "$root/team_classes.c" "$root/team_transition.c" "$root/rate.c" "$root/focus.c" \
  "$out/field_screen_preview.o" "$root/../native_assets/texture.c" \
  -lgdi32 -luser32 -o "$out/field-screen-probe.exe"
echo "Built $out/field-screen-probe.exe"
