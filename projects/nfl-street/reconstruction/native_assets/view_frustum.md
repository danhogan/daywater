# Fresh-view frustum, scale and projection join (row07)

NFL Street GNNE69 North America rev0; DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Confidence is **verified within the
controlled original-DLL boundary** when the private full report passes.
Existing captures14/15/16 have no established Dolphin build in this gate.

This reconstructs801DF60C/801DF6B4/801DF5F8 on records from view_nodes.c.
It also reconstructs their signed-angle tangent80188C80, reached80216C70,
80216FA0 kernel and the first802171C4 argument-reduction branch. Larger-angle
reduction is explicitly unsupported: converted radians must have absolute
binary32 bits<=4016CBE3 (below the next original reduction threshold). This
includes the original45-degree setup and scale's angle100000, plus supported
positive/negative edited angles. It does not claim the whole tangent library.

Context borrows bounded BE RAM, r13, angle constants802617B8 and FOV scale
80263CE8. These and r13 tangent constants/coefficients must be original, stable
and disjoint from destinations. DOL section mapping pins their exact bytes and
all captures agree. Caller owns memory/lifetimes; aligned bounded spans and
supported angle scope are host preconditions, not original validation. Finite binary32
parameters are the API contract; nonfinite parameters reject as host scope. Native return is host status; all setters are
originally void. Unsupported angles reject before stores, without an original
validation/failure interpretation. No rollback of other host errors is promised.

## Original behavior and fields

All listed view fields are BE32/F32,4bytes except byte45. Writers are the named
setters; camera_projection.c reads frustum fields. Game/hash/Dolphin provenance,
procedure and confidence above apply to every field. Remaining payload stays dirty.

| Operation | Original effects |
| --- | --- |
| Frustum801DF60C | +CC=fov, +D0=aspect, +E8=near, +EC=far; flags+1C=(old&FFFFFFFB)\|2 |
| Frustum tangent | angle=truncating fctiwz(F32(fov*scale)), signed divide2 toward0; signed-angle conversion/tangent |
| Extents | top+E0=F32(near*tangent), bottom+E4=-top, right+DC=F32(top*aspect), left+D8=F32(bottom*aspect) |
| Scale801DF6B4 | ratio=F32(top/near); gain=F32(tangent(100000)/ratio); +F0=F32(scale*gain) |
| Binding801DF5F8 | byte45=flag, word48=identity; byte44 and all other bytes retained |

The tangent kernel preserves original single operations, fused multiply-adds,
bit truncation for reciprocal correction, invalid-product/division NaN7FC00000,
high-kernel reflection, sign and
first reduction's split constants/tail. Existing nflstreet_fmadds/fctiwz helpers
are reused unchanged; no libc tanf substitution. Compile with FP contraction
disabled. Small direct arguments and near-pole first reduction are verified
through actual original binary32 returns. Original routines execute unhooked.

## Test procedure

Full mode:14/15/16 twice,401cases/process. Baseline uses a captured view with
the exact80004728 parameters(45,1.333333373,0.1,750),scale4. Edited cases build
a fresh list and F8view with existing verified modules, then set frustum,
scale and binding. Four tangent probes/case include focused thresholds/poles,
positive/negative uniformly edited supported angles and real float returns.
Frustum edits vary signed FOV, aspect, near/far, flags, scale and binding bytes.
Zero-near and zero-FOV/zero-scale cases exercise the original positive quiet-NaN result of
0*infinite gain; this is compared byte for byte, not masked.
For nonzero FOV and near plane, existing camera_projection.c rebuilds slots1/2 from these
fresh records. The original screen/perspective builders run on the same input;
both slots and the rest of RAM compare after their joined output. No borrowed
captured frustum is used for edited projection cases. FOV0 and near0 are tested as setter
and scale cases; degenerate projection is outside the join fixture.

All24MiB compare after phases, except precisely accounted stack unions:

- List/view setup `[80F46F90,80F47008)` (existing proof).
- Direct tangent `[80F46F98,80F47008)`: wrapper16+outer24+reduction64+LR8.
- Frustum `[80F46F70,80F47008)`: setter40+the tangent chain104+LR8.
- Scale `[80F46FA8,80F47008)`: setter24+wrapper16+outer24+kernel24+LR8;
  its fixed original angle uses the direct kernel branch.
- Perspective `[80F46FF8,80F47008)`: wrapper8+LR8; screen/binding are leaves
  with no exclusion. Only the just-compared stack is synchronized before narrower phases.

Ordered heap/error/geterror services observe dirty list and fixture arenas;
only these external services may return early. No original game/math routine
is substituted. Original setters have no invented r3-return assertion. Eight
host guards are separate from original invocations. Eleven production mutants
must cause RAM/float-return/service discrepancies; compile failures, rejection,
crashes and timeouts never count. Fast uses context15,40edits,no mutants.

This supplies configured fresh records and projection outputs within the stated
boundary. View-system bootstrap, field/HUD type constructors, active stack/scene
handles, camera runtime callbacks and live entry remain separate. Existing live
snapshot launch and all shared consumer files remain unchanged.

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_view_frustum.py
```

Private evidence: `W:/Daywater/research-temp/nflstreet-view-frustum-20261018/`.
