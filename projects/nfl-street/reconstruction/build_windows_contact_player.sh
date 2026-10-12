#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:-/mnt/w/Daywater/research-temp/nflstreet-visible-contact-20261009}
zig=${DAYWATER_ZIG:-/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig}
headers=${DAYWATER_MODERNGEKKO_HEADERS:-/mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/include}
export ZIG_GLOBAL_CACHE_DIR=${ZIG_GLOBAL_CACHE_DIR:-/tmp/daywater-zig-cache}
bash "$root/build_windows_live_player.sh" "$out"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -DNFLSTREET_KINEMATICS_NO_MAIN \
 "$root/native_assets/live_contact_check.c" "$root/native_assets/live_contact.c" \
 "$root/actor_target_input/animation_callbacks.c" "$root/actor_target_input/animation_channels.c" "$root/actor_target_input/asset_loader.c" \
 "$root/actor_target_input/contact_selection.c" "$root/actor_target_input/pair_requests.c" \
 "$root/actor_target_input/command_timing.c" "$root/actor_target_input/event_signals.c" "$root/actor_target_input/target_selection.c" \
 "$root/kinematics/kinematics_replay.c" -lm -o "$out/live-contact-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/native_assets/contact_stream_original_check.c" "$root/actor_target_input/command_timing.c" \
 "$root/actor_target_input/event_signals.c" "$root/actor_target_input/target_selection.c" -lm -o "$out/contact-stream-original-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -DNFLSTREET_KINEMATICS_NO_MAIN -I "$headers" \
 "$root/native_assets/contact_playback_original_check.c" "$root/native_assets/live_contact.c" \
 "$root/actor_target_input/animation_callbacks.c" "$root/actor_target_input/animation_channels.c" "$root/actor_target_input/asset_loader.c" \
 "$root/actor_target_input/contact_selection.c" "$root/actor_target_input/pair_requests.c" \
 "$root/actor_target_input/command_timing.c" "$root/actor_target_input/event_signals.c" "$root/actor_target_input/target_selection.c" \
 "$root/kinematics/kinematics_replay.c" -lm -o "$out/contact-playback-original-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/native_assets/ball_association_original_check.c" "$root/ball/ball_state.c" -lm -o "$out/ball-association-original-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/native_assets/ball_lifecycle_original_check.c" "$root/ball/ball_lifecycle.c" "$root/ball/ball_state.c" -lm -o "$out/ball-lifecycle-original-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/native_assets/ball_attachment_original_check.c" "$root/ball/ball_attachment.c" -lm -o "$out/ball-attachment-original-check.exe"
"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/native_assets/compact_pose_original_check.c" "$root/native_assets/compact_pose.c" "$root/ball/ball_attachment.c" -lm -o "$out/compact-pose-original-check.exe"

"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/native_assets/ball_orientation_original_check.c" "$root/ball/ball_orientation.c" "$root/ball/ball_attachment.c" -lm -o "$out/ball-orientation-original-check.exe"

"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/native_assets/ball_model_original_check.c" "$root/ball/ball_orientation.c" "$root/ball/ball_attachment.c" -lm -o "$out/ball-model-original-check.exe"

"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/native_assets/ball_ownership_original_check.c" "$root/ball/ball_ownership.c" "$root/ball/ball_lifecycle.c" "$root/ball/ball_orientation.c" "$root/ball/ball_attachment.c" -lm -o "$out/ball-ownership-original-check.exe"

"$zig" cc -target x86_64-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off -I "$headers" \
 "$root/native_assets/ball_flight_original_check.c" "$root/ball/ball_flight.c" "$root/ball/ball_state.c" "$root/ball/ball_orientation.c" "$root/ball/ball_attachment.c" -lm -o "$out/ball-flight-original-check.exe"
