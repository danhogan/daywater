# Scenery and EARS field draw

`scenery_draw.c` reconstructs how the original draws scenery instances
(`801EC8E4`) and the two venue loops that draw the EARS field: `80165DC4`
for the main field and `8015B670`/`8015B564` for the field passes. Like the
player draw (`player_draw.md`), it produces the exact GX SDK calls of the
original. Both draws share the render engine in `draw_engine.c`: models,
materials, shaders, chunks and groups.

## How the venue was located

A probe ran the original frame's scene traversal in the original module:
`80180A14(root, …, 801DEF50, 1)` from the frame routine `801DF4F4`. It
logged every scenery draw with its caller and instance. In the saved match
the frame draws, in order:

1. the five field passes (`8015B670`, object `80A9753C`);
2. three decal objects (`800E3F7C`, not modeled);
3. 15 main-field instances (`80165DC4`, object `8094D31C`);
4. the ball (`8015CB60`, not modeled);
5. the 14 players (`80164000` → `80161E10`).

The models are the EARS venue (TMdl Info names `EARS_rField.ea3` and
`EARS_FieldPass1..5.ea3`), the same venue as the 2026-10-08 GPU recording.
The probe needed two harness settings: HID2 paired-single enable for the
matrix library, and returns for the OS cache-maintenance routines at
`801F0B00..801F1000`.

## Source trace

- **Scenery instance**, 0xBC bytes. Fields: flags `+00`, group variant
  `+04`, model-view 3×4 `+08`, depth `+48`, ambient RGBA `+4C`, model `+54`,
  materials `+5C`, model view `+64` (its group table at `+78`), bounding box
  `+98`, shader override `+B8`.
- **`801EC8E4(instance, -, lights)`**, if `r13-6230` is set and the model is
  present:
  - **Frustum test (flag bit 14).** `8018A1E0` copies matrix-library slot 2
    (the projection) to `80348AD4`, and `801CB430` copies the instance
    matrix to `80348B14`. `801CAE78` computes the eight box corners as
    `x·m0 + ((y·m1 + z·m2) + m3)`. `801CB05C` computes clip codes against
    ±w (X = `P0·x + P2·z`, Y = `P5·y + P6·z`, Z = `P10·z + P11` with fused
    multiply-adds). The instance is skipped when all corners share an
    outside bit.
  - Shader override (`801C7A3C`), then material color 0x808080FF.
  - `GXLoadPosMtxImm` and `GXLoadNrmMtxImm` from `+08` into matrix 0, then
    `GXSetCurrentMtx(0)`.
  - Blend preset from flags 0x100000, 0x200000 and 0x2, giving presets 3,
    6, 2 or 1.
  - Flag 0x200 selects z compare before texturing with alpha > 32 AND > 0;
    otherwise z compare after texturing with alpha > 0.
  - **Depth.** 0x40000: LEQUAL with range `[0, 1 − r13-622C]`. 0x80000:
    LESS with range `[+48, +48]`. 0x400000: ALWAYS. Otherwise LEQUAL. Each
    writes depth unless flag bit 3. Flags 0x4C0000 restore LEQUAL/update
    and `[0, 1]` afterwards.
  - Without flag bit 0, fog is switched off for the instance and restored
    afterwards (`801EDDF8`/`801EDD74`: `GXSetFog` with the camera's
    near/far, or type 0 with the constant `80268E28`).
  - **Lighting.** Flag 0x20000 uses ambient `+4C` with vertex material and
    no lights. 0x8000 uses the light mask with spot attenuation. Otherwise
    lighting is off, with a register or vertex (0x10000) material.
  - Finally all chunks (`801C84D4`), or group 0 variant `+04` (`801C8544`).
- **`80165DC4`** draws instances `+258`, `+E0`, `+19C`, `+24` and `+314`,
  then ten from `+3D0` (0xBC apart).
