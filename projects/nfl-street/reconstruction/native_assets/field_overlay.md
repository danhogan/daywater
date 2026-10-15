# Field camera entry 14

`field_overlay.c` reconstructs `800E5CA0 -> 800D1118(0)`, the field-camera
traversal entry immediately after the players and before actor strips and
particles. It checks byte `r13-71C0`; its object argument is unused. The
0xC4-byte records are `*(802AA448)`, with word count `802AA440`; active byte
`+10`, visibility byte `+11`, fade `+14`, position `+18`, and subsidiary
quad descriptions through `+3C`. These names describe reads/writes, not
asserted gameplay semantics.

Evidence level: probable, reconstructed from GNNE69 revision 0 DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4` disassembly. Cross-compilation
passes. Original-module execution and mutation detection are pending the
external Windows runner; no emulator or pixel verification is claimed.
Read `verification.json` in the private evidence directory for the latest
gate status and exact per-context summaries. Dolphin build metadata is not
available here; saved contexts 14/15/16 were supplied by the owner.

The public API is
`nflstreet_field_overlay_draw(const nflstreet_field_overlay_context *, nflstreet_render_state *)`.
The context takes the existing scenery context, viewport matrix stacks,
small-data base (`803A9A40`), and a synchronous GXPeekZ depth service. RAM
uses big-endian F32/U32 fields, visibility and enable use U8. Missing memory
or depth service returns failure. This routine mutates the same scene RAM
and shared render-state object used by adjacent draws.

`8015A084` resets render state, pushes identity, sets vertex/channel/TEV
state, saves the seven projection words at `8030BDD4` and disables fog
(saving its restore byte at `r13-592C`). `80159E88` projects through slot 4,
tests the optional depth and strict screen bounds, and sets record `+11`.
`80159538` updates alpha and the optional kind-0 random stream, loads
textures/TLUTs, selects depth state, and draws four F32 xyz/st vertices per
layer. It uses orthographic projection except for the last layer with
record `+08 == 1`, which uses slot 3 and the saved projection. `8015A1C8`
pops/reloads the previous matrix and restores projection and fog. Despite
the task's initial scissor description, these functions issue no scissor
SDK call in the supplied disassembly.

The extended oracle header adds `80207D5C` (GXSetProjectionv),
`80207E1C..80207E7C` (GXGetProjectionv, executing original code),
`80203CE4` (GXPeekZ, supplied EFB read), `80207CA8` (GXSetProjection), and
`80200000..80200098` (C_MTXOrtho, executing original code). No game routine
is skipped or replaced. Like the existing GX oracle, setters are compared
at SDK entry, so their internal FIFO writes and GX-shadow changes are not
part of this abstraction. Every game-level FIFO write is compared.

Run from the worktree root on a host able to execute Windows binaries:

```sh
PYTHONPATH=projects/nfl-street/reconstruction/menus \
  python3 projects/nfl-street/reconstruction/native_assets/verify_field_overlay.py
```

The gate builds the executable, runs 601 cases twice in fresh processes for
each context, compares all RAM except the oracle stack and every ordered GX
call/pipe write, then builds all nine one-line mutations in
`field_overlay_mutants.json` and requires each to fail a GX/RAM comparison
in all three contexts. Case zero selects the viewport stack and forces the
entry enable byte on; all other saved RAM remains supplied. Edited cases
cover active/inactive records, modes 0/1/2/other, alpha bounds, layers,
texture changes and TLUTs, matrix/projection changes, fog, replay override
and random-stream advancement. Controlled slot-4 identity matrices make
visible and clipped cases reproducible; depth hardware responses are zero,
maximum, midpoint and a coordinate-dependent pattern. Input, baseline,
source, executable and trace hashes are recorded. Compilation failures or
crashes do not count as caught mutants.

Private evidence and all compiled output:
`/mnt/w/Daywater/research-temp/nflstreet-frame14-20261015/`.
No private RAM or game data is committed.

Not modeled: record creation and lifecycle, EFB storage/readback itself,
other random generator kinds, configuration-query machinery behind
`8014E29C(7)` (the field reads its table result, which is also selected in
mode 6). The original `80159BB4` leaves output locals unwritten when a
point clips; `80159E88` nevertheless reads them in mode 1 before clearing
visibility. The native clipped mode-1 path clears visibility without using
undefined stack contents. Edited mode-1 points stay in the written-output
region; this boundary is not a general equivalence claim for arbitrary
stack residue, nonfinite inputs or malformed records. The module does not
render pixels or update SDK projection shadow state. No live integration
has been run.

## Exact integration changes for the owner

These shared files were intentionally left unchanged.

1. In `scene_frame.h`, include `field_overlay.h` and add
   `nflstreet_field_overlay_context field_overlay;` and an explicit opt-in
   integer to `nflstreet_scene`. In `nflstreet_scene_link`, set
   `sc->field_overlay.scene=&sc->venue`,
   `sc->field_overlay.stacks=&sc->hud.stacks`, and
   `sc->field_overlay.r13=sc->r13` (or assign the r13 value after linking).
2. In `nflstreet_scene_players`, immediately after the actor loop, before
   clearing `camera_node_word` and before `nflstreet_strips_draw` /
   `nflstreet_particles_draw`, call
   `if(sc->field_overlay_enabled && !nflstreet_field_overlay_draw(&sc->field_overlay,s)) return 0;`.
   Keep the field camera selected, its derived slots 3/4 current, and use
   the same `s`; the callee handles its own enable byte. No extra matrix
   push/pop, render reset, or projection change belongs at this call site.
3. In `native/live/live_main.c`, after `scene` has its final address and
   after its existing context/link setup, supply the bridge-backed depth
   callback and user pointer, set the overlay context fields above and
   enable its opt-in. Compile `field_overlay.c` into the live target.
   The existing `nflstreet_scene_players` call at the frame's player phase
   then runs the overlay at the right point. Appending it after the whole
   players call would put it after strips/particles and change the order.
4. Extend the live GX sink (currently `native/live/gx_aurora.c`, whose
   default rejects unknown ops) to handle these local operations:
   `NFLSTREET_GX_SET_PROJECTION_V` (same value as `cloud_draw.h`, data[0]
   is projection type as F32, data[1..6] are parameters),
   `NFLSTREET_FIELD_GX_SET_PROJECTION` (arg[0] type, data[0..15] full
   row-major F32 matrix), and `NFLSTREET_FIELD_GX_PEEK_Z` (args x/y,
   data[0] the already supplied depth; observation only, no second read).
   Decode projectionv as `gx_aurora_begin_frame` already decodes the shadow:
   type 0 uses m00=p0,m02=p1,m11=p2,m12=p3,m22=p4,m23=p5,m32=-1;
   other types use m00=p0,m03=p1,m11=p2,m13=p3,m22=p4,m23=p5,m33=1.
   Flush pending primitives before setting projection or sampling depth.
   Implement the context's depth callback with the renderer's actual EFB
   depth readback and GameCube 24-bit depth convention; the check's
   deterministic callback is solely an oracle fixture, not a live substitute.
   This renderer service is a remaining integration prerequisite.

For future consolidation into `gx_oracle.h`, merge the additional SDK
entries and captures from `field_overlay_oracle.h`; the generic shared
header was not edited. No change to scene traversal, GX enums, or the
live bridge is included in this commit.
