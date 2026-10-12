#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:-/mnt/w/Daywater/research-temp/nflstreet-quick-game-20261010}
case "$out" in /mnt/w/Daywater/*) ;; *) echo 'Private output must be beneath /mnt/w/Daywater' >&2; exit 2 ;; esac
mkdir -p "$out"
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
headers=${DAYWATER_MODERNGEKKO_HEADERS:-/mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/include}
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
flags=(-target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers")
"$zig" cc "${flags[@]}" "$root/quick_setup_original_check.c" "$root/dispatcher.c" "$root/script.c" "$root/screen.c" -o "$out/quick-setup-original-check.exe"
"$zig" cc "${flags[@]}" "$root/quick_main_original_check.c" "$root/rate.c" "$root/main_menu.c" "$root/focus.c" "$root/script.c" "$root/screen.c" "$root/text.c" "$root/font.c" "$root/../native_assets/texture.c" -o "$out/quick-main-original-check.exe"
"$zig" cc "${flags[@]}" "$root/quick_route_check.c" "$root/order.c" "$root/dispatcher.c" "$root/rate.c" "$root/main_menu.c" "$root/focus.c" "$root/script.c" "$root/screen.c" "$root/text.c" "$root/font.c" "$root/../native_assets/texture.c" -o "$out/quick-route-check.exe"
"$zig" cc "${flags[@]}" "$root/quick_outer_original_check.c" "$root/dispatcher.c" "$root/script.c" "$root/screen.c" -o "$out/quick-outer-original-check.exe"
"$zig" cc "${flags[@]}" "$root/order_original_check.c" "$root/order.c" "$root/dispatcher.c" "$root/script.c" "$root/screen.c" -o "$out/order-original-check.exe"
"$zig" cc "${flags[@]}" "$root/controller_select_original_check.c" "$root/controller_select.c" "$root/controller_callback.c" "$root/controller_devices.c" -o "$out/controller-select-original-check.exe"
"$zig" cc "${flags[@]}" "$root/controller_devices_original_check.c" "$root/controller_select.c" "$root/controller_callback.c" "$root/controller_devices.c" -o "$out/controller-devices-original-check.exe"
