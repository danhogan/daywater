# Actor scale before palettes

Confidence: **probable**, from decompile and instruction-by-instruction inspection.
The Windows checker and seven mutant executables compile; original-module execution
and mutation results are **pending owner execution**. Nothing here claims live
Dolphin integration or a completed original-execution gate.

Scope: NFL Street, GameCube GNNE69 revision 0, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. RAM is big-endian MEM1 at
`80000000`, size 24 MiB; r13 is `803A9A40`. Dolphin build was not supplied
with these snapshots. Sources are the lowercase `800dfba8.txt`, `800dfb34.txt`,
`800df1ec.txt`, `800df5c4.txt`, and caller `80164000.txt` under
`/home/dan/daywater/cache/binaries/46e3de854998a0f66c9be8c72a1164d22cd29eb4/`.
The only callee of either scale routine is `800DF1EC`, which has no callees.

| State | Type / size | Read / write and interpretation | Confidence |
| --- | --- | --- | --- |
| r13-7238 (`803A2808`) | u8 / 1 | Both routines test nonzero; `800DF5C4` returns this byte | probable |
| r13-7234 (`803A280C`) | BE pointer / 4 | Scale-object base, read only when enabled | probable |
| visual actor+1361 | u8 / 1 | Unsigned index k; no bounds test | probable |
| object+464+4*k | BE pointer / 4 | Compared to actor; slot = k when equal, k+7 otherwise | probable |
| object+4E3+slot | u8 / 1 | `800DFBA8` writes exactly 1 | probable |
| object+49C+4*slot | BE binary32 / 4 | `800DFB34` reads scale once | probable |
| `80252D24` | BE binary32 / 4 | Disabled-path scale source; read from RAM, not hard-coded | probable |
| caller output+8, +0, +4 | three BE binary32 / 12 | Original stores z, x, then y, all from the same loaded float | probable |

There are no actor writes, scale updates, matrix operations, multiplies, additions,
or fused operations in these routines. `800DF1EC` performs integer pointer equality
and adds seven on mismatch. `800DFB34` performs one `lfs`/`lfsx` and three `stfs`.
The native API copies the binary32 word to host floats in z/x/y order. It accepts
finite binary32 inputs, including signed zeros and subnormals; NaN payload/FPSCR
behavior is unmodeled. Raw bytes above six follow the original unchecked indexing,
provided the requested spans exist. Wide indices can alias other object fields.
The mark and vector routines each independently reread the enable, base, actor
byte, and owner word, as the original does. In particular, the paired call does
not cache the index across a marker write.

Observed saved inputs in contexts 14, 15, 16: enable byte 0, scale-object pointer
0, disabled constant word `3F800000` (1.0f), and actor indices
`0,1,2,3,4,5,6,0,1,2,3,4,5,6` in visual-table order. Those observations do not
establish the enabled object's lifetime or its initialization/update semantics.

The native API is:

```c
int nflstreet_actor_scale_mark(nflstreet_draw *d,uint32_t r13,uint32_t actor);
int nflstreet_actor_scale_vector(nflstreet_draw *d,uint32_t r13,uint32_t actor,float out[3]);
```

Both return 1 for success, 0 for inaccessible spans (a host validation choice).
The vector routine writes three host float objects; it does not write a RAM
output token. The checker serializes these floats as BE words for RAM comparison.
The caller must provide a valid three-float host buffer. No external service,
GX interception, hook, or native stand-in is used by the original runner.

## Checks for owner execution

From the worktree root, build and run all checks plus mutants:

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_actor_scale.py --mutants
```

Compilation alone, as exercised in the sandbox:

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_actor_scale.py --build-only --mutants
```

Private evidence/output directory:
`W:\Daywater\research-temp\nflstreet-actor-scale-20261014`.
`--build-only` writes `build-verification.json` with `compiled-not-run`, never a
passing execution report. Normal execution writes `verification.json`, protects
baseline/input/source hashes and requires identical fresh-process logs twice per
context. Each baseline process must report 16,842 actor batches: 401 cases × 14
actors × 3 independent modes (mark-only, vector-only, paired). The modes execute
22,456 original routine calls altogether. Case zero keeps saved inputs unchanged;
only the guarded output scratch is seeded. The other 400 cases edit the enable
byte (0 and arbitrary nonzero bytes), object base (two supplied scratch objects),
actor index (ordinary and full-byte range), owner pointers, scale words, marker
bytes, and disabled constant. Marker bytes are seeded away from one before owner
edits; output vectors and guards are randomized. All edits are identical in both
RAM copies. The objects are supplied fixtures because all three saved pointers
are null. Scratch object bases `80F33000`/`80F34000` and output `80F37000` are
checker choices, not discovered game addresses.

After each mode, compare all 24 MiB outside `[80F38000,80F47000)` (the oracle
stack), including every actor's separate vector and its guard. Digests exclude
that same stack. Nonzero enabled, disabled, owner-match, owner-mismatch and
wide-index counts are required. The original `800DFBA8` / `800DFB34` / `800DF1EC`
all execute unmodified. The native calls run before the oracle from the same
prepared image. No scale-object initializer, scale interpolation, palette call,
GX work, or whole-frame drawing is tested here.

`actor_scale_mutants.json` contains seven exact, unique one-line replacements:
actor index byte, owner equality branch, second-bank increment, marker value,
scale-table offset, y sign, disabled constant address. `--mutants` builds each in
a temporary source directory inside this worktree and places executables under
`mutants/<name>/` in the private output directory. It requires exit 7 and a RAM
mismatch in all three contexts; a crash or unrelated failure does not count as a
caught mutant. Catch results are pending, and must not be inferred from compilation.

## Scene integration (shared files unchanged)

Add `#include "actor_scale.h"` to `scene_frame.c`, add `actor_scale.c` to its
consumers' build sources, and replace the final return in its `actor_palettes()`
helper with the following, after the existing `if(flags&1)return 1;`:

```c
float scale[3];const float *vector=NULL;
const uint8_t *enabled=nflstreet_draw_at(d,0x803a9a40u-0x7238u,1);
if(!enabled)return 0;
if(*enabled){
 if(!nflstreet_actor_scale_mark(d,0x803a9a40u,actor)||
    !nflstreet_actor_scale_vector(d,0x803a9a40u,actor,scale))return 0;
 vector=scale;
}
return nflstreet_matrix_select(d,&c->stacks)&&
 nflstreet_actor_palettes(c,actor,vector,actor+0x3ac,record+0x28,lod);
```

This follows `801641C8` through `80164230`: test `800DF5C4`, call `800DFBA8`
then `800DFB34` when enabled, pass that vector to `80162760`, otherwise pass NULL.
The existing palette implementation already applies the vector. The helper's
existing flag and LOD handling remains the integration boundary; this module
reconstructs no other part of `80164000`.