- **`8015B670`** draws layers 0–4 of the passes object through `8015B564`:
  - Layer 2 has a translation (`8028A090`) and uses the alpha at
    `8028A09C`; layer 3 uses `r13-58F4`.
  - A layer is drawn when the object's `+3D8` entry and its pass record's
    enable byte (`8028A004 + 0x1C·layer + 0x19`) are set.
  - The ambient (`801ECD98`) is (0.5, 0.5, 0.5, alpha × record alpha),
    each × 255 and truncated, and the flags gain 0x20000.
  - When the record depth (`+0C`) exceeds 0, `801ECD54` sets flag 0x80000
    and `+48`.
- **Shaders.** The venue's 78 materials use OnePass, plus the "Flat" shader
  (`801C9D5C`), whose setup matches NULL. A further setup (`801C9EEC`,
  texture replace) exists in the DOL but is not used here and is rejected.

## Sky and actor shadows

- **`801655D8`, the sky dome.** It resets the render state, pushes the
  matrix-stack top (`r13-65E8 → {-, depth, slots}`) and loads it with the
  translation cleared. It computes the dome colors (`8038668C` floats ×
  255, truncated) into `80310A78` and sets up one unlit vertex-color stage
  (`GXSetTevOp` pass color). Positions (F32, stride 16, `80385E8C`) and
  colors (RGB8) are indexed. The dome's quads and triangles (counts
  `80387590/92`; index tables `8038678C`/`80386F8C`, written in reverse
  vertex order) go to the pipe, and then the full matrix is loaded again.
- **`801643F4`, actor drop shadows.** It resets, switches fog on, and sets
  no cull, source-alpha blending, early depth test without update at depth
  range [0.01, 0.01], the shadow texture (`r13-58E4/-58E0`) and the
  matrix-stack top. Each actor with `+133D` set gets one four-vertex fan:
  stored corners `+10AC`, color and alpha `+109C..+10A8` × 255, texture
  coordinates `+10DC`. Afterwards it restores depth [0, 1], LEQUAL with
  update, and the fog state.

Immediate vertex data is written straight to the GX pipe (`0xCC008000`) by
game code, not through SDK calls. Natively it is recorded as
`NFLSTREET_GX_WRITE` entries after `NFLSTREET_GX_BEGIN`. The oracle captures
the same writes through the CPU state's external-write callbacks and compares
them in sequence with the SDK calls. Both routines run fully original,
matrix stack included. In each saved context there are 301 runs: the saved
state plus 300 edited cases. The edits cover:
- sky colors, including values outside [0, 1];
- dome counts;
- matrix-stack depth and top values;
- actor shadow flags, colors and corners, and the actor count;
- a missing shadow texture (which gives the default texture);
- the fog byte.

## Player-select rings and field lines

- **`800E3F7C`, the player-select rings.** In the saved match there are
  three ring objects (`PSRingA`/`PSRingB`, types 5 and 7):
  - Each is drawn when `+F8` is set and `+20` has bit 1, and types 5 and 7
    also need their `r13-71DC`/`-71DB` byte.
  - The ambient comes from color table `8027BED4` at index `+30`: RGB ×
    `802536EC`, then A, through `801ECD98`.
  - Then depth LEQUAL without update, the default texture on map 0, and the
    instance at `+3C` through the scenery draw.
  - Type 6 (ground-probed by `800E4BC8`) is rejected. Matrices are stored,
    as for the venue.
