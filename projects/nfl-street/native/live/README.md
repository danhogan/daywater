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
- **Camera control:** the controller (or `NFL_LIVE_ORBIT`) moves the field
  camera, and the whole scene follows it:
  - **Sky, lines, marks, ball and shadows** use the matrix stack.
  - **Field, field passes and rings** recompute their model-view matrices
    from the camera each frame, as the original does (verified against the
    original in the scenery gate).
  - **Players** still carry the saved frame's skinning palettes. The bridge
    re-expresses them, and the player lights, for the current camera
    (new view × inverse of the drawn view). That stand-in is geometrically
    exact but not a reconstruction of the game's skinning, which another
    thread is connecting (`cluster_palette.c` and its callers).
- **Not live yet:** game logic. Player poses, positions, the ball and the
  HUD values are the saved frame's. The name tag keeps its saved screen
  position.

## Build and run

```bash
projects/nfl-street/native/live/build_windows_live.sh
```

This copies the sources and `reconstruction/native_assets` to
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

The RAM image is your own saved state on W:. Nothing from the game is in
the repository.

If Aurora reports a shader error at startup after a code change, clear its
pipeline cache (`%APPDATA%\NFL Street live`). Pipelines cached from an
earlier, broken run are rebuilt on launch.
