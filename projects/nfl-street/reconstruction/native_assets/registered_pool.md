# Registered pool and existing factory join (row 05 slice)

GNNE69 North America revision 0, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. This slice reconstructs registration
`80195400` and connects its BE guest records through the existing unchanged
`actor_factory.c` to the previously verified pool kernel. It borrows an explicitly
provided registry table and RAM. Natural registry-table allocation/lifecycle,
scene owner `801954E4`, insertion `801957A4/801958E8`, destructors and game class
constructors remain deferred.

## Evidence boundary

The claim is verified within the controlled original-DLL boundary when the
private full report has status `passed`. The untouched DLL executes registration,
factory `801956A4`, the seven pool routines and original clearing `8017BC54`.
Only external heap `8018B7E4`, error `801AF57C`, geterror `801AF630` and supplied
constructor identities `800E1F7C/800D2E94` return early with controlled matching
responses. The constructors are service identities here, not reconstructed
classes. Their fixture effects dirty selected payload bytes. Original returns,
ordered service arguments/responses, exact registry and constructor-entry object
bytes, and all 24 MiB compare except `[80F46FB8,80F47008)`. This 80-byte stack
interval covers registration's 16-byte frame, constructor's 32-byte frame,
layout's 24-byte frame and caller LR slot. No object, padding, table, global or
heap bytes are masked. Constructor/error service return registers are deliberately
clobbered; registry error and factory object returns must survive them.

The registry root is BE u32 at `r13-4F14`. Records have 20-byte stride:

| Offset | Type / size | Meaning / writer / reader |
| --- | --- | --- |
| +00 | BE u32 / 4 | object size; registration writes, factory reloads after allocation |
| +04 | BE u32 / 4 | capacity; registration writes and passes to pool construction |
| +08 | BE u32 code identity / 4 | constructor; registration writes, factory reloads after clearing |
| +0C | BE u32 code identity / 4 | destructor; registration stores, no call in this slice |
| +10 | BE u32 guest pointer / 4 | pool; registration publishes after construction, factory reads |

Registration stores size, constructor, destructor, capacity before requesting
`pool_construct(owner1,capacity,size,supplied0)`. On success the pool constructor
reports error0 before the registry pool pointer is published; registration then
reports error0 again and returns0. On heap failure it publishes pool0, calls
geterror, reports that exact code and returns it, including zero or high-bit
codes. It performs no invented cleanup or rollback.

Factory allocates from the selected pool, rereads registry root/size, clears
that size, writes byte0 truncated type, byte1 header, BE u16 +10 equal2, rereads
constructor and optionally calls it with object/input. Constructor return is
ignored. The bridge refreshes the same host scratch record after the allocation
error callback, honoring source-point changes to registry root, size and
constructor. The gate edits these fields at that boundary and observes complete
object initialization before constructor entry. Registry root/current record
cannot overlap cleared object bytes under the documented host contract; without
that contract, clear itself could alter the subsequent constructor lookup.

## Validation and provenance

Full mode runs contexts14/15/16 twice, each with one saved-as-is exhaustion case
and 512 edited cases. The saved case derives type-zero pool from captured root,
checking capacity14, stride1A90, freecount0 and alignment4. Edited scratch tables
and pools are dirty; cases cover every header byte, type truncation through
record511, null/non-null constructors, varying size/capacity, one/two-byte pool
indices, repeated allocation, exhaustion, failed heap and geterror propagation,
relative alignment and allocation-boundary root/size/constructor reloads.
Registration always supplies block0 as the original does; successful heap
responses name bounded word-aligned backing RAM. The earlier pool-kernel gate
separately covers caller-supplied blocks. Nine host-only malformed-input checks
are distinct from the original comparisons. Eight production mutants target
registry fields, heap owner, publication, failure propagation, size/constructor
reloads and constructor input. Build failures/crashes/host rejection never count
as catches. Fast mode uses context15 once,64 edits, no mutants.

Position-neutral captures are regression inputs, not fresh boot evidence;
Dolphin build provenance is not established by this gate. No emulator, natural
registry lifecycle, real class construction, cleanup, fresh match or live claim.

## API and integration

`nflstreet_registered_context` carries bounded BE RAM, registry-root slot,
record count, relative alignment and explicit services. Call
`nflstreet_registered_register(context,type,size,capacity,ctor,dtor,&code)` on a
prepared table. On host success, code is the original registration return; heap
failure can also return code0. Inspect the published pool if success matters.

For factory calls, provide an array of `nflstreet_factory_record` with at least
context.record_count entries as ephemeral host scratch, then call
`nflstreet_registered_create(context,scratch,count,type,header,input,&object)`.
The adapter uses unchanged `nflstreet_actor_create`, maps allocation results to
bounded guest memory, and passes constructor service the original function,
object and input identities. Only scratch[type] is touched; ephemeral callback
pointers are cleared before return. Host success/object0 means original
exhaustion; host rejection returns0 and clears output. No separate allocator
rewrite or shared consumer change is needed. Callers retain RAM/table/services;
this API never allocates the registry table or frees game pools.

Host safety requires valid root/whole table, bounded type, nonwrapping pool
layout, power-of-two relative alignment, object size at least0x12, bounded
callback responses and disjoint root/selected records versus cleared objects.
These checks are host choices, not validation recovered from the original.
A callback can cause a later host rejection after prior writes/services; no
rollback is promised. Guest constructor identities are never cast to host
function pointers. API calls need externally serialized access to their spans.

Gate:

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_registered_pool.py
```

Private evidence/builds/logs/mutants:
`W:/Daywater/research-temp/nflstreet-registered-pool-20261018/{fast,full}/`.