- **`800DA074`, the field lines.** There are four objects at `80DEC17C`,
  0x124 apart: blue at y −33, red at ±40, yellow at −20.
  - **Gates, in order:** `r13-7078 → +30`; the `r13-7B88 → +182` byte
    clear; the `r13-7F40` flags (`+B64 & 0xC00880` or `+B60 == 4`) clear;
    the `r13-7258` byte equal to 1; `80308560` not positive; and the mode
    word at `*r13-7CF8` not 7, which is also stored to `r13-7CF4`.
  - **Distance scale (`800D9FA8`).** The reference box `802714D0` is
    translated to the line and projected (`801EC24C`: cull copies,
    `801CAE78` corners, `801CB1C4` bounds). The length of its projected xy
    extent (`801DE744`; the square root `802165C8` is exact, as `sqrtf`) is
    turned into a scale. Lengths of at least 0.08 give 1. Otherwise the
    scale is `1 + (min(0.08/len, 20) − 1)/2`, or 1 when that ratio is
    below 1.
  - **Drawing.** The strip has `x = ±` the half-width (`r13-7078 → +90`)
    and y from the ratio `r13-7260` and the scale. It is drawn with
    source-alpha blending, no depth test and the line texture.
  - **Alpha pulse.** `+E0` changes by `+E4` and reverses at `+E8`/`+EC`. It
    runs whenever the gates pass.
  - The translations through `80189BB0` (single-precision fused
    multiply-adds) are written to the stack slot above the top, as the
    original does.

## Not modeled

- **Matrices.** Each frame the loops recompute instance model-view
  matrices from the camera through the matrix stack (`801893C8`,
  `8018941C`, `80189BB0`, `8018949C`, and the `80189ED8` store through
  `801EC89C`). Native draws use the stored matrices, just as player draws
  use the stored skinning palettes. In the saved contexts the camera has
  already advanced past those stored matrices, so recomputing would mix
  frames.
- **Other callees.** `800D75EC` (between passes 3 and 4) and `801652BC`
  (after them) are not modeled; their content is not yet traced. The
  weather draws `8015A5CC`/`8015A7C4` (flags `802A391C/D`, clear here) are
  not modeled either.
- **Rest of the scene.**
  - effect quads (`8015F450`);
  - ten further models (`80168234`);
  - the HUD's uis screens (the name tags are in `hud_draw.md`).

  A frame traversal in the oracle lists them in draw order. Ground marks,
  ball shadows and the ball are in `ball_and_marks.md`.

## Verification

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus \
  python3 projects/nfl-street/reconstruction/native_assets/verify_scenery_draw.py
```

The original runs on the same RAM with the GX SDK entry points recorded
(`gx_oracle.h`). The matrix-stack routines and the four unmodeled callees
return at once. Every GX call, the render-state globals and all other RAM
must match. Per saved context (14, 15 and 16, each repeated twice in fresh
processes) there are 1,201 original runs, with 1,409,122 calls in context
14:

- **The saved venue:** both loops, 20 instances, 2,517 calls, 388 display
  lists.
- **300 edited loop runs:**
  - pass enables, depths and alphas, including layer 2's own alpha;
  - removed pass instances;
  - layer 3 alpha;
  - edited instances;
  - the fog byte.
- **900 single `801EC8E4` runs:**
  - random flag combinations, depth, ambient and group variants, including
    −1 and missing group tables;
  - shader overrides;
  - random and boundary bounding boxes: corners exactly on the x or y clip
    plane, with all other corners outside;
  - the enable flag, fog byte, fog camera and fog type, the depth trim, and
    light masks.

Context 15 was saved with fog off, which covers the other fog branch.

Fourteen deliberate faults were each rejected. They covered:
- the corner coordinate;
- the clip boundary (`>` for `>=`);
- an OR-based cull;
- the missing depth trim;
- blend-preset order;
- inverted z compare location;
- the fog-switch return;
- the unlit material source;
- the pass-depth threshold;
- ambient rounding;
- layer 2's alpha;
- instance order;
- group selection;
- the restoring depth mode.

A corner-order mutant is equivalent (clip codes are combined with AND/OR)
and was replaced. The first boundary boxes had their other corners inside,
so the clip-boundary fault went unseen; boxes now lie wholly outside except
the face on the plane.

The player-draw gate was re-run after the shared engine was split out. It
gives the same 854 runs and the same digests.

Private report:
`W:\Daywater\research-temp\nflstreet-scenery-draw-20261013\verification.json`.

## Use

`player_render_check.c` draws the sky, passes, shadows, lines, rings and
main field, then the 14 players and the name tags, into one target with
shared depth (`player_render.md`). Fresh native scene state, ground marks,
the ball and the uis screens remain.
