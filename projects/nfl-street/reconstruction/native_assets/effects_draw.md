# Actor fans, strips and particle emitters

`effects_draw.c` draws three more entries of the in-match scene traversal
(the table in [crowd_draw.md](crowd_draw.md)), with the original's GX calls,
immediate vertex writes to the GX pipe and matrix-stack writes, through the
shared draw engine and matrix library.

| Entry | Callback | Native | Draws |
| --- | --- | --- | --- |
| 11 | `80040C60` -> `801646CC` | `nflstreet_fans_draw` | each actor's textured fans |
| 15 | `80040C84` -> `801649C4` -> `801590F4` | `nflstreet_strips_draw` | four triangle strips per actor with flag 0x10000 |
| 16 | `800CF284` -> `80158D24`, `80158798` x N | `nflstreet_particles_draw` | emitter quads (textured or flat) |

Evidence level: verified against the original module (below); field names are
descriptive, and the ones the draws do not read are not claimed.

## What each does

- **Fans (`801646CC`)**: reset, fog forced on (`801EDDF8`, previous value
  kept), cull none, blend `(1,4,5,5)`, z compare after texture, z `(1,3,0)`,
  position/colour/texcoord formats, one channel, one texgen and TEV stage,
  the shadow texture `r13-58E4/-58E0`, the stack top as position matrix 0.
  For each actor (`r13-7BBC`/`r13-7BB8`) with `actor+133D` set, `actor+133C`
  records of 0x90 bytes from `actor+10FC` draw a `GXBegin(0xA0, 0, count)`
  fan (count byte at `+8C`): vertices (3 floats) at `+12 j`, then the actor's
  colour `actor+109C` (r, g, b, a floats x 255, low bytes), then st at
  `+54+8 j`. Afterwards depth range `80260B8C/90`, z `(1,3,1)`, fog restored.
- **Strips (`801649C4`, `801590F4`)**: for each actor whose record (`+4A8`)
  flag `+04` bit 0 is clear (and, with `+14` bit 3, when `800DFAF0` allows:
  r13-7238 clear) with `+14` bit 16, and the byte `r13-72C0` set: reset,
  blend 6 (or 2), alpha compare, top as position matrix 0, then for each of
  the four strips at `actor+1368 + 0x8C i` with count (`+7C`) above 1 a
  `GXBegin(0, 0x98, count + 2)`: the two edge points (`+60`/`+6C` selected by
  `+80`) in the carried colour, then `count` ring positions starting at the
  head (`+78`, advanced mod 8 each vertex), each with a colour (table
  `80270CA4` when the mode byte `+23C` is set, which also becomes the carried
  colour for the next strip) and the alpha `(a - i * (a / count)) * scale`
  (`+230`). Note the call order `GXBegin(0, 0x98, n)` is the game's.
- **Particles (`800CF284`)**: when `r13-7304` is set: push identity, the top
  as position and normal matrix 0 (`801C6D4C(0,0)`), pop; then
  `80158798` per emitter of the `r13-730C` list (`r13-7310` count): when the
  emitter (`+2B4` enabled, `+3B8` particles) draws, push, z `(1,3,0)`, blend
  by `+60` (`80158508`: 0 -> 2, 2 -> 0 plus `GXSetBlendMode(1,1,3,5)`,
  else 6), textured (`+45` with a record `+2AC`: TLUT `+9C`, texture `+7C`,
  shader `r13-68FC`, setup `80158580`) or flat (`r13-68F8`, setup
  `80158648`), `GXBegin(0x80, 0, 4 n)`, and per particle a quad: centre
  through the top (`801DEBE8`), colour with the fade-in/out of `80197594`,
  textured colours scaled by `+35B * 0.01` and clamped to [0, 1], the sheet
  cell of `8017A7C8`, size `+4C` or `+30` (`+41`), then push identity, rotate
  about z by `spin(+50) * 46603.38` (24-bit angle), the four corners
  `802898C0` scaled by the size, rotated by that matrix and added to the
  centre, pop. After the emitters `800CF66C` and `80167874` run only for
  `r13-72FC == 1` / `r13-58BC != 0` (both clear in every saved context).

The two particle shaders' setup callbacks `80158580` and `80158648` are
these emitters' own; `effect_shader` reproduces `801C7A3C` for them
(teardown check, `+04` callback, shader pointer kept) because the shared
engine's table knows only the four model shader setups.

## Verification

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus \
  python3 projects/nfl-street/reconstruction/native_assets/verify_effects_draw.py
