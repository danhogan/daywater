# Player draw submission

`player_draw.c` reconstructs what the original game sends to the GPU to draw
one player: `80161E10`, the per-actor draw called from `80164000` after the
skinning matrices are built. The native code issues the same GX SDK calls, in
the same order and with the same arguments, as the original. That covers
lighting, channel and blend presets, every model instance, each chunk's
material (shader, TEV stages, textures and palettes), vertex formats and
arrays, and display lists. It also leaves the render-engine state and the
actor in the same final state. A renderer that executes these calls draws the
player the way the game does.

This is the submission layer. GX SDK functions, the GPU and pixels are not
modeled here.

## Source trace

Game: NFL Street GameCube GNNE69 revision 0. DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`; contexts from Dolphin 2603a.

- **`80161E10(actor)`**:
  1. Resets the render state (`801C7648`).
  2. Computes the light color. Actor `+3EC` points to a target RGB at
     `+0C..+14`. A new target restarts the approach: the current color moves
     to `+1A74..+1A7C` and the target is stored at `+1A64..+1A6C`. Then 15
     frames (`+1A84`) interpolate in steps of 1/15. A fade (`+1A88`, counter
     `+1A8C`) scales the color by `2 − t/30` for 30 frames. Otherwise
     `+1A4C == 1` applies the tint `+1A50..+1A58`. All of this is single
     precision with fused `fmadds`.
  3. **`801EDC60`/`801ED6A8`** load light set 1 (`8034FE9C + 0x108`: count,
     ambient, up to four 64-byte light objects). Unless all three scales are
     1.0, each light's color is scaled per channel (above 255 clamps,
     otherwise truncates), then `GXLoadLightObjImm` and `GXSetChanAmbColor`.
  4. Material color white, one channel, `GXSetChanCtrl(COLOR0A0, on, reg,
     reg, lights, clamp, no attenuation)`, alpha compare off,
     `GXSetCullMode(1)` (`GX_CULL_FRONT` in the public enum) and blend preset
     1 (`801C7DC0`: no blending; preset 2 is source-alpha blending).
  5. Draws the instances `+4AC`, `+D1C`, `+E0C`, the head (`80162220`),
     `+B3C` and `+C2C`.
  6. Sets the viewport depth range to `[0, 1 − actor+1084 × r13-6850]`
     (`r13-684C` when actor flag bit 3 is set).
  7. Draws `+77C` and `+EFC`. Then alpha compare `GREATER 0` and blend
     preset 2, and draws `+95C`.
  8. Draws `+68C` with preset 1 when player record `+65` is 0 (the headwear
     flag from `800DE3B8`), otherwise with preset 2.
  9. Draws `+A4C` and `+86C`, then restores preset 1, depth `[0, 1]` and
     cull none.
- **`801EC140(0, instance, 0)`** draws one model instance (0xF0 bytes):
  `801EC0D4` rebuilds the chunk list when `+14` or `+18` is set (`801EB12C`:
  for each group with selector `+1C+i` ≠ 0xFF, the chunks of that variant;
  then `+04`/`+08` gain bit 0 and `+14`/`+18` clear). Then `801C7914` selects
  the model view `+0C`, `801C7D8C` the material array `+E8`, `801C79E4` the
  matrix palette `+E0` (both matrix arrays use its first word, strides 0x30
  and 0x10), and `801C8460` draws the chunk list `+60` (count `+5C`).
- **`80162220`** draws the head. For LOD 1 and 2 (draw struct `+08`) it is
  instance `+59C`. For LOD 0 it draws the model view at draw struct `+10` +
  0x10, or, when the draw flags have bit 3 and `801626E0` compiles the morphed
  head, the shared view `803102B4`. Both eyes use texture matrix `TEXMTX0`
  with an offset from `8016373C`. The skeleton's eye angles (`+A8`, `+AA`,
  `+AE`, `+B0`, in 24-bit angle units) are converted to degrees and remapped
  by `800D5800`: ±45.619° to ∓0.03, and ±47.028° to ∓0.1 (negated). The
  offsets are zero when actor flag bit 7 is clear. The groups drawn are 3
  (offsets from `+AE`/`+B0`), 4 (from `+A8`/`+AA`) and then 2 with the
  identity texture matrix. Which eye each group is has not been established.
- **`801C826C`** draws one chunk (0x18 bytes):
  - `801C7D54` applies material `+06` (a signed index; out of range gives
    none).
  - `801CCBD8` collects the chunk's vertex arrays (12-byte records) with
    the model's base overrides. Flag bit 3 reads S16 positions as F32 with
    stride 12.
  - `801C7F18`. If the chunk's format id equals the current one, only
    `GXSetArray` runs. Otherwise `GXClearVtxDesc`, then descriptor, format
    and array per attribute, plus the matrix-index attribute for flag bit 0;
    a nonzero id becomes current.
  - `GXCallDisplayList`, and `GXSetCurrentMtx(0)` after matrix-indexed
    chunks.
- **`801C7B58`** applies a material instance, skipped when it is already
  current:
  - `801C7A3C` switches the shader (a null shader means the NULL shader
    `r13-5684`). It runs the setup callback:
    - **NULL** `801C9D0C`: raster color only.
    - **OnePass** `801C9DAC`: texture × raster, scale 2.
    - **TwoPass** `801C9DFC`: stage 0 replaces with the base texture, stage 1
      decals the overlay by its alpha, stage 2 multiplies by the raster color
      with scale 2. Alpha is constant × raster alpha.
    - The TEV presets come from `801CACE4`.
  - Then each shader stage loads its texture map through `801C7AD4`. A null
    texture loads the default texture `r13-640C`. A palette object whose
    first word is 0 is a TLUT: `GXLoadTlut` and `GXInitTexObjTlut` run first.
  - Shader parameters follow: type 2 TEV color, 4/5 channel material/ambient
    color, 6 texture matrix plus texgen. The instance holds their values from
    `+14`.

The SDK entry points were identified from their register writes
(`80203738` XF 0x103F `GXSetNumTexGens`, `802075D0` gen-mode TEV stages,
`80206F1C` BP 0xC0+2n color inputs, …) and the public GX API. The comparison
does not depend on the names: it compares arguments.

## Native API

The shared render engine (models, materials, shaders, chunks and groups) is
in `draw_engine.c` and also serves the scenery draw (`scenery_draw.md`).
Known shader setups are NULL, OnePass, TwoPass and Flat (`801C9D5C`, which
sets up like NULL).

`nflstreet_player_draw(context, actor, state)` reads and writes
original-layout objects through a bounded span callback. Tokens are namespace
identities, and 0 is null. It emits `nflstreet_gx_call` records through a sink
and keeps the render-engine state in `nflstreet_render_state`; the caller
supplies only the viewport rectangle. The context also supplies:

- the light-set table, player records and shared head model;
- the default texture and the NULL shader;
- the three setup-callback tokens that identify the known shaders;
- the two depth scales;
- the head display-list compile (`801626E0`), as an explicit service.

It rejects, without rolling back, wherever the original would call an
unmodeled callback or read past its structures:

- a shader teardown, an unknown setup, the `8039E5EC` override or chunk
  effects;
- more than 8 arrays per chunk, more than 4 lights, a chunk index past the
  table or more than 64 rebuilt indices;
- a type-3 parameter, which makes the original read four uninitialized stack
  bytes;
- an instance without a shader.

Within this call tree, `8039E5EC`, the chunk effects and the chunk LOD index
are always zero. `801C7648` and `801C7914` reset them, and nothing in the
tree sets them.

## Verification

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus \
  python3 projects/nfl-street/reconstruction/native_assets/verify_player_draw.py
```

