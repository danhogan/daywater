# Actor palettes and placement

## Verified scope

`actor_palettes.c` reconstructs `80162760`, the per-actor skinning step
that `80164000` runs before each player draw (`80161E10`). It is called
with no lineup record. It works on saved RAM through the draw span:

- **Root:** push the viewport's stack, concat the actor's world matrix
  (`+3AC`), scale by `+18`, and by an optional x/y/z vector (`800DFB34`).
  Then push, translate by `+34` and turn about y by `+4C` (unless flag
  `0x2000`).
- **Body (`+4B8` → descriptor `+1008`):** `801EAC78`. The frame's palette
  pool (`801CA4B0`: an even matrix count from `r13-5680[r13-5678]`, the
  used count at `r13-5670`) gives the destination. `801CA4E8` builds the
  palette in the descriptor's locked-cache word (or in place), and the
  DMA `801F0DD4` copies it out in whole 32-byte blocks.
- **Attachments (`+5A8`, `+788`, `+968` → `+1014`, `+102C`, `+1038`):**
  `801EACF4`/`801CA720` into the locked cache. Below LOD 2, matrices whose
  cluster row is tagged 13 (or a one-cluster model's matrix) are scaled by
  `+1358` except their translation. Then they are copied out.
- **Head (`+698` → `+1020`):** on skeleton bone 13 (`+60` + `0x340`),
  with the body pose's bone-13 angles copied into the head pose (`+344`).
  Rows tagged 0 are scaled.
- **Hands (LOD 0) and held items (below LOD 2, when enabled at `+78`/`+C0`):**
  on bones 20/14 and 23/17, with three angles copied from the body pose.

The cluster arithmetic is the other thread's verified kernel, factored out
as `nflstreet_cluster_palette_rows` and shared with `cluster_palette.c`.
Its gate still passes.

`actor_world.c` reconstructs the placement `800D4A6C`: it copies the
gameplay actor's position (`+1A8`) and heading (`+1C0`) to its visual (`+04`,
`+24`), mirrored when the side record (`r13-7078`, `+39`) is 1. It then
builds the world matrix `800D4B0C`: a pushed identity, then translate,
turn about z by the heading plus a quarter turn, about x by a quarter turn,
and the `0x2000` offset, stored at `+3AC`.

**Limits:**
- Flag `0x4000` now routes through the compiled `801EAD98`/`801CA900`
  reconstruction in [gekko_quaternion/actor_slerp.c](gekko_quaternion/actor_slerp.c).
  This new route awaits the main checkout's original-module comparison;
  its procedure and limits are in [actor_slerp.md](gekko_quaternion/actor_slerp.md).
- The head and hand secondary dynamics (`8004C268`/`8004BF1C`) are modeled
  only in their disabled form (clearing the state's `+02` byte). When
  enabled they are skipped and their state stays as stored.

## Original execution gate

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_actor_palettes.py
```

Private evidence: `W:\Daywater\research-temp\nflstreet-actor-palettes-20261013`.
Per saved context (14, 15 and 16), 401 cases run all 14 actors natively and
then through the original module from the same RAM and locked cache. That is
5,614 runs of `80162760` per context, covering LOD 0 to 3, scale vectors, 482
hand and about 750 held-item palettes, and in-place bodies. Every RAM byte
outside the oracle stack and the whole locked cache must match. Edits cover:

- world matrices, scales, offsets and headings
- flags, LOD and pose angles
- the attachment scale and held-item enables
- scratch words, the dynamics and cache-enable flags
- hand models (borrowed held-item models: the saved actors have none)
- the camera

401 more cases run `800D4A6C` for every gameplay actor, with edited
positions, headings, `+30C`, flags, offsets and the mirrored side.

The original module runs everything except four routines, which return at
once: the enabled dynamics, the cache enable and flush (no RAM effect), and
the DMA, which is served from the oracle's locked cache. 16 planted faults
were caught: heading axis, DMA rounding, row tags, head and item angles,
even allocation, the dynamics byte, uniform scale, the attachment LOD gate,
the 16-word root store, the cache flag, and five in placement.

## In the scene

`scene_frame.c` rebuilds each drawn actor's palettes before its draw. Draw
flag bit 0 of the `+4A8` record skips the actor. The pool starts empty at
the players, as at frame start. The saved RAM holds the actor state one
update after the frame its stored palettes were drawn with. Rebuilt
palettes therefore show slightly later poses (the scene render digests
changed accordingly). In the live app the players follow the camera through
these rebuilt palettes. Only the player lights are still re-expressed from
the saved frame.
