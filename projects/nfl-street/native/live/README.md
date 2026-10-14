# Live native scene on Aurora

`nflstreet_live` draws the verified native scene every frame in a real
window on the GPU. It runs the original frame order (`scene_frame.c`):
- field camera;
- sky and field with marks and ball shadows;
- lines, rings, main field and ball;
- the 14 players;
- the HUD camera, name tags and in-match screens.

It uses [Aurora](https://github.com/encounter/aurora) (MIT), a
source-level GameCube GX/PAD/VI layer on WebGPU (Dawn; D3D12 or Vulkan on
Windows), so the GX implementation is not rebuilt here.

## The bridge (`gx_aurora.c`)

The native draws record GX calls (`nflstreet_gx_call`). The bridge
forwards each one to Aurora's GX as it is issued:
- **Addresses:** GameCube addresses resolve into the loaded RAM image.
- **Objects:** texture, TLUT and light objects are decoded from their SDK
  layouts and rebuilt with Aurora's init calls.
- **Immediate vertices:** vertex data goes to the GX FIFO (`GXCmd1u8/u16/u32`).
  Each immediate primitive is buffered until complete, so its indexed
  arrays can be set first.
- **Vertex and matrix arrays:** Aurora uploads arrays by size, so each is
  sized from the highest index its display lists (or immediate primitives)
  use. The scan follows a list's own CP descriptor and format writes and its
  indexed matrix loads (`LOAD_INDX`).
- **Frame start:** the saved projection and viewport, plus `801C769C`'s
  defaults (depth LEQUAL with update, every texture coordinate from TEX0,
  konst selections). `GXInit` runs once at startup, as the game does at
  boot. The render mode is the game's 640×448 EFB.

## Status

- **Matches the software render:** on saved context 15, the rendered frame
  matches the verified software render of the same state (sky, field,
  lines, ring, players, ball and HUD). It runs at about 105–125 fps with
  about 19,300 GX calls per frame on a GTX 1060 (D3D12).
- **What advances each frame:** whatever the native draws advance from
  frame to frame (field-line alpha pulse, mark alpha ramp, matrix stacks,
  screen state) carries over between frames.
- **Controlled player:** the player standing on the select ring (Thomas in
  the saved match) runs with the main stick. Each 60 Hz update runs, in
  order:
  1. the select-ring update (`800E4A24`): the rings follow the position the
     previous update left, so they trail by one update, as in the game;
  2. the verified movement runtime (`reconstruction/command_to_movement`),
     seeded from that actor's own RAM and written back to its gameplay actor;
  3. the animation pose (`8015E9C8`, `8015EC30`, `80053404`, decoding
     `animdata/00111.bin`) and the compact skeleton (`800D51B4`/`8015F08C`).
     Which clip plays is a host choice (the existing `live_locomotion.c`
     controller and an NSC1 set catalog), not the game's own selection.

  Then the native placement (`800D4A6C`/`800D4B0C`) puts it on the field.
  The camera follows by the player's movement (a host choice).
  `NFL_LIVE_SCRIPT=1` drives right, forward, then left for captures.
- **Each frame (scene_frame.c):** the field camera, its derived slots
  (`801DFAB0`) and the light update (`801ED3A0`), then the venue, the crowd
  and street props (`800D2F20`), the ball, then per player its name tag
  projection (`800DB0E4`), skinning palettes (`80162760`) and draw, then
  the HUD.
- **Camera:** the C-stick turns the field camera (or `NFL_LIVE_ORBIT`, a
  24-bit angle per frame).
- **Not live yet:** the game's own clip selection, the head and hand
  secondary motion (skipped when enabled), the slerped body form (flag
  0x4000), other players, the ball, clouds, particles and the HUD values.

## Build and run

```bash
projects/nfl-street/native/live/build_windows_live.sh
```

This copies the sources and the reconstruction folders they use (into a
fresh `src`, so stale copies can never shadow current headers) to
`W:\Daywater\research-temp\nflstreet-live-20261013`. It then builds with
MSVC 2022 and CMake/Ninja against the Aurora checkout at
`W:\Daywater\tools\aurora`. Aurora fetches a prebuilt Dawn and SDL3, and the
build copies their DLLs (and Dawn's D3D12 shader compiler) next to the
executable.

Run it from Windows:

```
nflstreet_live.exe <saved RAM image> [log file] [parts: 1 venue, 2 players, 4 HUD]
```

Options:
- `NFL_LIVE_ORBIT=1` (or a 24-bit angle per frame) turns the camera.
- `NFL_LIVE_DEBUG=1` logs Aurora at debug level.
- `NFL_LIVE_TRACE=<file>` logs every bridged call.
- `NFL_LIVE_ANIMDATA=<animdata/00111.bin>` and `NFL_LIVE_SETS=<NSC1 catalog>`
  (e.g. `W:\Daywater\research-temp\nflstreet-set-playback-20261009\sets-14.bin`)
  turn on the controlled player's animation; without them it keeps the
  saved stance.
- `NFL_LIVE_SCRIPT=1` replaces the stick with a scripted run for captures.

The RAM image is your own saved state on W:. Nothing from the game is in
the repository.

If Aurora reports a shader error at startup after a code change, clear its
pipeline cache (`%APPDATA%\NFL Street live`). Pipelines cached from an
earlier, broken run are rebuilt on launch.