**Verified against the original module, per saved context (14, 15 and 16, each
repeated twice in fresh Windows processes).** 854 runs of the actual
`80161E10` each match the native draw on the same RAM: every GX call with its
arguments and the values behind its pointers, the render-state globals, and
all other RAM outside the oracle stack. That is 368,870 calls in contexts 14
and 15 and 374,596 in context 16.

- **Captured players.** All 14 captured players, as they are: 4,955 GX calls
  and 340 display lists (context 14). Every chunk uses its live OnePass or
  TwoPass instance from the material hookup (`slot_materials.md`).
- **Controlled variants.** 840 draws in fresh copies cover the following:
  - LOD-0 heads with eye angles beyond both clamps, compiled and own head
    models;
  - light-color restarts, interpolation, fades and fade ends, tints that
    land exactly on the 255 clamp, and light sets with 0–4 lights;
  - shortened and edited material arrays and synthetic instances with
    NULL, OnePass, TwoPass and parameter shaders (types 2, 4, 5, 6 and
    ignored types);
  - null textures (default texture), TLUT and non-TLUT palettes;
  - chunk flags, format ids, signed and out-of-range material indices,
    position-array types and vertex-array overrides;
  - group-selector rebuilds with arbitrary instance flags;
  - the headwear flag and the depth-scale flag.

  Four further cases must be rejected natively, and are.

The original module runs every game routine of the draw. Only the GX SDK
entry points are intercepted (they would write the GPU FIFO);
`GXInitLightColor` and `GXGetLightColor` execute. The generated module
enters functions in the same code chunk directly, so `801626E0` itself
cannot be intercepted. It runs, and its helpers outside that chunk supply the
case's choice: buffer room (`801CB8AC`, threshold 0xCE3), a nonzero face
weight (`801CBC94`) and the compile calls as no-ops. The whole-RAM comparison
also shows that no unintercepted SDK function wrote its shadow state.

**Sensitivity check.** Twenty deliberate faults were each rejected:

- TwoPass decal preset;
- light rounding instead of truncation;
- `fmadds` with two roundings;
- format id 0 becoming current;
- inverted headwear blend;
- an eye-offset sign;
- missing rebuild flags;
- no default texture;
- inverted TLUT condition;
- swapped instance order;
- swapped depth scales;
- a transposed parameter matrix;
- position override without the S16 check;
- off-by-one fade expiry and interpolation step;
- unsigned material index and variant bound;
- missing `GXSetCurrentMtx`;
- a shader setup rerun for the same shader;
- no material-instance cache.

The first generator missed two of these faults: live instances already have
the rebuild flag bits, and every flagged position array was S16. It now
randomizes instance flags and array types. A clamp fault at exactly 255 is
equivalent (both branches give 0xFF) and was replaced by the rounding fault.

Private report:
`W:\Daywater\research-temp\nflstreet-player-draw-20261013\verification.json`.

## Remaining connections

- Execution: `gx_render.c` now runs these calls for the captured players
  (`player_render.md`).
- Produce the inputs natively: fresh actors with their model instances,
  skinning matrices (`80162760`), LOD selection and draw flags (`80164000`).
- The morphed head compile (`801626E0` and `801CBxxx`).

No selected-player pixels or playable match are claimed.
