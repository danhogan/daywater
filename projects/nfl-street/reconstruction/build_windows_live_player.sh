#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
out=${1:-/mnt/w/Daywater/research-temp/nflstreet-live-player-20261008}
mkdir -p "$out"
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off \
 -DNFLSTREET_ACTOR_UPDATE_NO_MAIN -DNFLSTREET_PREPARATION_NO_MAIN \
 -DNFLSTREET_COMMAND_NO_MAIN -DNFLSTREET_KINEMATICS_NO_MAIN -DNFLSTREET_RESET_NO_MAIN \
 -DNFLSTREET_INPUT_TRANSLATION_NO_MAIN -DNFLSTREET_ACTOR_TARGET_INPUT_NO_MAIN \
 "$root/native_assets/live_player.c" "$root/native_assets/compact_pose.c" "$root/ball/ball_attachment.c" "$root/ball/ball_orientation.c" "$root/ball/ball_ownership.c" "$root/ball/ball_lifecycle.c" "$root/ball/ball_flight.c" "$root/ball/ball_frame.c" "$root/ball/ball_state.c" "$root/native_assets/pose.c" "$root/native_assets/player.c" \
 "$root/native_assets/field_render.c" "$root/native_assets/sky.c" "$root/native_assets/model.c" \
 "$root/native_assets/texture.c" "$root/native_assets/archive.c" "$root/native_assets/lzh1.c" \
 "$root/native_assets/live_contact.c" "$root/actor_target_input/animation_callbacks.c" \
 "$root/actor_target_input/contact_selection.c" "$root/actor_target_input/pair_requests.c" \
 "$root/native_assets/live_movement.c" "$root/native_assets/live_locomotion.c" \
 "$root/actor_target_input/locomotion_selection.c" "$root/actor_target_input/asset_loader.c" \
 "$root/actor_target_input/animation_channels.c" "$root/command_to_movement/actor_runtime.c" \
 "$root/command_to_movement/actor_update_replay.c" "$root/command_to_movement/preparation_replay.c" \
 "$root/command_to_movement/command_to_movement_replay.c" "$root/command_to_movement/reset_acceleration_replay.c" \
 "$root/input_translation/input_translation_replay.c" "$root/actor_target_input/actor_target_input.c" \
 "$root/actor_target_input/target_selection.c" "$root/actor_target_input/command_timing.c" "$root/actor_target_input/event_signals.c" "$root/kinematics/kinematics_replay.c" \
 -lm -lgdi32 -luser32 -o "$out/live-player.exe"
