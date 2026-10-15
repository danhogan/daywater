# Sky node and clouds

`cloud_draw.c` reconstructs entry 1 of the field camera's scene node,
`800E398C` (object `80DE8E1C`; see the traversal table in
[crowd_draw.md](crowd_draw.md)), and the clouds it draws. Like the other
scene draws, it issues the original's GX SDK calls, its immediate writes to
the GX pipe and its RAM writes, through the shared draw engine
(`draw_engine.c`), matrix library (`matrix_stack.c`) and the existing sky
draw (`scenery_draw.c`, `nflstreet_sky_draw`).

## What the entry does

- **`800E398C`**: nothing unless the byte `r13-71E3` is set. Then
  `80187CB4(0)` (`*(r13-6608) + 0x7C × 0`, passed to the sky, which ignores
  its argument: no effect). Nothing more unless the byte `r13-71E4` is set.
  Then `GXSetZMode(0, LEQUAL, 0)`; `GXGetProjectionv` (`80207E1C`, the GX
  shadow `*(r13-4C90) + 420..438`: type as a float, six parameters); words 2,
  4 and 5 (p1, p3, p4) replaced by `802536CC` (0.0) and set with
  `GXSetProjectionv` (`80207D5C`); the sky `801655D8`; the clouds
  `800D5B3C`; the saved words put back and set again; `GXSetZMode(1, LEQUAL, 1)`.
  Returns 0.
- **`800D5B3C`**: nothing without the texture object at `802AA7A4`; else
  `GXLoadTexObj(texture, 0)` and `8015F83C`.
- **`8015F83C`**: render-state reset `801C7648`; the sky color record
  `8015F164(8030FEB4)`; its two colors `8015F3F0`; the viewport stack top
  pushed, turned a quarter about x (`80189854(0x400000)`), loaded as
  position matrix 0 after `GXSetCurrentMtx(0)`, popped; z off; direct F32
  positions and RGBA8 colors, 8-bit indexed texture coordinates
  (`8028CC80`, stride 8: the four corners); one channel, unlit, vertex
  color; one modulate stage; source-alpha blending; early z; alpha
  `GREATER 0 AND GREATER 0`; then `8015F450` for each of the `r13-7284`
  (u16) records listed at `80385C8C`; blending off; the unrotated top
  reloaded; the NULL shader (`801C7A3C(0)`).
- **`8015F164(out)`** with `8015F0E8` / `801DE858` (`out = c + (b - c) × t`,
  fused, per float over `+00..+1C`): from the period index `802A3088`
  (getters `80027718/24/38`), its blend `802A308C` (`80027758`) and the
  overlay amount `802A3068` (`80027660`) over the table `8028CBC0` (four
  0x20-byte records, then `+80` and `+A0`): period 1 and other values copy
  record 1 or 3; period 2 blends record 2 from the previous by `min(2b, 1)`;
  period 0 blends the next from record 0 by `max(2(b - 0.5), 0)`; unless the
  overlay is 0 or the period is 3, a blend toward `+80` (overlay ≤ 0.5) or
  from `+80` toward `+A0`; then period 2 above 0.5 toward the next, period
  0 at most 0.5 from the previous. NaN comparisons follow the original's
  `cror` forms.
- **`8015F3F0`**: color A `8030FED4` = record `{+00, +04, +08, +1C}`, color
  B `8030FEE4` = `{+0C, +10, +14, 802606E4}`.
- **`8015F450(record)`** (`+00` layer count, `+08` size, `+0C` height, `+10`
  fade, `+14` alpha, `+18` x, `+1C` z, `+20` top y; written by `800D59F0`
  at setup, not advanced per frame): `GXBegin(QUADS, 0, 4(n + 1))`, then
  for each of `n + 1` layers one square of side size around x/z, y
  descending by `height / (n - 1)` (0 for `n ≤ 1`) from the top, with
  `f = max(1 - i/n × fade × record+18, 0)`, color `f × A + B` (fused) × 255
  as an unsigned word clamped to 255, alpha `alpha / n` truncated (255 at
  or above 255, otherwise its low byte), and texture index 0..3.

