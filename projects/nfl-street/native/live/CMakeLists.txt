cmake_minimum_required(VERSION 3.25)
project(nflstreet_live C CXX)
# Aurora (MIT, https://github.com/encounter/aurora) is an external checkout
# outside the repository; pass its path with -DAURORA_SOURCE_DIR.
set(AURORA_SOURCE_DIR "" CACHE PATH "Aurora checkout")
if (NOT AURORA_SOURCE_DIR)
  message(FATAL_ERROR "Set AURORA_SOURCE_DIR to an Aurora checkout")
endif ()
set(AURORA_ENABLE_RMLUI OFF CACHE BOOL "" FORCE)
add_subdirectory(${AURORA_SOURCE_DIR} aurora EXCLUDE_FROM_ALL)
# The verified native draws and placement (reconstruction/native_assets) and
# the verified movement runtime (command_to_movement and the stages it uses),
# copied under src/reconstruction by build_windows_live.sh, and the GX bridge.
set(NFLSTREET_DRAWS scene_frame.c light_rig.c crowd_draw.c actor_palettes.c actor_world.c cluster_palette.c model_view.c scenery_draw.c marks_draw.c
  ball_draw.c hud_draw.c uis_draw.c matrix_stack.c gx_texobj.c player_draw.c draw_engine.c live_movement.c
  select_rings.c camera_slots.c name_tag_project.c camera_projection.c effects_draw.c actor_scale.c actor_channels.c anim_stream.c actor_pose.c actor_skeleton.c compact_pose.c live_pose.c live_locomotion.c pose.c)
list(TRANSFORM NFLSTREET_DRAWS PREPEND reconstruction/native_assets/)
set(NFLSTREET_MOVEMENT command_to_movement/actor_runtime.c command_to_movement/actor_update_replay.c
  command_to_movement/preparation_replay.c command_to_movement/command_to_movement_replay.c
  command_to_movement/reset_acceleration_replay.c input_translation/input_translation_replay.c
  actor_target_input/actor_target_input.c actor_target_input/target_selection.c actor_target_input/command_timing.c
  actor_target_input/event_signals.c kinematics/kinematics_replay.c actor_target_input/locomotion_selection.c
  actor_target_input/asset_loader.c actor_target_input/animation_channels.c)
list(TRANSFORM NFLSTREET_MOVEMENT PREPEND reconstruction/)
add_executable(nflstreet_live live_main.c gx_aurora.c ${NFLSTREET_DRAWS} ${NFLSTREET_MOVEMENT})
target_include_directories(nflstreet_live PRIVATE reconstruction/native_assets)
target_compile_definitions(nflstreet_live PRIVATE NFLSTREET_ACTOR_UPDATE_NO_MAIN NFLSTREET_PREPARATION_NO_MAIN
  NFLSTREET_COMMAND_NO_MAIN NFLSTREET_KINEMATICS_NO_MAIN NFLSTREET_RESET_NO_MAIN NFLSTREET_INPUT_TRANSLATION_NO_MAIN
  NFLSTREET_ACTOR_TARGET_INPUT_NO_MAIN)
# The original's single-precision arithmetic: no contraction into fused multiply-adds.
if (MSVC)
  target_compile_options(nflstreet_live PRIVATE /fp:precise)
endif ()
target_link_libraries(nflstreet_live PRIVATE aurora::core aurora::gx aurora::pad aurora::main aurora::vi)
# Runtime DLLs (SDL3, Dawn and its shader compiler) next to the executables.
foreach (target nflstreet_live)
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:${target}> $<TARGET_FILE_DIR:${target}>
    COMMAND_EXPAND_LISTS)
endforeach ()
# Dawn loads the D3D12 shader compiler at run time.
file(GLOB DAWN_RUNTIME_DLLS "${CMAKE_BINARY_DIR}/_deps/dawn_prebuilt-src/bin/dx*.dll")
if (DAWN_RUNTIME_DLLS)
  add_custom_command(TARGET nflstreet_live POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DAWN_RUNTIME_DLLS} $<TARGET_FILE_DIR:nflstreet_live>)
endif ()
