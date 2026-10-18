# Type-zero scene insertion (row 05 slice)

NFL Street GNNE69 North America rev0, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. New `scene_insert.c/h` reconstructs
`8019573C`, factory+insert `801957A4`, reached type0 node/list insertion and
default comparator `80194F04`. Registry, factory, scene and pool modules remain
unchanged. Removal `8019585C`, reordering `801958E8`, teardown and the full row05
join remain deferred; this slice does not declare row05 complete.

## Source and verification boundary

The verified claim applies when private `full/verification.json` says `passed`.
The untouched ModernGekko DLL executes insertion, factory+insert, node allocation
`80180700`, insertion dispatch `80180784`, prefix init `80180D64`, sorted scan
`80180C6C`, link helpers `80180B94/80180B3C/80180B5C/80180B7C`, default comparator,
pool and memset children unhooked. Original setup/composition routines also run
unhooked. Only explicit heap/error/geterror, constructor `800E1F7C`, and custom
comparator identity `80194FE4` return early with controlled matching responses.
The custom identity is a test service, not that routine's reconstructed behavior.
Default comparator is **never** intercepted or replaced with a native stand-in.

Compare real returns, exact node/list/object bytes, links/order, byte2 wrap,
ordered external service arguments/responses and complete bounded list/scene
observations, plus all24MiB except accounted stack intervals. Setup/composition
uses `[80F46F60,80F47008)` (168bytes): deepest direct scene/list/layout/pool-total/
pool-layout chain plus caller LR slot. Standalone insertion uses
`[80F46FB0,80F47008)` (88bytes): insert16 + dispatch16 + sorted32 + link16 plus
caller LR slot. After checking setup, only its declared stack is synchronized
before narrower insertion checks. No table, object, node, padding or global is
masked. Composition retains its larger stack union across component calls.

The saved-input baseline derives an existing object from captured registry type0
pool+14hex, constructs a new scene with explicit controlled heap responses on
otherwise saved RAM, then inserts that object. This is not a naturally captured
scene-allocation outcome or a fresh boot. Edited cases use dirty scratch data.
The gate pins all source/input/ABI hashes and fresh-process repeatability.

Default comparator cached listings are absent. Its exact52bytes at pinned DOL
offset191F04 and guest80194F04 in all captures decode to loading each payload's
object pointer, reading BE u16 object+10hex, then returning0 when equal, -1 when
first<second, +1 when first>second. Keys are unsigned16; the scan treats the
returned32-bit comparator value as signed. The report stores raw bytes and DOL/
capture hashes. Type0's table at80389E88 is pinned in all captures: node init
identity+08=`80180D64`, insert identity+0C=`80180C6C`, node prefix+28hex=16. Type0
list init80180C20 is independently pinned from DOL/captures as in the scene gate.
Other dispatch types and their meanings remain unavailable.

## Original insertion bytes and order

`80180700` allocates from list+14hex pool, clears only the16-byte node prefix,
clears prefix bit31 and returns payload=node+16. Scene insertion stores the
object identity in payload+0, then inserts the node. Node bytes are:

| Offset | Type / size | Writer / meaning within verified insertion |
| --- | --- | --- |
| +00 | BE u32 /4 | prefix initializer0; dispatch sets bit31 after linking |
| +04 | BE u32 /4 | prefix initializer0; no further meaning inferred |
| +08 | BE u32 pointer /4 | previous nodebase, or0 at head |
| +0C | BE u32 pointer /4 | next nodebase, or0 at tail |
| +10 | BE u32 pointer /4 | scene insertion's object identity |

List+18/+1C hold head/tail **nodebase** identities, not payload pointers. List+04
increments once. Payload alignment padding remains dirty. With comparator0 the
new node goes before the head. Otherwise the type0 scan advances while signed
comparison(existing_payload,new_payload)<0. It stops on zero or positive, so a
new equal-key node goes **before** existing equal-key nodes. Empty, head, tail
and middle insertion update exact forward/back links; markbit31 is set after
linking. Scene insertion increments object byte2 modulo256 after insertion.
This verifies the counter operation; no broader object schema is inferred.

On node exhaustion, pool allocation reports error30003 and returns0. Scene
insertion calls geterror and returns its exact code (including0/high-bit), with
no payload/link/count/byte2 write. Factory+insert first runs the unchanged actor
factory, attempts insertion only for nonzero object, then returns that object
regardless of insertion's guest error. If the factory exhausts it returns0 and
never dereferences the scene argument; the gate exercises this with scene0.
Constructor service can set order and byte2 before insertion; its returned r3
is deliberately clobbered and ignored.

## API and host contract

`nflstreet_scene_insert_context` borrows bounded BE RAM, type-table identity,
pool/error/geterror services and an optional pure custom-comparator service.
Call `nflstreet_scene_insert(&context,scene,object,&code)` for8019573C. Host
success and guest code are separate; code0 can mean geterror0 on exhaustion.
Inspect actual list/counter state when that distinction matters.

For801957A4, call `nflstreet_scene_create_insert(&insert_context,&registered_context,
scratch,scratch_count,scene,type,header,input,&object)`. It composes unchanged
`nflstreet_registered_create` with insertion and preserves the original object
result even when insertion fails. Both contexts must share RAM. Scratch follows
the existing factory adapter contract. The wrapper adds no rollback or cleanup.

`nflstreet_scene_default_compare(ram,left_payload,right_payload,&signed_result)`
exposes the bounded native default comparator. A non-default nonzero comparator
identity requires the explicit pure callback; returned INT_MIN/MAX/zero are
handled by signed32 comparison. No guest identity is cast to a host function.

Host choices require type0 descriptor identities/prefix16, bounded/aligned
accesses, nonoverlapping nodes/objects/list/table, valid links and free slots
separate from live nodes. Scene/list/table/pool/links must stay stable during
heap/error/custom-comparator services. Custom comparators do not mutate RAM.
Host traversal is bounded against malformed cycles. These restrictions are not
validation attributed to the original. Host rejection returns0/clears guest
result, possibly after earlier allocation/services; no free/rollback is invented.

## Gate and integration

Full mode: contexts14/15/16 twice, one saved-input setup plus400 edited cases
per process, nine production mutants. Insertion fixtures use alignments4/8/16/32/64
to preserve word-aligned node accesses. Fast mode: context15 once,40 edits,no
mutants. Cases cover capacities across255/256, dirty prefix/payload padding,
empty/head/middle/tail/equal insertion, default key0/7FFF/8000/FFFF boundaries,
custom INT_MIN/MAX/zero responses, comparator0, duplicate object identities,
byte2 FF->00, exhaustion/geterror0/high-bit and direct comparator returns.
Every16th edit composes fresh registry init -> register -> scene -> factory+insert,
including too-small node pools, returned objects on insertion failure and later
factory exhaustion. Seven malformed-host checks are separate. Mutants target
key signedness, signed scan/ties, prefix clearing, back links, markbit, byte2,
ignored insertion status and exhaustion error. Build/crash/host rejection never
counts as a caught mutant.

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_scene_insert.py
```

Integrate only after a supported type0 scene/list has been constructed and an
object has been registered/allocated in the same RAM. Call insertion explicitly,
or the factory+insert wrapper when replacing801957A4; retain all borrowed spans
and services. No shared/live consumer is rewired by this slice. Private outputs:
`W:/Daywater/research-temp/nflstreet-scene-insert-20261018/{fast,full}/`.
Dolphin build provenance is not established by the regression captures. No
natural heap implementation, class implementation, fresh-match or emulator/live
integration claim is made.