Field names are descriptive. Evidence: verified against the original
module (below), including the NaN, negative, saturating and exact-boundary
paths.

`GXSetProjectionv` is not in `player_draw.h`'s op table and that header is
shared, so `cloud_draw.h` emits it as `NFLSTREET_GX_SET_PROJECTION_V`
(value `NFLSTREET_GX_OPS`, the next free op) with its seven argument words
as data. Sinks that do not know the op must handle or reject it.

## Verification

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus \
  python3 projects/nfl-street/reconstruction/native_assets/verify_cloud_draw.py
```

`cloud_draw_original_check.c` prepares each case as the frame does before
the venue draws (the viewport's stack selected), runs
`nflstreet_sky_node_draw` natively, then `800E398C` in the original module
from the same RAM, through `gx_oracle.h`'s runner and a service hook:
`GXSetProjectionv` is recorded (its seven words) and compared like the other
SDK calls, and `GXGetProjectionv` executes (it only reads the GX shadow).
Every pipe write is compared. All RAM outside the oracle stack and the
render-state globals must match.

Per context (14, 15, 16, each twice in fresh processes): 401 cases. Case 0
is the saved frame: 9,430 GX calls and pipe writes (134 calls, 9,296 pipe
writes: sky and 80 clouds). In all: 3,105,756 calls and writes, 714
projection changes, 330 cloud passes, 23,491 cloud strips. The edits cover
the enable bytes, the texture word, the count (0 to 84) and record list,
every record field (layer counts to 255, negative sizes, NaN heights,
alphas and colors that saturate, go negative or hit 255 exactly, fades
that reach exactly zero), all period indices (including 5 and
0xFFFFFFFF), blends and overlays at their thresholds and NaN, the color
table, every constant read, the GX projection, the stack top, the sky table
and the render state.

Fourteen planted faults were each rejected (log `mutants.log`): unfused
blend; overlay threshold `<`; color clamp on a signed word; alpha without
its low-byte mask; fade clamp losing NaN; corner order; height step sign;
color A's fourth word; projection word 5 kept; period 0 threshold `>=`;
previous period of 0 wrong; rotation sign; no final matrix reload; step
over `n` instead of `n - 1`. One first attempt (clamp at `> 0xFF`) was
equivalent to the original (255 either way) and was replaced.

`cloud_render_check.c` renders the saved frame in the `scene_frame.c`
order twice: as `scene_frame.c` draws it, and with the sky call replaced by
`nflstreet_sky_node_draw` (the proposed integration). It applies
`GXSetProjectionv` to the executor's frame projection (the reason it
includes `gx_render.c`), and draws each frame twice on one executor, keeping
the second, because the clouds rely on GX state from the previous frame
(they set only COLOR0's channel control, not ALPHA0's).

**Result: the clouds add 562 triangles but no visible pixels in these
saved frames.** The records' alpha (`+14`, 0.47 to 1.30) is divided by the
layer count and truncated without a 255 scale, so the vertex alpha bytes
are 0 or 1; with the texture's alpha (IA8, up to 248) the blend changes
nothing. With the alpha forced to 255 (a private inspection RAM,
`opaque-clouds-14.mem1`), soft clouds appear above the buildings in the top
48 rows (`opaque-compare-top.png`). Whether the hardware shows them depends
on the ALPHA0 channel state the previous frame leaves (the HUD's lit
channel, light 7 and its material color), which the inspection renderer
does not claim to reproduce. So the clouds are not the largest visual gap
of the saved frame; the original draws them as faint as above.

Private evidence: `W:\Daywater\research-temp\nflstreet-cloud-draw-20261014\`
(gate report `verification.json`, logs, renders, mutants and their script,
the opaque inspection RAM and crops, the render debug sources).

## Not modeled

- Nothing inside `800E398C` is left to the original; `80187CB4` has no
  effect and is not called natively.
- The GX shadow's projection words are read, not written: like the other
  SDK calls, `GXSetProjectionv` is emitted, not executed, so a host that
  keeps its own projection must supply it at `gx_state + 0x420` (the
  native code reads it through the span).
- The other node entries listed in `crowd_draw.md` (lights hook,
  `801646CC`, `800D1118`, `801649C4`, particles).
