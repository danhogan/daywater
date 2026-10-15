#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
r=$root/..
out=${1:?private output required}
case "$out" in /mnt/w/Daywater/*) ;; *) exit 2 ;; esac
mkdir -p "$out"
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation -ffp-contract=off -Wno-unused-function \
 -DNFLSTREET_ACTOR_UPDATE_NO_MAIN -DNFLSTREET_PREPARATION_NO_MAIN -DNFLSTREET_COMMAND_NO_MAIN -DNFLSTREET_KINEMATICS_NO_MAIN \
 -DNFLSTREET_RESET_NO_MAIN -DNFLSTREET_INPUT_TRANSLATION_NO_MAIN -DNFLSTREET_ACTOR_TARGET_INPUT_NO_MAIN \
 "$root/live_pose_demo.c" "$root/live_pose.c" "$root/actor_pose.c" "$root/actor_skeleton.c" "$root/compact_pose.c" \
 "$root/actor_palettes.c" "$root/gekko_quaternion/quaternion_kernels.c" "$root/gekko_quaternion/actor_slerp.c" "$root/actor_world.c" "$root/cluster_palette.c" "$root/model_view.c" "$root/matrix_stack.c" "$root/draw_engine.c" \
 "$root/live_locomotion.c" "$root/pose.c" "$root/live_movement.c" \
 "$r/actor_target_input/locomotion_selection.c" "$r/actor_target_input/asset_loader.c" "$r/actor_target_input/animation_channels.c" \
 "$r/actor_target_input/command_timing.c" \
 "$r/command_to_movement/actor_runtime.c" "$r/command_to_movement/actor_update_replay.c" "$r/command_to_movement/preparation_replay.c" \
 "$r/command_to_movement/command_to_movement_replay.c" "$r/command_to_movement/reset_acceleration_replay.c" \
 "$r/input_translation/input_translation_replay.c" "$r/actor_target_input/actor_target_input.c" "$r/actor_target_input/target_selection.c" \
 "$r/actor_target_input/event_signals.c" "$r/kinematics/kinematics_replay.c" \
 -lm -o "$out/live-pose-demo.exe"
