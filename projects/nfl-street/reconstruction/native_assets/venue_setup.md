# Venue/config scalar producer (plan row 02)

`venue_setup.c` reconstructs `80026C34` and `80026CCC`. Static confidence is
**probable**, based on both cached disassemblies and decompilations. Windows
cross-compilation passes; original execution and mutation results are pending
external gate execution. No new live/emulator integration is claimed.

Provenance: NFL Street, GNNE69 North America revision 0, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`, MEM1 base `80000000`,
r13 `803A9A40`. Existing constructor documentation identifies Dolphin 2603a
for position-neutral captures; newer gameplay documentation sometimes says
unavailable. The private verification manifest preserves that discrepancy.

## API and integration

`nflstreet_venue_archive_base(uint32_t raw_key)` returns the original archive
base, including the original default branch for keys outside 0..7.
`nflstreet_venue_setup(uint8_t *mem1, size_t bytes, const
nflstreet_venue_setup_services *services)` reads the published key and matching
DOL constants and writes the original config/index destinations. The span must
cover at least offset `2A391E`; the intended owner supplies fresh 24 MiB MEM1.
The host success value is not a return value of the original void routine.
Invalid span/service arguments return zero before any write; valid inputs do
not introduce rejection branches. Services are synchronous `word` and `real`
callbacks named by original reader PC, with identical execution order.

In the later venue-startup owner, first publish the selected raw venue key at
`802A301C` (original `80027270`). Then call
`nflstreet_venue_setup(ram, ram_bytes, &services)` before environment acquisition
`8010870C`. The caller must supply matching DOL/data constants `80245820/24`
and backed FNIG reader services. Do not copy this gate's supplied responses
into production startup. No shared/live file changes are needed for this gate.
The index/config block is the output; no heap allocation or output owner is
created by these routines.

## Source-backed fields and readers

All multibyte fields below are big endian. Confidence remains **probable** until
the pinned original-module gate passes; its complete RAM comparisons supply
the reproducible verification procedure and observed values. Existing saved
contexts are controlled regression states, not independent fresh boots.

| Location | Primitive / size | Producer behavior |
| --- | --- | --- |
| `802A301C` | u32 / 4 | Raw selected venue input, retained |
| `802A3020..302D` | seven u16 / 14 | Base, base+1,+2,+3,+4,+9,+10 |
| `802A305C` | u32 / 4 | Reader `8014B588` |
| `802A3060` | byte / 1 | Raw values 0/1/2/3 select 1F/2D/44/5F; other values retain byte |
| `802A3061..3063` | bytes / 3 | Retained, including when selector is written |
| `802A3064` | u32 / 4 | 0 for query <= threshold; otherwise 1 for raw selector zero, 2 otherwise |
| `802A3068/6C` | f32 / 4 each | Readers `8014B5B8/5E8`, rounded on store |
| `802A3070` | i32 / 4 | Reader `8014B618` times f32 `80245820`, fmuls rounding, then fctiwz |
| `802A3074` | f32 / 4 | Reader `8014B648` |
| `802A3078/7C` | i32/f32 / 4 each | Copies of `3070/74` |
| `802A3080` | f32 / 4 | Reader `8014B678` |
| `802A3084/88` | u32 / 4 each | Readers `8014B6A8/6D8` |
| `802A308C` | f32 / 4 | Reader `8014B708` |
| `802A391C/1D` | byte / 1 each | Both cleared, in original order |
| `80245820/24` | f32 / 4 each | Scale and comparison threshold inputs, not owned here |

Keys 0..7 map to `4D,2C,16,0B,00,21,37,42`; other keys return `0B`.
The checker compares original return bits, rather than using a duplicate mapping
as its expected result. The `fcmpu; cror SO,EQ,LT; bso` sequence sends unordered
comparisons down the same path as greater. This differs from the decompiler's
plain `<` spelling and is preserved explicitly. Integer conversion models
truncation, saturation and the Gekko NaN result, avoiding undefined host casts.

The nine original reader wrappers execute in the oracle, in this order:

| Reader | Lower FNIG query | Raw tag |
| --- | --- | --- |
| `8014B588` | `80149614` word | `4C545645` |
| `8014B5B8` | `8014966C` real | `4C505645` |
| `8014B5E8` | `8014966C` real | `46505645` |
| `8014B618` | `8014966C` real | `44575645` |
| `8014B648` | `8014966C` real | `4C575645` |
| `8014B678` | `8014966C` real | `4C465645` |
| `8014B6A8` | `80149614` word | `43465645` |
| `8014B6D8` | `80149614` word | `53445645` |
| `8014B708` | `8014966C` real | `44455645` |

No additional weather/rule meaning is assigned to these raw tags.

## External integration gate

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_venue_setup.py
```

Set `NFLSTREET_GATE_FAST=1` for context 15, one fresh process, 1 saved-as-is RAM
case plus 32 edited cases, no mutants. Full mode uses contexts 14/15/16, two
fresh processes each, 1 saved-as-is RAM case plus 512 edited cases per process.
Both modes print the same case/threshold/digest summary format and a gate
summary. Private artifacts, logs, scratch mutants and `verification.json` live
under `W:\Daywater\research-temp\nflstreet-venue-setup-plan02-20261016`.
`NFLSTREET_VENUE_SETUP_OUT` may select another private research-temp directory.

The gate pins DOL/module baseline hashes, captures, complete native sources,
checker/build/gate/doc, imported helper, original cached routine evidence and
transitive CPU/module ABI headers before execution and checks them afterward.
There is no generated native closure; the pinned DLL is the executable oracle.
It verifies the module's ABI, CPUState size and game ID at load. Each case
starts from identical prepared RAM. After the base function, return and all RAM
must match; after setup, all RAM and ordered query observations must match.
There is one declared stack exclusion `[80F46F00,80F47000)`, encompassing only
oracle caller/wrapper frames. No broad allocation mask, GX boundary or native
stand-in is involved. `80026CCC` has no defined return to compare.

The saved-as-is case leaves input RAM and constants untouched. Query values are
explicitly supplied: corresponding stored scalar values, except reader
`8014B618` receives 1.25. This is not a replay of natural FNIG ownership.
Edited cases dirty the entire `802A301C..308F` block and both clear bytes;
cover all keys, default keys, each query, selector values 0..3 and other raw
words, source scale/threshold edits, equality/adjacent threshold values,
unordered thresholds, signed truncation and fmuls rounding boundaries. Bytes
outside actual destinations, including the selector's neighboring bytes, must
remain equal. No generator restriction is used to hide a reached branch.

Eight one-line mutants are compiled and run concurrently in separate private
output directories, with stable report order: wrong venue branch, wrong
resource increment, duplicated/swapped config result, omitted state clear,
threshold inversion, omitted copy, widened selector store, and wrong scaled
value. A kill requires checker exit 7 with a real RAM or base-return mismatch;
compilation errors, crashes, timeouts, native rejection and service-only
mismatches do not qualify. Fresh full-mode baseline logs must be identical.

## Remaining scope

This delivers the complete bounded row-02 producer and its external gate,
awaiting that gate's execution evidence. FNIG owners, selected-key menu join,
DOL-backed immutable-data initialization, environment loading/relocation,
lighting/sky resource startup and the remaining match owners are not modeled.
The next useful join is this block with the disc-backed environment owner.
Neither a fresh match nor live gameplay is implied by compilation or scalar
comparison success.
