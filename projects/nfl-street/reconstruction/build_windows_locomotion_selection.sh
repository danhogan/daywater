#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
out=${1:-/mnt/w/Daywater/research-temp/nflstreet-locomotion-selection-20261009}
headers=${DAYWATER_MODERNGEKKO_HEADERS:-/mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/include}
mkdir -p "$out"
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror \
 -ffp-contract=off -I "$headers" "$root/native_assets/selection_original_check.c" \
 "$root/actor_target_input/locomotion_selection.c" "$root/actor_target_input/asset_loader.c" \
 "$root/actor_target_input/command_timing.c" "$root/actor_target_input/event_signals.c" \
 "$root/actor_target_input/target_selection.c" -lm -o "$out/selection-original-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror \
 -ffp-contract=off -I "$headers" "$root/native_assets/pose_original_check.c" \
 "$root/native_assets/pose.c" -lm -o "$out/pose-original-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror \
 -ffp-contract=off -I "$headers" "$root/native_assets/locomotion_playback_original_check.c" \
 "$root/native_assets/live_locomotion.c" "$root/native_assets/pose.c" \
 "$root/actor_target_input/locomotion_selection.c" "$root/actor_target_input/asset_loader.c" \
 "$root/actor_target_input/command_timing.c" "$root/actor_target_input/event_signals.c" \
 "$root/actor_target_input/target_selection.c" "$root/actor_target_input/animation_channels.c" -lm -o "$out/playback-original-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror \
 -ffp-contract=off -I "$headers" "$root/native_assets/locomotion_fade_original_check.c" \
 "$root/native_assets/live_locomotion.c" "$root/native_assets/pose.c" \
 "$root/actor_target_input/locomotion_selection.c" "$root/actor_target_input/asset_loader.c" \
 "$root/actor_target_input/command_timing.c" "$root/actor_target_input/event_signals.c" \
 "$root/actor_target_input/target_selection.c" "$root/actor_target_input/animation_channels.c" -lm -o "$out/fade-original-check.exe"

"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror \
 -ffp-contract=off -I "$headers" "$root/native_assets/curve_choice_original_check.c" \
 "$root/actor_target_input/locomotion_selection.c" "$root/actor_target_input/asset_loader.c" \
 "$root/actor_target_input/command_timing.c" "$root/actor_target_input/event_signals.c" \
 "$root/actor_target_input/target_selection.c" -lm -o "$out/curve-choice-original-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror \
 -ffp-contract=off -I "$headers" "$root/native_assets/curve_playback_original_check.c" \
 "$root/native_assets/live_locomotion.c" "$root/native_assets/pose.c" \
 "$root/actor_target_input/locomotion_selection.c" "$root/actor_target_input/asset_loader.c" \
 "$root/actor_target_input/command_timing.c" "$root/actor_target_input/event_signals.c" \
 "$root/actor_target_input/target_selection.c" "$root/actor_target_input/animation_channels.c" -lm -o "$out/curve-playback-original-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror \
 -ffp-contract=off -I "$headers" "$root/native_assets/directional_playback_original_check.c" \
 "$root/native_assets/live_locomotion.c" "$root/native_assets/pose.c" \
 "$root/actor_target_input/locomotion_selection.c" "$root/actor_target_input/asset_loader.c" \
 "$root/actor_target_input/command_timing.c" "$root/actor_target_input/event_signals.c" \
 "$root/actor_target_input/target_selection.c" "$root/actor_target_input/animation_channels.c" -lm -o "$out/directional-playback-original-check.exe"

"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror \
 -ffp-contract=off -I "$headers" "$root/native_assets/variant_original_check.c" \
 "$root/actor_target_input/variant_selection.c" "$root/actor_target_input/locomotion_selection.c" \
 "$root/actor_target_input/asset_loader.c" "$root/actor_target_input/command_timing.c" \
 "$root/actor_target_input/event_signals.c" "$root/actor_target_input/target_selection.c" \
 -lm -o "$out/variant-original-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror \
 -ffp-contract=off -I "$headers" "$root/native_assets/variant_second_original_check.c" \
 "$root/actor_target_input/variant_selection.c" "$root/actor_target_input/locomotion_selection.c" \
 "$root/actor_target_input/asset_loader.c" "$root/actor_target_input/command_timing.c" \
 "$root/actor_target_input/event_signals.c" "$root/actor_target_input/target_selection.c" \
 -lm -o "$out/variant-second-original-check.exe"

"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror \
 -ffp-contract=off -I "$headers" "$root/native_assets/set_catalog_export.c" \
 "$root/actor_target_input/variant_selection.c" "$root/actor_target_input/locomotion_selection.c" \
 "$root/actor_target_input/asset_loader.c" "$root/actor_target_input/command_timing.c" \
 "$root/actor_target_input/event_signals.c" "$root/actor_target_input/target_selection.c" \
 -lm -o "$out/set-catalog-export.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror \
 -ffp-contract=off -I "$headers" "$root/native_assets/set_playback_original_check.c" \
 "$root/native_assets/live_locomotion.c" "$root/native_assets/pose.c" \
 "$root/actor_target_input/locomotion_selection.c" "$root/actor_target_input/asset_loader.c" \
 "$root/actor_target_input/command_timing.c" "$root/actor_target_input/event_signals.c" \
 "$root/actor_target_input/target_selection.c" "$root/actor_target_input/animation_channels.c" -lm -o "$out/set-playback-original-check.exe"
