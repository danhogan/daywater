# Default type-zero scene removal and reordering (row 05 slice)

NFL Street GNNE69 North America rev0, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. `scene_remove.c/h` reconstructs the
scene+0C=0 path of removal8019585C and reorder801958E8, its default search,
unlink and release children. All previously verified adapters remain unchanged.
The supported search path is the one used by the known80004830 scene creation.
Nonzero scene+0C is a **callback identity**, not a boolean: it routes through
80180AAC/80180F18. That callback search path remains unavailable and receives
host rejection; no behavior is invented for it.

## Evidence and exact boundary

The controlled verification claim applies when private `full/verification.json`
has status `passed`. Untouched ModernGekko original routines execute search
80180A14, mode0 dispatch80180D8C, scan80180E6C, predicate80195344,
unlink80180E0C, dispatch801803F0, release801808EC, quotient/push80181334 and
all reused registry/pool/factory/scene/insertion children. Default comparator
also executes original code unhooked. Only declared external heap/error/geterror
and constructor800E1F7C return early with controlled matching responses.

Scene removal, reorder, unlink and release entrypoints are void: **no original
r3 return contract is asserted** for them. Native int returns are host status.
Original nonvoid search status, mutated target word, allocation/index-pop returns
and reused setup returns are compared. Ordered external arguments/responses and
exact scene/list/object/table/pool observations compare, together with every
byte of24MiB except precise phase stacks. Only the declared setup stack is
synchronized before narrower standalone operations:

| Phase | Excluded guest interval | Accounting including caller LR slot |
| --- | --- | --- |
| setup/composition | 80F46F60..80F47008 | 168bytes: scene/list/layout/pool-total/pool-layout chain |
| remove | 80F46FA8..80F47008 | 96bytes: remove24 + search32 + scan32 + caller8 |
| reorder | 80F46F90..80F47008 | 120bytes: wrapper24 + removal chain88 + caller8 |
| find | 80F46FC0..80F47008 | 72bytes: search32 + scan32 + caller8 |
| released-slot insert | 80F46FB0..80F47008 | 88bytes: insert16 + dispatch16 + sorted32 + link16 + caller8 |
| direct pool allocation | 80F46FF0..80F47008 | 24bytes: allocate16 + caller8 |
| unmarked node release | 80F46FE8..80F47008 | 32bytes: release16 + quotient8 + caller8 |
| raw quotient/index-pop | 80F46FF8..80F47008 | 16bytes: quotient8 + caller8; pop is leaf |

No game object, table, node, payload, neighboring byte or global is masked.
Composition retains its already-accounted setup union across component calls.

Missing cached leaves are decoded from the pinned DOL and crosschecked byte for
byte in all three captures. Gate reports exact raw bytes and offsets:

| Original entry | DOL offset / size | Relevant decoded behavior |
| --- | --- | --- |
| 80180D8C | 17DD8C /128 | mode0 returns list+18 head; other modes are not exposed here |
| 80180E0C | 17DE0C /96 | updates head/tail if node matches, next.prev, prev.next, then count-1; node links untouched |
| 80195344 | 192344 /36 | compares payload.object to target; on equality stores payload identity to target and returns0; otherwise1 |

Existing type0 table, init and comparator evidence is pinned again. The baseline
is a controlled fixture on saved input RAM, not a naturally captured removal or
fresh boot state. No missing-actor behavior is inferred from that baseline.

## Search and removal behavior

Default scene search starts at list head and walks next pointers. The predicate
uses a temporary in/out word initialized to the requested object identity. First
matching payload replaces that word. Search80180A14 returns1 for empty list,
0 for absence and4 for a found match. The default predicate remains internal
original code, not a controlled service. Safe child-search absence is verified;
original scene removal ignores search status and assumes the word now names a
valid payload. Therefore the host removal API requires present membership and
rejects absence separately instead of inventing an original safe failure path.

Node release subtracts the16-byte prefix. If its flags word is negative (bit31),
unlink updates head, tail, next.prev, prev.next, then decrements list count.
After unlink it clears only bit31. The freed node's own previous/next pointers,
payload object and other bytes remain stale. If the flag is already clear,
release skips unlink and still returns the index to the list pool.

Raw pool release computes unsigned32 `(node-data_base)/stride`, then calls the
unchanged push routine. Subtraction wraps; quotient is truncated to u8 when
capacity<=255 or u16 otherwise. This helper only performs arithmetic on the node
token, never dereferences it. Its synthetic tests use bounded headers/index
storage for quotients65535/65536/65537 and wrapping subtraction, without huge
allocations or pretending those raw tokens are valid scene members.

Scene removal decrements object byte2 modulo256 after node release. A present
actor with byte2=0 remains a valid input and becomesFF. Duplicate object
memberships are allowed; search removes the first matching payload in forward
list order. Reordering performs removal, stores the low16bits of its order input
as BE u16 at object+10, then calls unchanged insertion. Reorder's insertion
error is ignored by the original void entrypoint; native host status does not
reinterpret it as an original return. Ordering matches original operations even
when duplicates share one object's mutable key.

Scene removal releases list-node storage only. It does not release actor storage,
run class destructors or clear actor payload. The composed gate pins actor pool
header/free-index bytes across removals and confirms the actor factory remains
exhausted afterward.

## API and integration

Use existing `nflstreet_scene_insert_context` with the same RAM/type table and
services. Call `nflstreet_scene_remove(&context,scene,object)` for an existing
member, or `nflstreet_scene_reorder(&context,scene,object,order)` to remove/update/
reinsert. These int results are host status only; original entries are void.
Scene+0C must0. Unsupported callback identity, malformed spans or absent member
return host0; no validation is attributed to the original game. Prior writes or
services may remain on a later host rejection; no rollback is promised.

`nflstreet_scene_find` exposes safe default search status/target mutation.
`nflstreet_scene_match` exposes the built-in predicate's host-word adaptation.
`nflstreet_scene_node_release` is for a valid owned node payload: marked nodes
must have valid live links; unmarked nodes must already be unlinked.
`nflstreet_scene_pool_release` is the separate raw arithmetic helper and does
not require its token to be a mapped node address. Nonzero stride and bounded
accessed pool/index bytes are host safety requirements, not original checks.

The stable/disjoint RAM/service contract from insertion remains in force;
malformed cycles are bounded host rejection. Callers retain RAM and services.
No shared/live consumer is rewired. Arbitrary callback search, other list types,
actual class constructors, natural heap behavior and teardown stay separate.
This slice supplies evidence for the known default type0 startup path; broader
row05 completeness requires manager assessment.

## Gate

Full mode: contexts14/15/16 twice, one controlled saved-input setup plus400 edits
per process, nine production mutants. Fast mode: context15 once,40 edits,no
mutants. Cases exercise all link positions, last node, ties/u16 order boundaries,
low16 truncation, repeated reorder, duplicate memberships, decrement wrap,
stale node bytes, release/reallocate LIFO, unmarked release, raw quotient width,
safe absent/empty search and eight separate malformed-host cases. Every16th edit
joins init->register->scene->factory-insert->reorder->remove, then proves actor
allocation still exhausted. Build failures/crashes/host rejection never count
as mutant kills.

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_scene_remove.py
```

Private evidence/builds/logs/mutants:
`W:/Daywater/research-temp/nflstreet-scene-remove-20261018/{fast,full}/`.
Dolphin build provenance is not established by these regression captures. No
fresh match, natural heap/class implementation or new emulator/live claim.