```

`effects_draw_original_check.c` edits a fresh copy of the saved RAM per
case, runs entries 11, 15, 16 natively in that order (the render state
carries over), then runs `80040C60`, `80040C84` and `800CF284` in the
preserved original module from the same RAM. Every routine executes in the
original except the GX SDK entry points (recorded and compared call by
call) and the OS cache routines `801F0B00..801F1000`; every immediate write
to the GX pipe is compared as well, and all RAM outside the oracle stack and
the render-state globals must match. Per context (14, 15, 16, each twice in
fresh processes): the saved frame (3,869 / 2,487 / 3,264 GX calls, of which
3,612 / 2,296 / 3,028 pipe writes) plus 400 edited cases, 1,203 original
runs. Context 14: 1,291,638 GX calls and 1,186,493 pipe writes over 1,951
fans, 5,802 strips and 22,499 particle quads.

Edits: the stack top; the gates (`r13-72C0`, `r13-7304`, `r13-72FC`, the
actor count, actor flags 0x10000 and 8, record flag bits, the fog byte);
fans (enable, count 0-4, colour, vertices, st, vertex counts 0-7); strips
(mode byte, scale, colours, alpha, ring positions, head including wrap and
far negative values, count 0-11, edge selector 0-2, edge points, the
history table); emitters (enable, particle count up to the valid list,
textured byte, blend kind 0-4, size source, fade flag, gradient byte with no
gradient, colours, alpha, fade times at the exact boundaries, colour scale,
sheet-rate, the sprite sheet record (frames, animation byte, rows, columns,
cell size), particle positions, sizes, ages, frames, ticks, spins); the
shader pointers (swapped), the render state's shader and blend.

Refusals are verified too: six directed cases (scaled-actor gate, gradient,
`r13-72FC == 1`, `r13-58BC`, zero sheet divisor, missing shader) must be
refused natively rather than drawn.

Mutation tests (37 faults, one at a time in a scratch copy, each caught in
all three contexts; the list is in the private `mutate.py`): ring wrap mask,
second edge selector, fade ramp offset, history alpha, history colour not
carried, strip colour not carried, no lower/upper colour clamp, sheet width
and rows swapped, size sources swapped, fade kind, no additive blend, spin
axis, unfused view transform, strip vertex count, TLUT/texture order,
negative-head wrap, unanimated cell, animation rate step, frame wrap, kind-2
fade-out, gradient test, scaled gate, record flag mask, flat shader, flat
TEV order, no particle pop, strip blend, colour byte order, and for the fans
tex order, colour order, fog restore, gate, count, depth range, stride and
blend. Four were missed at first and the edits were strengthened (far
negative heads, other record-flag bits, a directed refusal for the scaled
gate, a syntactically broken mutant fixed).

Private evidence: `W:\Daywater\research-temp\nflstreet-effects-draw-20261014\`
(gate report `verification.json`, logs, `mutate.py`, mutant sources).

## Where the calls go in `scene_frame.c`

- Entry 11 (`nflstreet_fans_draw`): between the main field's neighbours as in
  the traversal table: after the crowd (entry 10) and before the ball
  (entry 12), i.e. in `nflstreet_scene_venue` just before `nflstreet_ball_draw`.
- Entries 15 and 16 (`nflstreet_strips_draw`, then `nflstreet_particles_draw`):
  after the players (entry 13), i.e. at the end of `nflstreet_scene_players`
  after the loop, inside the camera-node bracket (the node word is zero
  between frames; these callbacks do not read it) -- or immediately after
  `nflstreet_scene_players` returns in the host. They follow entry 14
  (`800E5CA0`), which is not modeled.
- All three need the viewport's matrix stack selected as the players left
  it (`palettes->stacks`) and the shared render state `s` passed on.

Context fields (all values come from saved globals; the fixture in
`effects_draw_original_check.c` shows each): `draw` (the scene's player draw
context), `stacks` (the scene's `venue`/`palettes` stacks), `actors` and
`actor_count` (`r13-7BBC` value, `r13-7BB8` token), `gate` (`r13-72C0`),
`scaled_gate` (`r13-7238`), `history` (`80270CA4`), `byte_scale` (`80260438`);
particles: `enable` (`r13-7304`), `count` (`r13-7310`), `list` (`r13-730C`
value), `gate_72fc`, `gate_58bc`, the two shader values (`r13-68F8`,
`r13-68FC`), `corners` (`802898C0`) and the constants `802603D8..E8`,
`80261218/1C`, `8026323C`; fans: the scenery context (fog and shadow texture
fields) plus `80260B88/8C/90`.

## Not modeled

- Entry 14 (`800E5CA0` -> `800D1118`, run when `r13-71C0`): the
  projection/scissor changes `8015A084`/`8015A1C8` (SDK entries `80207D5C`,
  `80207E1C`, outside `gx_oracle.h`'s table) and the visibility tests of
  `80159E88`; only the two bracket calls issue GX in the saved frames (23).
- The scaled-actor gate `800DFAF0` when `r13-7238` is set (`800DF1EC`).
- The particle gradient `80198D74` (emitter `+3D` set with `+8C`).
- `80158E00` (`r13-72FC == 1`) and `80167874` (`r13-58BC` nonzero).
- A zero sprite-sheet divisor (the original divides by zero) and a null
  emitter shader (the shared NULL shader path).
- Clouds and the sky node's own calls (see crowd_draw.md).
