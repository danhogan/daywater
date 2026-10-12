# Independent procedural sky — 2026-10-08

The subsequent [fixed-pose player preview](native_player_render.md) adds one
bounded body/head/hair assembly. Its face/skin limitations remain explicit;
the established field and sky outputs remain unchanged.

## Result

The independent Windows C field renderer now constructs and draws the original
procedural sky before the six EARS field layers. It generates 41 vertices,
32 quads and eight cap triangles from original constants; it does not load a
captured sky mesh. All 152 referenced positions match the original capture
bytes exactly in each of two fresh reference runs. Every color and index pair
also matches. The actual Windows constructor's vertices and all 72 triangulated
faces/colors match the independently checked topology.

**Verified:** this saved match's procedural geometry, RGB8 colors, topology,
recorded draw state, repeatable native output and unchanged legacy field output.
**Candidate:** original pixel equivalence. The reference screenshot is a nearby
frame; an exact-frame comparison has not passed. This remains a static scene
slice, without players, a live camera or native match initialization.

## Original-module evidence

NFL Street GNNE69 revision 0; DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Reference uses the local
ModernGekko/Dolphin research build, Dolphin checkout
`10f7ac4fe48f74485fbc161ebf4d67242e11e671`, JIT/Vulkan, restored saved match.
These are research dependencies; the new renderer does not link them.

| Original routine | Checked interpretation |
|---|---|
| `0x800e3a60` | Initializes the constructor with radius 300 and eight segments |
| `0x800e35f0` | Builds five rings, an apex, quad indices and cap indices |
| `0x801888f0` | Initializes the eight sine-polynomial coefficients |
| `0x8018889c` | Evaluates paired float powers and fused multiply/adds |
| `0x80188a70`, `0x80188aec` | Signed 24-bit sine and quarter-turn-shifted cosine |
| `0x800e3524`, `0x800e34bc` | Interpolates the original five RGB triplets |
| `0x801655d8` | Removes camera translation, converts RGB floats to bytes and draws the dome |

Ghidra output was checked against original PowerPC instructions. Paired-single
instructions in the math routine require explicit interpretation; generic
disassembly mislabeled some of them. Native C preserves float rounding and
the fused operations, rather than substituting host sine/cosine approximations.
Original numeric constants and palettes remain private on W:.

### Verified generated arrays

| Field | Address | Representation | Captured observations |
|---|---|---|---|
| Sky positions | `0x80385e8c` | XYZ float32, big endian, 16-byte stride; 41 vertices | Radius 300, five eight-segment rings and apex `(0,0,300)` |
| Sky draw colors | `0x80310a78` | RGB8, three-byte stride; five entries | `e5e5e5`, `cce5ff`, `b2ccff`, `667fe5`, `000066` |

Procedure: restore the supplied match in two isolated fresh research runs,
record the GPU frame, apply recorded memory updates in order, and compare each
referenced position/color and index pair at the first two draw commands.
`export_sky_render.py` repeats this without running the emulator. Position
writer: constructor; position reader: sky draw routine/GPU array. Color writer:
sky draw routine's float-to-byte conversion; reader: its GPU color array.
These arrays are generated in BSS, rather than named TMdl sky assets.

The RGB bytes match palette row seven at `0x8027babc + 7*60`. This is a checked
source palette for the native profile, **not** evidence of a particular live
time value or a reconstructed palette-selection policy. Quantized colors
alone do not determine the interpolation factor.

## Measured rendering behavior

- Draw sky first, using original camera rotation with translation zeroed.
- Original perspective sky projection has zero far coefficient.
- Unlit vertex RGB; register-sourced alpha 255.
- Raster-only one-stage TEV, scale one and clamp.
- Blending, depth test and depth writes disabled.
- The existing field lighting, scale-two TEV and depth/blend settings follow.

Public GX/Dolphin definitions already consulted in
[native_field_render.md](native_field_render.md) supply hardware semantics.
No emulator code or original machine-generated C is linked into this renderer.

## Format and verification

The original `NFR1` profile remains supported. `NFR2` adds a 128-byte `NSK1`
block after the same 40-byte header, then two sky draws before the existing
field commands. The block supplies radius, angle constants, elevations,
polynomial coefficients and a selected original palette. Geometry is generated
by `sky.c`. Layer six and source offsets zero/one are native internal selectors,
not claimed original game enums or addresses.

Meaningful Linux and Windows checks cover unlit colors, scale one, register
alpha, disabled blending/depth, rotation-only matrices, malformed configs,
and unchanged field behavior. Both private profiles and Windows BMPs match
exactly. The extended executable also produces the previous `NFR1` field BMP
byte-for-byte unchanged. Baseline runtime source was not edited; baseline DLL
SHA1 remains `dd521cc38b0db6bca98a0bb261570a2be5e6e0ed`.

Build with `reconstruction/build_windows_field_startup.sh`. Private artifacts:
`W:\Daywater\research-temp\nflstreet-render-capture-20261008`:

- `nflstreet-sky-field-startup.exe`, `reference-sky-field-profile.bin`.
- `native-sky-field.png`, `native-sky-verification.json`.
- `windows-sky-geometry.bin`, `sky-checks.log`, `sky-field-checks.log`.

`W:\Daywater\Start-NFLStreet-SkyFieldPreview.cmd` regenerates and opens the
combined image. Live sky palette updates, camera control, remaining scene
objects, actors and exact original pixels remain open.
