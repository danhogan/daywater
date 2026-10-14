# Crowd and street props draw

`crowd_draw.c` reconstructs how the in-match frame draws the venue's crowd:
the ten spectators (`EARS_SPECTATOR_A..K`), their ten copies, two garbage
cans and the static street props. These are the "ten further models
(`80168234`)" that earlier docs listed as missing. Like the other scene draws,
it issues the exact GX SDK calls of the original and the same RAM writes,
through the shared draw engine (`draw_engine.c`), matrix library
(`matrix_stack.c`), cluster palette (`cluster_palette.c`) and cull helpers
(`scenery_shared.h`).

## How the remaining draws were located

A probe ran the frame's scene traversal from `801DF4F4`
(`80180A14(*(r13-638C), 0, 0, 0, 801DEF50, 1)`) in the original module on
saved context 14, logging every function entry with its call depth and every
GX SDK entry. The source is in the private evidence directory
(`probe-src/probe.c`, log `trav14-full.log`). The field camera node
`806A1CA8` draws its scene node `80888500` through `80195634` → `80194F38`,
which calls one callback per entry. In order, with GX calls in context 14:

| # | Callback (object) | Draw | GX | Native |
| --- | --- | --- | --- | --- |
| 0 | `801DEEAC` hook → `801ED3A0` (node `806A1CA8`) | light objects `801ED324(8028C974)`: 15 × `GXInitLight*` (memory only) | 60 | no |
| 1 | `800E398C` (`80DE8E1C`) | `80187CB4`, sky `801655D8`, clouds `800D5B3C` → `8015F83C` → `8015F450` × 80 records | 5 + 19 + 110 | sky only |
| 2 | `8015B670` (`80A9753C`) | field passes, marks, ball shadows | 689 | yes |
| 3 | `800E8C18` × 10 (`80C6A8F8`…, 0x60 apart) | copy updates, no GX | 0 | — |
| 4 | `80040C3C` | actor shadows `801643F4` | 38 | yes |
| 5 | `800DB770` × 26 | no GX | 0 | — |
| 6 | `800DA074` × 4 | field lines | 104 | yes |
| 7 | `80013EAC`, `800E5430`, `800E3F7C` × 27 | rings (3 draw) | 85 | yes |
| 8 | `800DAA14`, `800D9D64` (`8094D31C`) | main field `80165DC4` | 2289 | yes |
| 9 | `800DB348` × 3 | no GX | 0 | — |
| 10 | **`800D2F20` × 45 (`80C4E094`…, 0x1DC apart)** | **the crowd and props (this module)** | 599 | **yes** |
| 11 | `80040C60` (`80B7F7BC`) | `801646CC`: per-actor textured primitives; state only in the saved frame (no `GXBegin`) | 23 | no |
| 12 | `800D23D4` (`80DEEE24`) | ball `8015CB60` | 42 | yes |
| 13 | `800E20E4` × 14 | players `80164000` → `80161E10` | 4835 | yes |
| 14 | `800E5CA0` (`80DED1FC`) | `800D1118` when `r13-71C0`: projection/scissor changes and a quad (`8015A084`/`80159E88`/`8015A1C8`) | 23 | no |
| 15 | `80040C84` (`80B7F7FC`) | `801649C4`: per-actor `801590F4` strips (actor flag 0x10000, from +1368), 12 `GXBegin`, 840 pipe writes | 69 | no |
| 16 | `800CF284` (`80D452BC`) | particle emitters `80158798` × 14 (`r13-730C` list, `r13-7310` count), pipe sprites | 165 | no |

The HUD node `806A1DB0` (name tags, uis screens) is native (`hud_draw.md`).
The overlay node `806A1BA0` only switches fog and `80187980`.

