# Bounded original pool kernel (row 05 slice)

GNNE69 North America revision 0, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. This isolated kernel supplies the
pool underneath the existing `actor_factory.c` allocator callback. Registry,
scene/list insertion and actor construction remain separate, deferred owners.
No shared consumer is changed.

## Evidence and scope

The verification claim applies when `full/verification.json` has status
`passed`: untouched ModernGekko original DLL routines execute on the same 24 MiB
prepared RAM as production `object_pool.c`. Original heap `8018B7E4` and error
`801AF57C` return early with explicit controlled responses; no original pool
routine is intercepted. Compare real returns, layout output words, ordered
heap/error arguments and responses, and every RAM byte except
`[80F46FC8,80F47008)`. This 64-byte stack interval is the constructor's deepest
layout frame at initial SP `80F47000` through the caller LR slot; no global,
pool, heap, data or padding bytes are masked. Reset/push are original void
functions; native success is a host status, not their original return.

Verified within that controlled boundary: alignment arithmetic `80180328`,
layout `80181040`, construction `801811F0`, reset `80181128`, push `801810EC`,
pop `801810A4`, allocation `801812D4`. Exact assembly establishes these BE
fields relative to the explicitly supplied pool identity:

| Offset | Type / size | Interpretation | Readers / writers |
| --- | --- | --- | --- |
| +00 | BE u32 / 4 | capacity; selects one-byte indices through 255, two-byte indices from 256 | construct; layout/reset/push/pop |
| +04 | BE u32 / 4 | allocation stride | construct; allocate |
| +08 | BE u32 / 4 | number of free indices | reset/push/pop |
| +0C | BE u32 / 4 | passed heap owner token | construct |
| +10 | BE u32 guest pointer / 4 | free-index bytes at block +24 | construct; reset/push/pop |
| +14 | BE u32 guest pointer / 4 | data base at block +aligned(24+capacity*width) | construct; allocate |

Reset pushes indices 0 through capacity-1, producing descending LIFO returns.
Push truncates values to u8/u16; it neither validates index nor capacity. Pop
returns `FFFFFFFF` when empty. Allocation returns data+index*stride (32-bit
arithmetic), reports error 0 on success or `30003` with return 0 on exhaustion.
Construction requests heap(owner,total,0) only without a supplied block; heap
zero returns zero without error notification. Successful construction reports
error 0. Payload and alignment padding remain dirty. The global alignment
source is BE u32 at `r13-66A8`, read by the original alignment helper.

The saved-as-is test derives the registry through `r13-4F14` and the type-zero
pool through record +10 (hex). Each capture pins observed alignment 4,
capacity14, stride `1A90`, freecount0, then compares pop/allocation exhaustion
without changing input RAM. Pool address is reported from each capture, not
chosen as an allocator outcome. Dolphin build provenance is not established
by this gate; the supplied position-neutral captures are regression states,
not fresh boot evidence. No emulator integration or natural heap lifecycle
claim is made.

Edited cases reconstruct dirty scratch blocks at explicit guest identities,
including blocks word-aligned but offset from global alignment. Capacities
0,1,2,3,14,254,255,256,257,511,1024,65535,65536 cover empty, width and u16
boundaries. Cases compare raw alignment/layout wrap, supplied versus heap
allocation/failure, LIFO draining, duplicates/truncated/overcapacity pushes,
allocation results, exhaustion and repeated reset. Large pools sample pops;
width-boundary pools drain fully. Full mode uses 390 edited cases plus one
saved case in contexts14/15/16 twice. Eight production mutants cover width,
alignment, offsets, reset order, push/pop counts, stride and error code.
Build failure, host rejection, crash, timeout or oracle exception never counts
as a caught mutant. Fast mode uses context15 once,39 edits and no mutants.

## Native API and host choices

`object_pool.h` exposes a bounded BE RAM span (`bytes,size,base`), explicit
pool addresses, relative alignment and service callbacks. Host success/failure
is separate from original guest return values. Raw layout/alignment preserve
32-bit modular arithmetic, including unusual alignment masks. Construction
preflights nonwrapping size and uses a nonzero power-of-two alignment as a host
contract. It requires only word alignment of block identity; alignment affects
the relative data offset. Header reads and index accesses must fit the span,
halfword slots must be aligned and index bytes cannot overlap the header.
These are host memory-safety choices, not recovered original validation.
Malformed host inputs are checked separately from original comparisons.

Integration: retain the existing factory allocator signature. Its adapter can
call `nflstreet_pool_allocate(ram, owner, &services, &guest_result)` where
`owner` is the original pool identity from an explicitly owned registry.
On host success and nonzero guest result, return a factory memory descriptor
whose token is that address and whose bytes/size bound the caller's object
stride in the same RAM span. Do not call the factory if the host rejects or
the original allocation result is zero. The factory continues to own clearing,
header writes and construction. Before that adapter is useful, a separate
registry owner must call `nflstreet_pool_construct` with capacity/stride/owner,
a supplied block or heap callback, alignment and error callback; it must retain
RAM and service lifetimes. This slice provides no registry adapter or cleanup.

Gate:

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_object_pool.py
```

Private builds, logs, mutated source copies and evidence remain under
`W:/Daywater/research-temp/nflstreet-object-pool-20261018/{fast,full}/`.
No saved RAM, asset, executable or credentials are added to the repository.
