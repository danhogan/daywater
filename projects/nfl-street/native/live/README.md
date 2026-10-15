# Live native scene on Aurora

`nflstreet_live` draws the verified native scene every frame in a real
window on the GPU. It runs the original frame order (`scene_frame.c`):
- field camera;
- sky node and clouds, then field with marks and ball shadows;
- lines, rings, main field and ball;
- the 14 players, field overlay, strips and particles;
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
- **Projection and depth:** projection vectors go directly to `GXSetProjectionv`;
  overlay matrices go to `GXSetProjection`. Overlay depth probes flush pending
  primitives and use Aurora's real asynchronous EFB Z24 readback. The latest
  completed depth is retained per probe order; zero initially keeps an unready
  probe hidden. Results may lag a few frames, including after a camera move:
  this is a host timing choice, not synchronous GameCube `GXPeekZ`. An
  unavailable readback rejects the draw. The emitted peek op observes the
  supplied value and does not read a second time.
- **Frame start:** the saved projection and viewport, plus `801C769C`'s
  defaults (depth LEQUAL with update, every texture coordinate from TEX0,
  konst selections). `GXInit` runs once at startup, as the game does at
  boot. The render mode is the game's 640×448 EFB.

## Status

- **Ball handoff module (probable; original events are not dispatched live):**
  the live target links `ball_handoff.c` and its ownership/lifecycle kernels.
  It shares the ball update context and the fixed update's matrix-stack
  selection. The handoff APIs consume already-decoded snap, assignment,
  release and event callbacks; this loop has no original event decoder, so it
  does not synthesize ownership changes each tick. Automatic possession is
  not implemented.
- **HUD values (partial producer slice):** `hud_values.c` runs its turbo
  producer once per fixed 60 Hz update and stops the update if it rejects RAM.
  Score publication, UIS scripts and meter geometry are not implemented by
  this module, so this does not claim a complete scoreboard or meter refresh.

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
  3. the movement-state clip-request phase (`8003A1F4`), through
     `locomotion_predicates.c` (`800150E8` and all eleven selectors), using
     the restored movement/input blocks and stored manager/set/channel pointers.
     Changed/restarted keys call `nflstreet_clip_request_group` with the existing
     group/event adapters; suppressed/unchanged requests leave channels alone;
  4. the channel manager, program and pose passes for **all 14 actors**,
     including the controlled player, then twist and the compact skeleton
     (`80053404`, `800D51B4`/`8015F08C`). Selector and callback writes are
     copied back into persistent movement input/state before its next step;
  5. actor placement (`800D4A6C`/`800D4B0C`) for all actors, then each ball
     record in the saved pool (byte count, stride `0xF4`) runs `800C0FE4`,
     visual publication `800C09D8` and its draw matrix refresh. Delta is
     **1 game tick**, not wall-clock seconds. Matrix translation publication
     is the ball module's documented host scene connection.

  Ball motion consumes the current carrier root after movement and pose.
  Simulation runs once per fixed update, independently of render rate.
- **Animation integration (candidate; live smoke verification pending):**
  with `NFL_LIVE_ANIMDIR` set, streamed frames come from the bank files.
  The persistent `channel_callbacks` context supplies program events and
  group callbacks, including follow-on clip starts when programs finish.
  Every phase checks its retained failure status; failures log the phase,
  actor, source PC and address, stop updates and return exit code 5.
  The request adapters also supply those callbacks; request scans use
  `80015D8C`'s operand rules against the request's private scan storage.
  See [channel callbacks](../../reconstruction/native_assets/channel_callbacks.md)
  and [clip requests](../../reconstruction/native_assets/clip_requests.md)
  for the modules' evidence and remaining verification limits.

  The default controlled-player selector is now the reconstructed predicate
  module. Its request adapter propagates clip/group/event failures; every
  selection result other than 1 stops the update with diagnostics. The two
  downstream gameplay event services `80076C48` and `80076E18` remain
  unreconstructed: reaching one logs its function, actor and mode and stops
  with exit code 5. No event effects or predicate values are invented.

  `NFL_LIVE_HOST_LOCOMOTION=1` explicitly bypasses native selection and runs
  the previous host pose controller using `NFL_LIVE_ANIMDATA` and
  `NFL_LIVE_SETS`. This fallback remains for those missing gameplay event
  systems or operation without streamed banks. With channel animation on,
  all actors still run every channel pass before the fallback overwrites
  the controlled player's pose. Default animation needs `NFL_LIVE_ANIMDIR`,
  with no host catalog. The live target gives the predicates entry a local
  link alias (`nflstreet_live_locomotion_select`) because the older host
  catalog selector exports the same name; neither verified module is changed.

  The camera follows by the player's movement (a host choice).
  `NFL_LIVE_SCRIPT=1` drives right, forward, then left for captures.