Entries 0, 1 (clouds and the node's own projection calls), 11, 14, 15 and 16
are the remaining gaps. Several use SDK entry points outside `gx_oracle.h`'s
table (`80207D5C`, `80207E1C`, `80203CE4`, and the `GXInitLight*` family
`80204DD0`/`80204F60`/`80205030`/`80205040`), which a future check must add.

## The 800D2F20 entries

- **`800D2F20(object)`**: when the byte `r13-72A4` is set, calls the hook at
  `+14`. The 33 props (banners, buildings, ground, bushes, sideline) use
  `800D2860`, which is `li r3,0; blr` (read from the saved code bytes). The
  spectators and garbage cans use `80168234`.
- **`80168234(object)`** (+04 position, +18 quaternion, +B4 offset, +CC
  heading, +DC pose, +E8 flags, +128 model instance, +12C skinning source,
  +134 skinned-this-frame byte, +138 selector, +13C fade, +144 light source,
  +150 name, +1D8 first copy):
  - Lights `801EDC60(+144 RGB or 80260F04, set 0)`, material color white,
    one channel lit by the loaded mask, alpha compare (GEQUAL 16), z compare
    before texturing, z LEQUAL with update, cull back, blend preset 2.
  - The viewport's stack: select, push, push identity, translate +04,
    `801897B8` quaternion (`801A3008` on an identity, concatenated with the
    top, rotation kept), x quarter turn, translate +B4, y rotation +CC,
    store the **model** matrix; pop; the same on the view copy gives the
    **model-view**, loaded as position/normal matrix 0.
  - `801EC298`: push identity, load slot 3 (`80331F80`), copy slot 2 to
    `80348AD4`, push, top × model into `80348B14`, pop; clip codes of the
    box `8028E2B4`; pop.
  - `8016870C` hide test: in game modes 4/7 (`802A301C`), for names
    containing `SPECTATOR` (`8017CDC8`), with the mode word (`8002E4F8`,
    which also stores `r13-7CF4`) 2, 3, 4 or 7: hidden when
    `80260F2C < ref.y × y` and both |y| exceed `802A3050`; `ref` is
    `*(*(r13-5BA4)+8)` (`800C7160(0)`).
  - `+134 = 0`. Unless culled or hidden: `801EC06C` (top × model into the
    slot above the top), the selector `801EB204(+128 + 0x20, 0, +138)` when
    play is not held (`8000C2D8`), the fade (+13C, **not modeled**), the
    instance's chunk count `+68 = 1`, then when `+E8` bit 0:
    - `+12C == 0`: rigid `801EC140(+128 + 0x0C)`.
    - first chunk flag bit 3 clear: `+134 = 1`, `801EAC78` palette on the
      model-view, then `801EC140`.
    - otherwise CPU skinning: materials `+F4`, `801EC0D4` refresh, and once
      per frame (`+134`): select the model view, palette `*(+EC)`, push an
      identity, `801EAC78` (pool `801CA4B0`, `801CA4E8` into the locked cache
      and DMA), `801CABC4` (scale the allocated matrices' 3×3 by `80260F1C`,
      DMA again), pop, `801C83A0`. Then matrix 0 and the model-view, one
      texgen/stage, `801C8460(+128 + 0x6C, +68)`.
  - Pop, cull none, then the copy chain `80168E08(+1D8)`.
- **`801C7FB4`** (per chunk of the model): for each S16 position array
  (attribute 9, type 3) and S8 normal array (10, type 1), a buffer from the
  vertex scratch ring `801C6344` (`r13-6430/-642C/-6428`, restarting when
  no more than n bytes remain, zero-padding the cursor to 32 bytes) becomes
  the array's override (`*(8039E580) + 4 × index`). With an identity pushed,
  each vertex goes through its matrix-index entry's palette matrix:
  positions by `PSMTXMultVec` (`fma(m2, z, m0·x) + (m3 + m1·y)` per row),
  normals by `PSMTXMultVecSR` (`fma(m2, z, m0·x + m1·y)`) times `8026381C`,
  stored with `psq_st` S8 (clamped, truncated). The quantization registers
  are the game's: GQR4 = 0x00060006 (S8), GQR5 = 0x00070007 (S16), set by
  `8015668C`.
- **`80168E08(copy)`** (+30 position, +3C quaternion, +50 owner, +58 light
  source, +5C next): the owner's model at its own placement, blend preset 1,
  no z-mode call, the hide test `801687FC` on its own y (+34) and threshold
  `80260F30`, the cull after the model-view (pop after), and the owner's
  `+134` deciding whether it skins (palette scale `80260F58`; the GPU path
  loads the model-view into the top first).

Evidence: verified against the original module (below). Field names are
descriptive; the ones the draw does not read are not claimed.

## Verification

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus \
  python3 projects/nfl-street/reconstruction/native_assets/verify_crowd_draw.py
```

`crowd_draw_original_check.c` prepares each case as the frame does before
the venue draws (the viewport's stack selected, the palette pool emptied as
`801CA494` does at frame start), runs the 45 entries natively, then runs
`800D2F20` for each entry in the original module from the same RAM and
locked cache. Every routine executes in the original except the GX SDK
entry points (recorded and compared call by call) and the OS cache routines
`801F0B00..801F1000` (no RAM effect); the locked-cache DMA `801F0DD4` copies
whole 32-byte blocks from the oracle's locked cache. All RAM outside the
oracle stack, the render-state globals and the locked cache must match.

Per context (14, 15, 16, each twice in fresh processes): 401 cases, 18,045
runs of `800D2F20`. Case 0 is the saved frame (599 GX calls; 20 display
lists in context 14). The 400 edited cases change placements, quaternions
(normalized and not), offsets and headings; `+E8`; `+12C`; the first
chunk's skinning bit (the GPU path); selectors with play held or not; light
sources; poses; the hide-test inputs (game mode, mode word, reference y,
limits and thresholds set exactly at |y| or at a product); the scratch
descriptor (in-place palettes) and the locked-cache flag; the pool; the
vertex ring (including exactly n and n + 1 bytes left); vertex positions and
normals (including extremes); the normal and palette scales (normals that
clamp); the stack top; the blend state; the copy chains; and the enable byte.
Context 14: 184,573 GX calls, 4,702 display lists, 3,106 per-frame skins.

Eighteen deliberate faults were each rejected: no S8 clamp; ring wrap
`>=`; hide threshold `<=`; hide limit `<=`; a copy's GPU path without the
model-view load; the GPU path not marking `+134`; unfused position
transform; normals with a translation; one fewer scaled matrix; selector
without the stale mark; a quaternion sign; culling with the model-view;
ignoring play held; no identity around the skinning; no `801EC06C` write;
a copy hidden by its owner's y; no ring padding; a copy using its own pose.
Three were missed at first and the edits were strengthened (exact
thresholds; spectators without copies, so the last one's stack writes stay
visible). A fused normal sum was dropped as a fault: its single-rounding
difference does not survive the S8 quantization in any case.

`crowd_render_check.c` renders the saved frame in the `scene_frame.c` order
with the crowd between the main field and the ball, through the software GX
executor (`gx_render.c`). The spectators appear behind the far hedge and a
garbage can by the right wall (context 14: 752 display lists, 30,644
triangles; 411 pixels change). It is an inspection render, not a pixel
claim.

Private evidence: `W:\Daywater\research-temp\nflstreet-crowd-draw-20261014\`
(gate report `verification.json`, logs, renders) and
`W:\Daywater\research-temp\nflstreet-scene-extras-20261014\` (traversal
probe and logs, mutants, crops).

## Not modeled

- The distance fade of objects with `+13C` set (`80005854`, `80007D90`,
  `801DE5DC`, `801DDFFC` and its material alpha); the draw returns 0.
- `801C6BF4`, which allocates a model view's override array when `+1C` is
  0; the draw returns 0.
- Other `+14` hooks than `800D2860` and `80168234`.
- `801EC140` with a nonzero mode (unused here).
- The remaining frame draws listed above (lights hook, clouds and the sky
  node's own calls, `801646CC`, `800D1118`, `801649C4`, particles).
