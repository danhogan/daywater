# Palette-pool owner (row 06, remaining storage slice)

This supplies new storage and reservations for model matrix palettes. Together
with `matrix_storage.c`, it covers the two storage owners in row06. It does not
construct models/cameras, calculate palette transforms, or change live startup.

NFL Street, GameCube GNNE69 North America rev0, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Evidence level is **verified within
the controlled original-DLL boundary** only when the private full report passes.
The existing position-neutral regression captures have no established Dolphin
build provenance in this gate. No fresh boot or new emulator/live claim.

## Original behavior

`801CA3F4` calls wrapper `8018B7B8` for capacity*multiplier*48 bytes. The wrapper
calls heap `8018B7E4` with owner0 and flags0. Creator publishes the returned
identity into both roots. Only multiplier2 makes the second root differ:
root1=base+capacity*48. Other multipliers, including0/3, leave roots identical;
this is not an inferred general N-buffer API. Creator stores capacity, then
calls `801CA494` to zero the cursor and select `(previous_active==0)`.

The creator has **no heap-failure branch**. Heap0 still publishes roots/capacity
and flips/reset state. With multiplier2, root1 can therefore be nonzero even
when root0 is0. No error notification, cleanup or payload clearing is invented.
Repeated creation does not free the earlier allocation. The original caller
`80155C40` supplies `(0x1E78,1)`; the gate exercises that exact component call,
not the full bootstrap function and its unrelated children.

`801CA494` resets only cursor and active selector. It does not clear matrices.
`801CA4B0` computes rounded=(requested+1)&FFFFFFFE with wrapping32-bit arithmetic,
publishes cursor+rounded, then writes descriptor+8=rounded and descriptor+0=
root[active]+old_cursor*48. Descriptor+4 remains untouched. Capacity is not
checked. Raw cursor multiplication, pointer addition, count rounding and cursor
advance all preserve the original modular arithmetic; these identities need
not point to mapped RAM. Actual palette transform writers remain separate.

| Field | Primitive / size / endian | Writer/reader and meaning |
| --- | --- | --- |
| r13-5680 /-567C | u32 pointer /4 each /BE | creator roots; reservation reads selected root |
| r13-5678 | u32 /4 /BE | active selector; flip maps old0 to1, all other values to0 |
| r13-5674 | u32 /4 /BE | configured capacity, stored by creator; reserve does not inspect it |
| r13-5670 | u32 /4 /BE | cursor in48-byte records; flip zeroes, reserve advances |
| descriptor+00 | u32 pointer /4 /BE | reservation's data identity |
| descriptor+04 | u32 /4 /BE | opaque preserved word; no meaning inferred here |
| descriptor+08 | u32 /4 /BE | rounded reserved record count |

These fields and sizes are verified only within the procedure below. The test's
scratch arena and heap replies are explicit harness choices, not discoveries
of actual heap outcomes. Source revision, unknown Dolphin provenance, procedure
and observed patterns apply to every field in this table.

## Native API

`nflstreet_palette_pool_context` borrows bounded BE RAM, a globals address
(source r13-5680) and a heap service. Call `nflstreet_palette_pool_create` with
the evidenced capacity/multiplier, `nflstreet_palette_pool_flip` for buffer/frame
reuse, and `nflstreet_palette_pool_reserve` for a descriptor. Original entries
are void; each native return is **host status only**, not an invented guest
success/error return. Successful host execution can still publish heap0.

`nflstreet_palette_pool_span` is a host convenience with no original counterpart:
it bounds-checks descriptor.data/count*48 against the RAM span and returns the
byte span plus length. This validates mapped RAM, not allocation ownership,
configured capacity, alignment for a future writer, or descriptor lifetime.
Callers retain all RAM/services and ensure usable backing when consuming data.
The source-compatible raw reservation is intentionally distinct from that check.

Host safety requires bounded aligned accessed globals/descriptors, stable RAM
and service lifetime, and a nonwrapping supported creator allocation size.
Reservation accepts active0/1 as a host contract; the original can read other
table-relative words for unusual selectors and does not perform this check.
Flip itself accepts every32-bit selector and maps it to0/1. Unsupported inputs
are host rejection, not original validation. No teardown or heap implementation
is supplied. Existing shared consumers and live files are unchanged.

## Integration gate

The untouched original DLL executes creator/flip/reserve unhooked.
Only public heap API `8018B7B8` returns early with controlled size/flags/results.
Its owner0 mapping to `8018B7E4` is established by the pinned assembly, not by
executing that wrapper in this gate. Native service traces normalize that
source-derived mapping to owner0. The DLL dispatch combines the wrapper and
allocator, so intercepting only the nested allocator entry misses the boundary;
the heap API wrapper is explicitly outside this proof.
Ordered heap observations include global, descriptor and bounded arena bytes.
After each game operation, compare all24MiB outside exactly
`[80F46FF0,80F47008)`: creator's16-byte frame + caller LR slot8.
Leaf flip/reserve use no additional stack. No global/descriptor/payload bytes
are masked. Void entries have no incidental r3 return assertions.

Full mode: contexts14/15/16 twice, one saved-state flip plus400 edited cases per
process. At most three independent processes run concurrently; stable-order
reporting and same-context repeat equality remain mandatory. Cases cover dirty
storage, heap0/unmapped identity publication, multiplier0/1/2/3, arbitrary old
selector values for flip, odd/even/zero/wrapping reservations, raw cursor wrap,
opaque descriptor preservation, repeated creation and25 caller-based startup
calls/process. Controlled caller payload writes are identical fixture effects,
not a claim to reproduce transform writers. The host span helper is checked
separately; eight malformed-host cases are separate from original invocations.

Eleven production mutants target heap owner/request, zero-response publication,
second-root offset, flip/index/cursor state, count rounding, cursor advance,
matrix stride, preserved descriptor word and accidental payload clearing.
Only actual RAM/service-trace discrepancies count; build errors, crashes,
timeouts and host rejection never count as mutant kills. Fast mode uses
context15 once,40 edits and no mutants.

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_palette_pool.py
```

Private builds, logs and evidence remain at
`W:/Daywater/research-temp/nflstreet-palette-pool-20261018/{fast,full}/`.
