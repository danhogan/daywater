# View-system bootstrap with fresh type-zero dispatch (row07)

NFL Street GNNE69 North America rev0; DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Confidence is **verified within the
controlled original-DLL boundary** when the private full report passes.
Dolphin build provenance of the existing regression captures is not established.

This supplies original80180FB4's type0 dispatch producer and bootstrap801DEFD8
with optional scene801DF834. It reuses unchanged scene_owner/object_pool and
matrix_storage modules. It does not substitute a captured type table or matrix
allocation in edited bootstrap cases. It does not run whole nine-type library
initialization80180444; other dispatch types remain separate.

## Type0 record producer

Original80180FB4 calls80180490, writing a48-byte record at80389E88. Native API
accepts a bounded word-aligned table identity and the source alignment input.
The nine callback words at+00..+20 are80180C20/C30/D64/C6C/D8C/E0C/E6C/F18/FAC.
Word+24=align(32), word+28=align(16), byte+2C=1. Final three bytes remain dirty.
Alignment arithmetic is the original wrapping expression(n-1+a)&-a, including
0 and non-power values in direct producer checks. Usable list compositions
retain the existing power-of-two alignment contract. No callbacks execute
inside the producer; original child functions execute unhooked in the gate.

## Bootstrap publications and failures

All words/pointers below are BEu32/4bytes, flags/index are U8/1byte. The original
r13 is803A9A40. Writer801DEFD8 unless identified otherwise. The stated game/hash,
unknown Dolphin build, procedure/confidence apply to every field.

| Address | Meaning and observed behavior |
| --- | --- |
| r13-6390 /803A36B0 | init byte; nonzero returns80001 with no allocation; attempted init writes1 even on error |
| r13-638C /803A36B4 | view list; published even if first allocation returns0 |
| r13-6388 /803A36B8 | callback list; untouched if first list fails; published0 if second list fails |
| r13-6380 /803A36C0 | optional scene;801DF834 publishes returned identity including0 only for nonzero capacity |
| r13-4EE8 /803A4B58 | viewport stack index byte; published before slot allocation |
| r13-4EEC /803A4B54 | active identity word; set0 after every attempted initialization |

The12-byte descriptor contains BEu32 view/callback capacities, BEu16 optional
scene capacity and U8 stack index/depth. Natural input80288AD8 is
`00000005 00000006 0028 00 0C`:5views,6callbacks,40scene members,index0,depth12.
It is pinned to DOL offset285AD8 and all captures. Native context borrows the
existing list/scene owner, matching matrix context, r13 and geterror service.
All RAM/service/type/alignment/count inputs remain stable; callers own them.

First list uses owner1/type0/F8payload/ordering801DEF40. Second uses owner1/type0/
10payload/ordering801DEE3C. A failed heap request returns exact geterror including0
or highbit. On two successful lists, publish index and allocate its matrix slots.
Nonzero stack error skips optional scene; a stack failure with geterror0 still
continues, matching original. Optional scene uses owner1,type0,callback0,
field0C0,multiplier1,field10=3,field1C=1. Scene heap0 does not propagate as error:
801DF834 returns0 anyway. Scene list failure retains the allocated scene with
list0. No cleanup, rollback or automatic retry is invented. All attempted paths
clear active/set init1; guarded repeats return80001 without reading a descriptor.
Original bootstrap return survives the final error callback's clobbered r3.

Host rejects malformed spans, nonmatching RAM contexts or unsupported layouts;
that is separate from original error semantics and may follow earlier writes.
Invalid matrix index is host rejection, not an original guard. Previously
verified allocation contracts remain applicable. An initialized guard needs
only its flag/error path, without validating unused descriptor/matrix inputs.

## Original-code procedure

Untouched DLL executes producer/registration, bootstrap/optional scene and all
reached list/pool/matrix/scene/memset/identity routines unhooked. Only external
heap8018B7E4/error801AF57C/geterror801AF630 return early. Ordered observations
include arguments/results, complete bounded dirty allocation arena, table bytes,
view/matrix globals and active/index neighbors. Compare actual nonvoid returns
and all24MiB after each phase, excluding exactly these source-accounted stacks:

- Type producer `[80F46FA8,80F47008)`: wrapper32+registration56+LR8.
- Matrix table `[80F46FE8,80F47008)`: open24+LR8.
- Matrix slots and guarded repeat `[80F46FF0,80F47008)`:16+LR8.
- Bootstrap `[80F46F48,80F47008)`: bootstrap16+optional8+scene48+list48+
  layout24+pool-total16+pool-layout24+LR8. Other branches fit this closure union.

Only just-compared stack bytes synchronize before narrower phases. No allocation,
table, global, payload or padding is masked. The full gate uses14/15/16 twice,
401cases/process: baseline captured initialized guard, then400 edited fresh
producer/table/stack setups. Each edited case runs raw and usable type0 producers,
opens a new matrix table and allocates stacks3/1/2, then bootstrap and a guarded
repeat. Cases vary dirty record padding, raw alignments, capacities across255/256,
registry count, index/depth including0, all five heap-failure stages, geterror0/
highbit, nonzero guard bytes and unsigned highbit optional capacities on failing
list heaps.25cases/process use the exact natural descriptor/alignment4/count30.
Eight malformed-host cases are separate. Eleven exact production mutants must
cause RAM/return/service mismatches; build failures, host rejects, crashes and
timeouts never count. Fast uses15once,40edits,no mutants.

This constructs owners and publishes their bootstrap handles within the bounded
proof. Prior view_nodes/view_frustum modules produce records and projection from
these kinds of owners; no complete native entry is wired here. Actual field/HUD
constructors, runtime callback scheduling, other type-table initialization and
live startup remain separate. Existing snapshot launcher is unchanged.

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_view_system.py
```

Private evidence: `W:/Daywater/research-temp/nflstreet-view-system-20261018/`.
