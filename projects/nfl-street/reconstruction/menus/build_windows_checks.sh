#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:-/mnt/w/Daywater/research-temp/nflstreet-original-menus-20261009}
mkdir -p "$out"
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
headers=${DAYWATER_MODERNGEKKO_HEADERS:-/mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/include}
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/font_original_check.c" "$root/font.c" "$root/../native_assets/texture.c" -o "$out/font-original-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/screen_original_check.c" "$root/screen.c" -o "$out/screen-original-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/text_original_check.c" "$root/text.c" "$root/font.c" "$root/../native_assets/texture.c" -o "$out/text-original-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off \
 "$root/background.c" "$root/title_preview.c" "$root/broadcast.c" "$root/title.c" "$root/rate.c" "$root/script.c" "$root/text.c" "$root/screen.c" "$root/font.c" "$root/../native_assets/texture.c" -luser32 -lgdi32 -o "$out/title-preview.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/script_original_check.c" "$root/script.c" -o "$out/script-original-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/focus_original_check.c" "$root/focus.c" "$root/script.c" "$root/screen.c" -o "$out/focus-original-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/rate_original_check.c" "$root/rate.c" -o "$out/rate-original-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/title_original_check.c" "$root/title.c" "$root/script.c" "$root/screen.c" "$root/rate.c" -o "$out/title-original-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/background.c" "$root/title_background_check.c" "$root/broadcast.c" "$root/title.c" "$root/script.c" "$root/screen.c" "$root/rate.c" -o "$out/title-background-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/main_focus_check.c" "$root/rate.c" "$root/main_menu.c" "$root/focus.c" "$root/script.c" "$root/screen.c" "$root/text.c" "$root/font.c" "$root/../native_assets/texture.c" -o "$out/main-focus-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off \
 "$root/main_preview.c" "$root/rate.c" "$root/main_menu.c" "$root/focus.c" "$root/script.c" "$root/screen.c" "$root/text.c" "$root/font.c" "$root/../native_assets/texture.c" -luser32 -lgdi32 -o "$out/main-preview.exe"

"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/main_transition_check.c" "$root/rate.c" "$root/main_menu.c" "$root/focus.c" "$root/script.c" "$root/screen.c" "$root/text.c" "$root/font.c" "$root/../native_assets/texture.c" -o "$out/main-transition-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/activation_original_check.c" "$root/activation.c" -o "$out/activation-original-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/link_original_check.c" "$root/link.c" "$root/activation.c" "$root/script.c" -o "$out/link-original-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/dispatcher_original_check.c" "$root/dispatcher.c" "$root/script.c" "$root/screen.c" -o "$out/dispatcher-original-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/construct_original_check.c" "$root/construct.c" "$root/screen.c" -o "$out/construct-original-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/text_construct_original_check.c" "$root/image_construct.c" "$root/text_construct.c" "$root/construct.c" "$root/screen.c" "$root/text.c" "$root/font.c" "$root/../native_assets/texture.c" -o "$out/text-construct-original-check.exe"

"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/options_original_check.c" "$root/options.c" -o "$out/options-original-check.exe"

"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/options_screen_original_check.c" "$root/options_screen.c" "$root/coroutine.c" "$root/events.c" "$root/options.c" "$root/main_menu.c" "$root/focus.c" "$root/rate.c" "$root/script.c" "$root/screen.c" "$root/text.c" "$root/font.c" "$root/../native_assets/texture.c" -o "$out/options-screen-original-check.exe"

"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/options_tick_original_check.c" "$root/options_screen.c" "$root/coroutine.c" "$root/events.c" "$root/options.c" "$root/main_menu.c" "$root/focus.c" "$root/rate.c" "$root/script.c" "$root/screen.c" "$root/text.c" "$root/font.c" "$root/../native_assets/texture.c" -o "$out/options-tick-original-check.exe"

for check in options_events options_ready options_navigation options_continuation; do
 "$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
  "$root/${check}_original_check.c" "$root/profile_store.c" "$root/options_screen.c" "$root/coroutine.c" "$root/events.c" "$root/options.c" "$root/main_menu.c" "$root/focus.c" "$root/rate.c" "$root/script.c" "$root/screen.c" "$root/text.c" "$root/font.c" "$root/../native_assets/texture.c" -o "$out/${check//_/-}-original-check.exe"
done
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off \
 "$root/options_preview.c" "$root/profile_store.c" "$root/profile_store_windows.c" "$root/options_screen.c" "$root/coroutine.c" "$root/events.c" "$root/construct.c" "$root/text_construct.c" "$root/image_construct.c" "$root/options.c" "$root/main_menu.c" "$root/focus.c" "$root/rate.c" "$root/script.c" "$root/screen.c" "$root/text.c" "$root/font.c" "$root/../native_assets/texture.c" -luser32 -lgdi32 -o "$out/options-preview.exe"

"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/coroutine_original_check.c" "$root/coroutine.c" "$root/script.c" -o "$out/coroutine-original-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/storage_dialog_original_check.c" "$root/storage_dialog.c" -o "$out/storage-dialog-original-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/storage_dialog_poll_original_check.c" "$root/storage_dialog.c" -o "$out/storage-dialog-poll-original-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/profile_defaults_original_check.c" "$root/profile_store.c" -o "$out/profile-defaults-original-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off \
 "$root/profile_store_check.c" "$root/profile_store.c" "$root/profile_store_windows.c" -o "$out/profile-store-check.exe"

"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/text_lines_original_check.c" "$root/text_lines.c" "$root/font.c" "$root/../native_assets/texture.c" -o "$out/text-lines-original-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/card_screen_original_check.c" "$root/card_screen.c" "$root/storage_dialog.c" "$root/options_screen.c" "$root/options.c" "$root/coroutine.c" "$root/events.c" "$root/construct.c" "$root/text_construct.c" "$root/image_construct.c" "$root/main_menu.c" "$root/focus.c" "$root/rate.c" "$root/script.c" "$root/screen.c" "$root/text.c" "$root/font.c" "$root/../native_assets/texture.c" -o "$out/card-screen-original-check.exe"

prompt_sources=(card_screen.c card_prompt.c text_lines.c storage_dialog.c options_screen.c options.c coroutine.c events.c construct.c text_construct.c image_construct.c main_menu.c focus.c rate.c script.c screen.c text.c font.c ../native_assets/texture.c)
prompt_paths=()
for source in "${prompt_sources[@]}"; do prompt_paths+=("$root/$source"); done
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/card_prompt_original_check.c" "${prompt_paths[@]}" -o "$out/card-prompt-original-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off \
 "$root/card_prompt_preview.c" "${prompt_paths[@]}" -luser32 -lgdi32 -o "$out/card-prompt-preview.exe"
