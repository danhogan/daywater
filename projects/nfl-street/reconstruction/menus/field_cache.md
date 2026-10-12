# Field selection: cached record reader

## Scope and evidence

**Verified controlled function behavior**, NFL Street GameCube GNNE69-r0.
Original DOL SHA1 `46e3de854998a0f66c9be8c72a1164d22cd29eb4`; untouched
original DLL SHA1 `dd521cc38b0db6bca98a0bb261570a2be5e6e0ed`.
This is the cache-hit branch of original `8011F1F0`, not full field selection.

Original `8011F0C8` allocates0x668 bytes and initializes eight205-byte records
through `8011F1F0`. In the latter's cache-hit branch, the record pointer is read
from `r13-6E08`; selected index is written to `r13-6E04`. The checker supplies
its own scratch RAM and r13, without Dolphin or a saved match. Those scratch
addresses are not newly discovered live fields.

| Record offset | Primitive / size | Controlled behavior |
| --- | --- | --- |
| 0, 0x21, 0x42 | Three byte spans,33 bytes each | Copy through original `8017CF30` with caller length32+1 |
| 99 | Byte span,102 bytes | Copy with caller length101+1 |
| 0xC9 | uint8,1 byte | Zero-extend into caller's uint32 output |
| 0xCA | uint8,1 byte | Nonzero selects the cached branch |

Text copying stops at the first NUL, retaining later destination bytes; when
the bound is exhausted, the last destination byte becomes NUL. Outputs use
neutral names `text0/text21/text42/text99`; no unsupported meaning is assigned
to the three short strings or rawC9. Original RAM uint32 fields are big endian;
the native output stores ordinary host integers.

## Completed comparison

`field_cache_original_check.c` executes untouched `8011F1F0` and its copy helper
in24 enabled cases: all eight slots with early NUL, no NUL inside the span,
and near-bound NUL. It compares all four output spans, rawC9, selected index,
unchanged record bytes and destination tails. Separate native-only checks
reject out-of-range index/count, truncated records, output overlap and an
unset cache flag without changing output. The original cache-miss path does
construct data; native rejection is not an original no-op claim.

Strict Windows build and original check pass. Private executable/log:
`W:\Daywater\research-temp\nflstreet-field-cache-20261011\`.
Reproduce with `build_windows_field_cache_check.sh`, then run its
`field-cache-original-check.exe` with the private original DLL path.

## Remaining connection

The native helper can read already populated records. A native catalog reader
now builds those records from the actual private RVNE table, as scoped below.
Original profile services and complete cache-miss population remain open;
field-screen construction, activation,
navigation, preview images, unlock policy and selected-field handoff remain
unfinished. This code is a reusable recipient component for the team-to-field
flow, not a visible field picker or fresh match startup.

## Actual private field catalog — 2026-10-11

`field_catalog.c` reads the eight RVNE rows from private
`template/00002.bin`, requiring unique raw DIVE keys0..7. Archive row order is
0,1,2,7,3,4,5,6; the native output is indexed by DIVE. MNVE, OLVE and EDVE
populate record offsets0,0x21 and99. The offset0x42 text and rawC9 byte remain
explicit caller inputs. No field-name enum or unlock semantics are inferred.

**Verified controlled consumption:** `field_catalog_original_check.c` passes
all eight native-built records to untouched original `8011F1F0` and compares
its cache-hit output to the native reader: text spans, retained tails, rawC9
and selected index. Root independently reran the Windows checker successfully.
Truncated native catalog input is rejected atomically. Private evidence:
`W:\Daywater\research-temp\nflstreet-field-catalog-20261011\`.

The enhanced checker also executes original primitive readers `801B94C8` and
`801C4388` on actual RVNE record bytes: all40 numeric and24 text values consumed
by the catalog match, including untouched output tails. Other RVNE fields,
including type4 fields, are excluded. This verifies bounded raw extraction
and consumption of native-built cached records. It does not reconstruct
original query ownership or cache-miss/profile/unlock initialization.
Original DLL hash remains unchanged.

`verify_field_catalog.py` builds this checker and runs two fresh processes,
requiring identical traces and unchanged DOL/DLL/runner/template hashes.
Use `--no-build` to reuse the current checker. The private `verification.json`
records executable, input and log provenance.

## Actual UI slot order

The UI index is not the raw DIVE key. `field_bootstrap.c` maps original DOL
sections and reads eight bytes at original `r13-6E10` (`803A2C30` for supplied
r13 `803A9A40`): `7,1,5,2,6,0,4,3`. It reorders the catalog accordingly and
uses the original DOL description at `80256E40` for key0, following the static
cache-miss override in `8011F1F0`. Other metadata and supplied text42/C9 retain
their raw-key ownership.

**Verified bounded original key selection:** the catalog checker now executes
untouched `8011F1F0` with uncached scratch records for all eight UI slots,
stopping at its first `8014B4F0` query. Each requested key agrees with the native
DOL-backed order. It does not execute the remainder of cache-miss population.
Both fresh verification runs pass; malformed DOL input retains output.

## Number labels and bounded no-profile availability

`field_stat_text.c` reproduces original `80129B24` signed decimal labels and
comma grouping. Thirteen controlled original cases from zero through both
signs of `2147483647` agree, including output tails. `INT32_MIN` is rejected
pending an original edge check. The statistic itself remains a caller input;
this does not establish any original saved-stat default.

`field_availability.c` consumes the actual IVID rows. Original `8011F1F0`'s
availability branch agrees for all eight raw field IDs when original profile
queries return `FFFFFFFF` and actual IVID iterator outcomes are supplied.
OCID0 resolves through DIGD7 to IFID7, and OCID1 through DIGD3 to IFID1.
In that explicit no-profile context, keys7 and1 have C9 zero; other keys have
C9 one. This covers that branch, not general career unlocks, original query
ownership, or complete cache-miss initialization.

`verify_field_catalog.py` now builds and repeats both checks in fresh Windows
processes and records their separate executable/log hashes. Protected original
DOL, DLL, runner source and private template hashes must remain unchanged.
