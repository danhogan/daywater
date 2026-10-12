# Independent original-camera field rendering — 2026-10-08

The subsequent [procedural sky milestone](native_sky_render.md) adds the
checked original dome to this path. `NFR1` field output remains unchanged;
`NFR2` supplies private sky constants and two additional draw commands.

## What now works

An independent Windows C executable renders the captured EARS field from
the original gameplay camera. It loads the six original TMdl field assets
from `envrnmt.dat`, owns geometry/color/texture decoding, and applies a
bounded subset of the measured rendering pipeline. The result visibly
reconstructs the ground, walls, trees and buildings from that camera.

Runtime inputs are extracted local assets and a private rendering profile.
There is no Dolphin/ModernGekko library, original module, ISO, savestate or
emulator RAM at rendering time. The profile contains measured draw order,
asset/texture selectors, render settings and matrices. Original camera update
logic and texture animation selection are not implemented.

## Reuse of existing knowledge

We used existing public definitions instead of reverse engineering GX again:

- [devkitPro GX header](https://github.com/devkitPro/libogc/blob/master/gc/ogc/gx.h)
  and [GX implementation](https://github.com/devkitPro/libogc/blob/master/libogc/gx.c)
  for public formats and API behavior.
- [Dolphin vertex colors](https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoCommon/VertexLoader_Color.cpp),
  [projection](https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoCommon/VertexShaderManager.cpp),
  [lighting](https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoCommon/LightingShaderGen.cpp),
  [TEV arithmetic](https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoCommon/PixelShaderGen.cpp)
  and [register layouts](https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoCommon/BPMemory.h).
  Semantics were checked against the actual local reference source. The new
  native renderer does not link Dolphin or embed its generated shaders.
- [tevsl](https://github.com/crtc-demos/tevsl) is an existing public TEV shader
  compiler/reference. This bounded path needs only the observed one-stage
  configurations; a broader material implementation can consult it later.
- [noclip.website's GX material implementation](https://github.com/magcius/noclip.website/blob/main/src/gx/gx_material.ts)
  already generates modern GPU shaders for broader GX material behavior.
  [RiiStudio](https://github.com/snailspeed3/RiiStudio/blob/master/README.md)
  documents using noclip's shader-generation work. These are further reuse
  candidates for broader materials; they are not integrated into this build.

Game-specific choices come from the original private capture, not public
hardware definitions or guessed material names.

## Evidence and confidence

The two original GPU captures still match all defined contents and all
10,638 decoded draws, now including their selected texture matrices.

`export_field_render.py` identifies **3,029 field draws** using:

1. The measured field camera established by whole position-array matches.
2. Exact original primitive command bytes and index formats.
3. Every referenced position, RGBA8 color and UV byte at the recorded draw.
4. Original texture descriptors and complete base-plane resource bytes.

There are **12,671 referenced vertices**, with each of those three attributes
checked, and **3,000 textured draws** resolving **87 original image planes**.
The remaining draws use raster color alone. Counts refer to draw commands,
not gameplay objects. Texture names with animation are resolved to the actual
captured plane by byte identity; frame timing remains open.

The earlier whole-array matcher found only 1,295 EARS draws: the recorder
stores used vertex spans, so unused array tails may be unavailable. This newer
check requires all actually referenced bytes, the original command, the known
field camera and matching texture resources. It does not invent missing data.

**Verified:** bounded draw/attribute/resource identity and repeatable native
output for this saved-match capture. Both exports produce identical profiles;
both native Windows renders produce identical BMP bytes.

**Candidate:** visual fidelity to the original frame. The original screenshot
is a nearby frame, and no exact-frame pixel acceptance test has passed.

## Implemented measured behavior

- Original perspective projection, viewport and model-view matrix.
- Clip-space clipping and perspective-correct interpolation.
- Original RGBA8 vertex colors; ambient-only lighting branch `0x403`.
- Observed texture × raster and raster-only one-stage TEV configurations,
  including scale 2, clamp, and integer interpolation/lighting arithmetic.
- Source-alpha blending, alpha compare, early/late depth, compare functions,
  and field passes with depth writes disabled.
- Original selected texture matrix and clamp/repeat/mirror base-plane sampling.

The old inspection renderer stays separate. `nflstreet_mesh_open_render`
consumes RGBA8 colors explicitly. Existing strict/inspection/stored-pose APIs
retain their previous acceptance behavior and diagnostic geometry format.
Out-of-count color indices stay file-bounded; their actual referenced bytes
are checked against the original capture for this rendering profile.

## Verification

- Linux and Windows behavioral checks: draw order, depth-write on/off, early
  alpha rejection, late alpha rejection, projection/near clipping, ambient
  integer lighting, sampled texture modulation and unsupported-state rejection.
- Model checks include RGBA8 loading, source-command identity, rejection of
  other color formats by the new path, and unchanged inspection acceptance.
- All 14 established Windows model dumps still match independently decoded
  geometry exactly: 10,582 triangles.
- Two fresh capture exports produce the same native profile; two corresponding
  Windows renders produce identical output.

Build:

```bash
bash projects/nfl-street/reconstruction/build_windows_field_startup.sh
```

Private launch/evidence root:
`W:\Daywater\research-temp\nflstreet-render-capture-20261008`.
`nflstreet-field-startup.exe` takes `envrnmt.dat`, `reference-field-profile.bin`,
and an output BMP path. `native-original-state.png` shows the result.
`native-field-verification.json` records final hashes and checks.
`W:\Daywater\Start-NFLStreet-OriginalFieldPreview.cmd` rerenders the private
reference profile and opens the resulting image.

## What remains

This is a static field reconstruction, not a full scene or playable game.
Animated players, ball, separate props, crowd, effects, UI and audio are
not rendered by this path. It uses captured order/camera/texture selection,
not reconstructed live scheduling or camera control.

Sampling currently uses bilinear base planes. Mip/LOD behavior, fog, exact
blend rounding, EFB precision, edge coverage, and exact pixel equivalence
remain open. Texture resources are verified across the capture interval;
cache load timing is not reconstructed. Complete GPU behavior is not claimed.

Next rendering milestone: obtain an exact-frame original reference, extend
the checked material path to native scene objects, and then animate actors.
Fresh match initialization and the joined simulation loop remain separate work.
