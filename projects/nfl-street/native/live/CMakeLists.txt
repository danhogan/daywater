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
# The verified native draws (reconstruction/native_assets, copied beside
# these sources by build_windows_live.sh) and the GX bridge.
set(NFLSTREET_DRAWS scene_frame.c scenery_draw.c marks_draw.c ball_draw.c hud_draw.c uis_draw.c matrix_stack.c
  gx_texobj.c player_draw.c draw_engine.c)
add_executable(nflstreet_live live_main.c gx_aurora.c ${NFLSTREET_DRAWS})
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
