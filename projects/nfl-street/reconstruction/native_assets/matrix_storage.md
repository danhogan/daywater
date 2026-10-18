# Fresh matrix storage (row 06, stack-storage slice)

This replaces the captured matrix table and slot allocations with newly
allocated storage, then connects the existing matrix operations to it.
Cameras and model constructors need this storage before they can be created
without saved RAM. Palette-pool creation behind `801CA4B0` is a separate owner.

NFL Street, GNNE69 North America rev0, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Evidence level is **verified within
the controlled original-DLL boundary** only when the private full report passes.
Dolphin build provenance is unavailable for the existing regression captures;
they are not fresh boot captures. No emulator/live or complete match claim.

## What is reconstructed

`matrix_storage.c/h` implements table initialization `8018908C`, lookup
`80189068`, and per-stack allocation `801891E8`. Reached `80189664` is exactly
`blr` in the cached disassembly; no initialization is invented for it.
The source caller `80155DA8` supplies the concrete matrix-only startup sequence:
initialize with argument4, then allocate stacks3,1,2 with twelve slots each.
The gate reproduces that component sequence; it does not execute the complete
bootstrap routine or its unrelated subsystems.

The table initializer adds eight entries to its argument and requests
`heap(1,(argument+8)*12,0)`. Every word of the new table is zeroed. It publishes
the table address immediately after heap return, including table0 on failure.
Failure obtains the error code and leaves count/flag/current unchanged. Success
publishes count and flag1. An already nonzero flag reports `50001` and skips the
heap. Every branch calls the error service and returns the saved error code,
regardless of the service's incidental return register. Current stack is never
reset by table initialization.

Per-stack allocation requests `heap(1,slot_count*64,0)`. Success stores slot
count, current depth0 and the returned slot identity, then initializes only the
first 4x4 matrix. The other slots remain dirty. Failure leaves all three entry
words unchanged, obtains/reports the error and returns it, including error0.
Replacing an existing entry does not free its previous allocation. Slot count0
with a nonzero heap result still writes the first64 bytes; the bounded backing
span must cover that write even though the original request size is0.

| Field | Primitive / size / endian | Interpretation and source |
| --- | --- | --- |
| r13-65F4 | u8 /1 | initialized flag; read/written by8018908C |
| r13-65F0 | u32 pointer /4 /BE | table identity; published even on heap failure |
| r13-65EC | u32 /4 /BE | table entry count including eight reserved entries |
| r13-65E8 | u32 pointer /4 /BE | selected entry; existing801893C8 changes it |
| table+i*12 +00 | u32 /4 /BE | allocated slot count; also tested as nonzero by8018939C |
| table+i*12 +04 | u32 /4 /BE | current slot depth; existing push/pop/reset update it |
| table+i*12 +08 | u32 pointer /4 /BE | contiguous64-byte matrix slots |
| 80261800 /80261804 | IEEE f32 /4 /BE | source identity zero/one; captured values00000000/3F800000 |

The API's explicit slot identities come from those original globals/constants;
scratch addresses used by the test harness are controlled service outcomes,
not discoveries of a natural heap layout. The fields above are verified only
by the procedure below, not by fresh game-state sampling.

## Connection to existing matrix operations

After `nflstreet_matrix_storage_open` and the required
`nflstreet_matrix_storage_allocate` calls, use
`nflstreet_matrix_storage_bind(context,&view)` to refresh the existing
`nflstreet_matrix_stacks` table/count/current fields. Caller-provided viewport
index, sine and angle constants remain intact. The existing `matrix_stack.c`,
`draw_engine.c` and all shared headers are unchanged.

The gate drives existing select/top/reset/push/push-identity/pop functions from
this new storage and compares each with its original routine. It checks copied
matrix bits, identity reset, retained dirty popped slots, raw depth underflow,
selection bounds and replacement without free. Consumer reset assumes the
original DOL's ordinary zero/one identity constants; they are checked in every
capture. This is a storage/consumer join, not camera construction or live wiring.

## Integration test and scope

The untouched original DLL executes every game routine in this closure. Only
heap8018B7E4, error801AF57C and geterror801AF630 return early with explicitly
matched service arguments/results. Full table/slot/global bytes are observed at
each service boundary. Real nonvoid returns and all24MiB compare after each
operation outside exactly `[80F46FE8,80F47008)`. This32-byte union is the
initializer's24-byte stack frame plus the caller LR slot; all other reached
functions have smaller frames. No table, slot, padding or global is masked.

Full mode uses contexts14/15/16 twice, one saved already-initialized case plus
400 varied cases/process. Cases cover nonzero flag bytes, counts0..511, all
eight reserved entries, initial/repeated heap failure and retry, error0/high-bit
codes, slot counts0..64, dirty extra slots, replacement, lookup/selection bounds,
and25 matrix-only startup sequences/process. Ten production mutants target
reserved count, requests, failure publication, zeroing, capacity, identity,
current pointer preservation and the consumer binding. Build errors, crashes,
timeouts and host rejection never count as caught mutants. Fast mode uses
context15 once,40 edits and no mutants. Seven malformed-host checks are separate.

Host status is distinct from the original guest result. Safety contracts require
bounded/aligned accessed words, nonwrapping supported allocations, valid stack
indices for allocation, stable services and nonoverlapping storage/constants.
Invalid inputs may reject after an observed heap request/publication; no cleanup
or rollback is invented. The original does not perform these safety checks.
Callers retain RAM, services and allocations. This module does not implement
the actual heap, freeing/teardown, palette pool or a fresh startup entrypoint.

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_matrix_storage.py
```

Private artifacts stay at
`W:/Daywater/research-temp/nflstreet-matrix-storage-20261018/{fast,full}/`.