- **Each frame (scene_frame.c):** the field camera, its slots (`801DF8C4`,
  `801DF938` and `801DFAB0`'s tail) and the light update (`801ED3A0`), then
  the sky node/clouds (entry 1, original enable bytes), the venue, the crowd
  and street props (`800D2F20`), the actor fans
  (`801646CC`), the ball, then per player its name tag projection
  (`800DB0E4`), skinning palettes (`80162760`, with the scaled form when
  `r13-7238` is set) and draw, then field overlay entry 14 (`800E5CA0` ->
  `800D1118`, gated by byte `r13-71C0`) while the field camera node is
  selected, then the actor strips (`801649C4`) and
  particles (`80158798`), then the HUD.
- **Camera:** the C-stick turns the field camera (or `NFL_LIVE_ORBIT`, a
  24-bit angle per frame).
- **Remaining boundaries:** the other players' movement, HUD values, full
  ball state/force dispatch, release/catch decisions, spin, wind, ground-rest
  and contacts, and the two downstream locomotion gameplay event systems.
  Unsupported ball inputs stop the update. Clouds are integrated but their
  original saved alpha yields effectively invisible pixels.
- **Integration validation:** render executables and changed live C sources
  cross-compile here. Original-module gates, render expectation regeneration,
  full MSVC build and window smoke run belong to the external check script;
  this integration makes no new runtime or exact-pixel verification claim.

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
- `NFL_LIVE_DISC=<ISO path>` reads animation banks directly through nod (the
  disc reader underlying Aurora DVD). Example:
  `NFL_LIVE_DISC=W:\Daywater\research-temp\nflstreet-GNNE69.iso`.
  It takes precedence over `NFL_LIVE_ANIMDIR`; no extracted animation files,
  `NFL_LIVE_ANIMDATA` or `NFL_LIVE_SETS` are needed for the default native
  animation path. Virtual `animdata/NNNNN.bin` paths refer to decoded entries
  of the disc's `animdata.dat` TERF archive. Invalid disc/archive reads fail
  startup with code 4. This integration is candidate until the external
  Windows byte gate and live smoke run pass; see
  [disc files](../../reconstruction/native_assets/disc_files.md).
- `NFL_LIVE_ORBIT=1` (or a 24-bit angle per frame) turns the camera.
- `NFL_LIVE_DEBUG=1` logs Aurora at debug level.
- `NFL_LIVE_TRACE=<file>` logs every bridged call.
- `NFL_LIVE_ANIMDIR=<unpacked/animdata directory>` enables channel animation
  for every actor, native group/event callbacks and the controlled player's
  reconstructed locomotion predicates and clip requests. Without it, channel animation
  and requests are off unless `NFL_LIVE_DISC` is set; with no host fallback
  the saved stance remains. The existing directory-based smoke path is retained.
- `NFL_LIVE_HOST_LOCOMOTION=1` bypasses native selection for the explicit
  host controller fallback (default: off), retained for unreconstructed
  gameplay events or missing bank streaming. Requires both files below; invalid
  fallback inputs or an animation directory with no mapped banks exit with
  code 4.
- `NFL_LIVE_ANIMDATA=<animdata/00111.bin>` and `NFL_LIVE_SETS=<NSC1 catalog>`
  (e.g. `W:\Daywater\research-temp\nflstreet-set-playback-20261009\sets-14.bin`)
  supply the explicit host fallback. They are unused unless
  `NFL_LIVE_HOST_LOCOMOTION=1`; native channel animation needs no NSC1 catalog.
  With `NFL_LIVE_DISC`, the fallback bank comes from disc entry
  `animdata/00111.bin` instead of `NFL_LIVE_ANIMDATA`. The fallback still
  requires `NFL_LIVE_SETS`: that NSC1 inspection catalog is generated from
  saved RAM and has no disc-file equivalent.
- `NFL_LIVE_SCRIPT=1` replaces the stick with a scripted run for captures.

The RAM image is still required for game state, including in disc mode. It
is your own saved state on W:. Nothing from the game is in the repository.

If Aurora reports a shader error at startup after a code change, clear its
pipeline cache (`%APPDATA%\NFL Street live`). Pipelines cached from an
earlier, broken run are rebuilt on launch.
