# First independent fixed-pose player — 2026-10-08

## What works

The independent Windows C preview now renders one clothed player, both on
the reconstructed field and in a close-up. It loads original model geometry,
normals, UVs and texture index planes itself. The bounded preview uses a
measured fixed pose, light settings and appearance palettes.

There are **557 draw commands, 2,092 triangles and 13 decoded texture planes**.
No Dolphin/ModernGekko library, original module, ISO, savestate or saved RAM
is needed at rendering time. Inputs are locally extracted assets and a private
`NAP1` pose/appearance profile. This does not yet evaluate native animation or
initialize a match.

## Confidence and limits

**Verified:** 465 body/hair draws have exact original primitive, referenced
position/normal/UV bytes and selected pose-matrix identity in both captures.
All 557 draws have checked original command/UV/pose selections. Original
texture index planes are matched by full resource bytes. Actual Windows
triangle positions match an independent asset/affine calculation within
`0.000001303` units; all 6,276 triangle-vertex UVs match exactly.

**Probable:** the chosen contiguous body/head/hair palette group is one player.
The original player/roster identity is not established. Duplicate geometry
sources remain aliases; a matching hair filename does not prove identity.

**Candidate:** complete player appearance and original pixels. There are two
explicit substitutions/omissions:

- All 92 head draws use original `BaseHead1500` geometry. The capture's face
  positions and normals differ from the original template; runtime face
  personalization is not reconstructed. The template's commands, UVs and
  pose are checked, but its geometry is not claimed to match that player's face.
- For 66 three-stage skin draws, the secondary texture index plane has no
  complete immutable resource match in this recording. Its origin is unresolved.
  The preview renders the checked base skin texture without that overlay.

Pose matrices, light state and dynamic palettes are captured constants.
Native evaluation of the 75-bone body skeleton and its 128 weighted cluster
matrices remains open. This is fixed-pose rendering, not native animation.
Lighting follows the measured branch, while exact rasterization, back-face
culling, mip/LOD, texture animation and exact original pixel comparison remain
open. The existing screenshot is a nearby frame, not an exact-frame comparison.

## Original capture evidence

Game: NFL Street GNNE69 revision 0. DOL SHA1:
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`.
Local ModernGekko/Dolphin JIT/Vulkan research build, Dolphin checkout:
`10f7ac4fe48f74485fbc161ebf4d67242e11e671`.

Frame-one draws 4,329 through 4,885 form the selected contiguous group. Exact
asset references identify `HeavyPlayer1500`, original head template commands,
and matching hair sources. The original body palette is followed by head and
hair palettes, before the next repeated body begins.

Measured first-instance position-matrix source arrays:

| Role | Captured CPU alias | Type / stride | Interpretation and confidence |
|---|---|---|---|
| Body | `0x80744060` | Big-endian float32, 12 floats / 48 bytes | Indexed matrix-load source; verified for this frame |
| Head | `0x80745860` | Big-endian float32, 12 floats / 48 bytes | Separate indexed source following body; verified capture, player association probable |
| Hair | `0x80745d40` | Big-endian float32, 12 floats / 48 bytes | Separate indexed source preceding next body; verified capture, player association probable |

These are measured addresses for this restored state, not stable actor pointer
chains. Test: restore the supplied match in two isolated fresh research runs,
record the GPU frame, apply memory updates in order and compare every original
selected vertex and indexed matrix load. Position readers are FIFO indexed
loads (`0x20`) followed by vertex matrix selection. The upstream CPU pose writer
has not been identified here. Observed values are the fixed captured pose;
private reports retain source selectors and full matrices.

The research-only inspector optionally exports position/normal matrix banks,
lights and palette uploads with `--actor-state`. Its default output stays
unchanged. Palette uploads are replayed into an isolated TMEM buffer, starting
from captured initial TMEM; the selected RGB5A3 palette then accompanies each
native texture selector. No runtime memory addresses enter the native player
loader or a gameplay policy.

## Native implementation

`player.c` loads selected original display-list primitives and applies the
recorded matrices to original positions and normals. Position selectors can
use both matrix banks; normal selectors use the public GX `index & 31` rule.
It applies the observed `0x51e` channel branch: three unattenuated lights,
clamped diffuse response, register material and ambient colors. Paletted MMAP
planes are decoded by the existing native texture reader using measured color
palettes. Base materials use measured scale-two texture/raster modulation and
depth/alpha/blend settings. The head and skin limits above remain explicit.

Hardware semantics reuse the public references already listed in
[native_field_render.md](native_field_render.md), checked against the local
reference's `VertexShaderGen.cpp` and `LightingShaderGen.cpp`. The independent
renderer does not link those sources or their generated shaders.

`NAP1` contains source asset/category selectors and primitive references,
matrices, lights, palette colors and rendering state. It does not contain
triangles, original vertex arrays, normals, UVs or texture index planes.
Native category/layer identifiers are internal selectors, not original enums.

## Verification and use

- Both fresh original captures produce identical private player profiles.
- Windows geometry/texture/portrait checks pass; truncated and bad-version
  profiles are rejected.
- Both Windows field-player images and both portraits match byte-for-byte.
- The same extended build reproduces prior field-only and sky-field BMPs
  byte-for-byte unchanged. Existing Linux renderer checks still pass.
- Original runtime source hash is unchanged; baseline DLL SHA1 remains
  `dd521cc38b0db6bca98a0bb261570a2be5e6e0ed`.

Build with `reconstruction/build_windows_player_startup.sh`. Private files:
`W:\Daywater\research-temp\nflstreet-render-capture-20261008`:

- `native-single-player.png`, `native-player-portrait.png`.
- `reference-player-profile.bin`, `repeat-player-profile.bin`.
- `native-player-verification.json`, `native-player-mesh.bin`.
- `player-checks.log`, `native-player-geometry-verification.json`.

`W:\Daywater\Start-NFLStreet-PlayerPreview.cmd` rerenders and opens the portrait.
The next substantial step is native skeleton/cluster evaluation and one
animation clip, followed by a native clock and movement input.

## Subsequent animation preview

The [native clip preview](native_player_animation.md) now computes moving body
and template-head poses from original compressed frames. The fixed-pose
preview described here remains available and unchanged.
