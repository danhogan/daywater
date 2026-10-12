#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:-/mnt/w/Daywater/research-temp/nflstreet-controller-start-20261010}
case "$out" in /mnt/w/Daywater/*) ;; *) echo 'Private output must be beneath /mnt/w/Daywater' >&2; exit 2 ;; esac
mkdir -p "$out"
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
sources=(controller_screen controller_callback controller_devices controller_select options_screen options coroutine events construct text_construct image_construct main_menu dispatcher focus rate script screen text font)
paths=()
for source in "${sources[@]}"; do paths+=("$root/$source.c"); done
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off "$root/controller_start_probe.c" "${paths[@]}" "$root/../native_assets/texture.c" -o "$out/controller-start-probe.exe"
headers=${DAYWATER_MODERNGEKKO_HEADERS:-/mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/include}
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" "$root/controller_start_original_check.c" "${paths[@]}" "$root/../native_assets/texture.c" -o "$out/controller-start-original-check.exe"

"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off "$root/controller_preview.c" "${paths[@]}" "$root/../native_assets/texture.c" -luser32 -lgdi32 -o "$out/controller-preview.exe"
