# Direct type-zero list nodes for view/callback setup

NFL Street GNNE69 North America rev0, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Confidence: verified within the
controlled original-DLL boundary only after the private full report passes.
Dolphin build provenance is not established by this gate's existing captures.

The existing `scene_owner.c` already reconstructs list construction801805E0.
This module adds direct allocation80180700 and mode0 publication80180784,
reusing unchanged `object_pool.c`. Unlike scene insertion, these operations
return a payload identity without modifying a scene object's membership byte.
Original view creator801DF14C and callback registration801DF39C call this path.
The view bootstrap801DEFD8 requests lists with payload sizesF8 and10 hex;
those sizes are exercised here, without claiming camera initialization.

Allocation calls original pool allocation801812D4. Exhaustion reports30003 and
returns0; success reports0, clears exactly16 header bytes through80180D64,
clears bit31 and returns node+16. Payload, padding and list headers stay dirty.
Publication subtracts16 from the payload, calls the type0 insertion path and
sets bit31, returning the payload. It never allocates or clears payload data.
Without a comparator it prepends. With a comparator, it inserts before the
first node whose comparison is nonnegative, or appends. Equal keys therefore
place the new entry before the older one. The list count increments modulo32.

All accessed words are BEu32/4 bytes. The raw node header has membership bits
at+00, preserved word+04, previous+08, next+0C; payload starts+10. List header
has type+00, count+04, element size+08, owner+0C, comparator+10, pool+14,
head+18 and tail+1C. Existing owner/pool documents record their initialization.
Callbacks/link writers:80180700/80180D64 allocate;80180784/80180C6C/80180B94
and80180B3C/5C/7C publish. Original callback-mode1 duplicate suppression,
unlink/free/teardown and arbitrary dispatch types are outside this API.

The context borrows bounded RAM, type-table identity, error service and an
optional pure comparator. Host status is distinct from the original guest
result, including exhaustion0. Callers retain all memory/services. Nodes must
be uniquely allocated, unlinked and disjoint from table/list/pool metadata.
Alignment, bounded spans, supported type0 dispatch and stable service inputs
are host requirements, not recovered game guards. Host rejection can follow
prior writes/services; there is no rollback. Cyclic malformed lists reject.
The test uses the existing default scene comparator as an integration fixture;
actual view/callback ordering functions remain a separate source/proof task.

The untouched original DLL executes constructor, allocation, publication,
memset, type0 link helpers, pool operations and default comparator unhooked.
Only heap8018B7E4 and error801AF57C return early. Error r3 is deliberately
clobbered to test retained returns. Ordered service observations include the
entire bounded dirty list arena and synthetic object arena. Actual nonvoid
returns and all24MiB compare after each phase, excluding exactly:

- Creation `[80F46F90,80F47008)`: list48 + layout24 + total16 + pool-layout24
  plus caller LR8; alternative pool/reset paths fit inside that union.
- Allocation `[80F46FD8,80F47008)`: allocator24 + pool allocator16 + caller LR8.
- Publication `[80F46FC0,80F47008)`: publisher16 + insertion32 + link16 + LR8.

After a comparison, only that phase's stack interval is synchronized before
the narrower next phase. No global, list, node, payload or padding is masked.
Controlled identical caller writes populate payload object identities and
synthetic ordering keys; these are fixture inputs, not camera data discoveries.
Type0 dispatch table is pinned from all captures; initialization provenance
of that table is still outside this slice. Missing leaf initializer/comparator
code is pinned to exact DOL and capture bytes, as in the prior scene gates.

Full gate: captures14/15/16 twice,401cases/process. Saved baseline directly
allocates from the captured view list; edited cases construct fresh lists,
use F8/10 payload sizes, capacities across255/256, dirty backing, varied
alignment, heap0, exhaustion, empty/head/middle/tail insertions, ties,
count wrap and comparator0. Eight malformed-host checks are separate.
Ten exact production mutants must be caught by RAM/return/trace differences;
build failures, host rejects, crashes and timeouts do not count. Fast uses
capture15 once,40edits and no mutants. No emulator, fresh-match or live claim.

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_list_nodes.py
```

Private evidence/builds/logs:
`W:/Daywater/research-temp/nflstreet-list-nodes-20261018/{fast,full}/`.
