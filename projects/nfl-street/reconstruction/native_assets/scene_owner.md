# Type-zero scene and list owner initialization (row 05 slice)

NFL Street GNNE69 North America rev0, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. New `scene_owner.c/h` reconstructs
`801954E4` and the reached type0 list owner `801805E0`, layout children
`80180388/80180340/80181184`, and type0 initialization `80180C20`. It reuses
`object_pool.c` unchanged. Registration/factory and all shared/live files remain
unchanged. Scene insertion/removal, default/custom callback execution, natural
registry-table allocation/lifetime, teardown and other list types remain outside
this slice.

## Exact evidence

The controlled verification claim applies when the private full report says
`passed`. The untouched ModernGekko DLL executes all the above routines plus
original pool construction/reset/push/alignment and memset `8017BC54`. Only
original heap `8018B7E4` and error `801AF57C` return early with matching controlled
responses. Geterror is not called by this closure. Compare real scene/list/layout
returns, ordered service arguments/responses, complete bounded scene/list
allocation bytes at each service boundary, and all 24 MiB except
`[80F46F58,80F47008)`. The 176-byte stack union includes the eight-byte
`80004830` wrapper frame, scene48, list48, layout24, pool-total16, pool-layout24,
and caller LR slot. No object/global/padding bytes are masked. Error-service
return r3 is clobbered to check that owner returns survive it.

The baseline executes original `80004830` unhooked using the original saved RAM
and controlled heap responses. Its eight arguments are
`(1,0x150,0,0,0,1,0,FFFFFFFF)`. The wrapper is void; its incidental r3 is not
asserted as a return contract. Edited cases call `801954E4` directly and compare
its actual result. Separate direct list cases cover supplied backing storage,
varied element sizes and original `80180388` output stride/total.

The captured dispatch table at `80389E88` has 0x30-byte records. All three pinned
captures agree on the type0 record:

```text
80180C20 80180C30 80180D64 80180C6C 80180D8C 80180E0C
80180E6C 80180F18 80180FAC 00000020 00000010 01000000
```

Layout reads header size at table+24hex (32 bytes), node extra at +28hex
(16 bytes), and initializer identity at +00. Other dispatch entries are retained
as observed identities; their semantics are not reconstructed. The list's node
stride is align(element_size)+node_extra, not a guessed host node layout.
`80180C20` is absent from the cached listings. Its exact 16 bytes from pinned
DOL offset17DC20 and guest address80180C20 in all captures decode as:
`li r0,0; stw r0,0x1c(r3); stw r0,0x18(r3); blr`.
The gate pins those bytes, the DOL SHA1/hash and each capture/table slice. Table
creation is not inferred from captured table bytes.

## Original bytes and behavior

Scene allocation size is 40 + multiplier * registry_count *4. Registry count is
read once from `r13-4F18` before the scene heap request. Callback0 selects stored
identity `80194F04`; no callback is invoked here. The list is constructed with
owner, type, capacity, element_size4, selected callback and supplied_block0.

| Scene offset | BE u32, 4 bytes | Writer / interpretation |
| --- | --- | --- |
| +00 | list identity | scene after list construction, including list0 on heap failure |
| +04 | type | scene argument |
| +08 | callback identity | selected default/custom argument |
| +0C | opaque argument5 | scene argument; no invented meaning |
| +10 | opaque argument7 | scene argument |
| +14 | multiplier | argument6 |
| +18 | heap owner | argument1 |
| +1C | opaque argument8 | scene argument |
| +20 | zero | scene initialization |

Zeroing starts at +24hex for multiplier*registry_count*4 bytes. The final four
allocated bytes remain dirty, including when the length is zero. Scene heap
failure returns0 without error notification. List heap failure still returns
the initialized scene, with list pointer0 and its variable region zeroed; it
adds no invented cleanup, error propagation or failure return.

The list allocates aligned_header + original pool total. Its header stores
+00 type, +04 zero, +08 aligned element size, +0C owner, +10 callback, +14 pool
identity. A supplied pool sits at list+aligned_header with owner0, capacity and
node stride. Pool construction reports error0 before list+14 is written. Type0
init then zeros list+1C and +18; list creation reports error0 again. Allocation
payload, pool padding and list padding are not cleared. These byte meanings are
verified in the bounded initialization comparison; list insertion semantics
remain unmodeled.

## API, host choices and integration

`nflstreet_scene_owner_context` borrows a BE RAM span, type-table identity,
alignment, registry-count snapshot and existing pool heap/error services. Its
alignment/count inputs must match the original globals at the call boundary;
alignment/type descriptor must remain stable through callbacks. It does not
allocate or publish the registry table. Type0/init80180C20 is the only available
dispatch; unsupported types are host rejection, not original validation.

For the `80004830` path call:

```c
nflstreet_scene_owner_create(&context,1,0x150,0,0,0,1,0,0xffffffffu,&scene);
```

Use the returned guest identity as a scene owner only after host success. Inspect
scene+0 before invoking later list consumers, since original list heap failure
still returns a nonzero scene. Callers retain RAM/services and supply real heap
backing for both requests. `nflstreet_scene_list_create` separately exposes the
same type0 list constructor with an optional supplied block; list layout is
available through `nflstreet_scene_list_layout`. No existing scene consumer is
rewired here.

Host safety checks require bounded word-aligned spans, nonwrapping allocation
sizes, power-of-two alignment and header room for the type0 initializer. Raw
layout arithmetic remains 32-bit modular. These are host choices, not checks
attributed to the original. Host rejection returns0 and clears the result;
original heap failure is host success with guest result0. Invalid service
responses may reject after earlier original writes/services; no rollback is
promised. Calls need serialized access and stable descriptors.

## Gate and provenance

Full mode: contexts14/15/16 twice, one saved wrapper case plus400 edited cases
per process, eight production mutants. Fast mode: context15 once,40 edits,no
mutants. Edited cases use dirty bytes, default/custom callbacks, zero/nonzero
array length, capacities across255/256, varied alignment/count/multiplier,
controlled header/node-extra descriptor edits, direct supplied lists, scene and
list heap failure. Complete RAM comparisons include all padding and neighboring
bytes. Eight malformed-host checks are separate from game comparisons. Mutant
build failure, host rejection, crash or timeout never counts as a catch.

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_scene_owner.py
```

Private evidence/builds/logs/mutants stay in
`W:/Daywater/research-temp/nflstreet-scene-owner-20261018/{fast,full}/`.
Dolphin build provenance is not established by these regression captures. No
fresh boot, natural heap lifecycle, callback execution, new emulator integration
or live gameplay claim is made.
