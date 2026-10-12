# Selected hair postattachment binding

`hair_postattach.c` connects the existing owned three-slot hair resources to
native `801C6B38` reconstruction. It supplies each model/view and a fresh
SKL1 target in a bounded caller namespace. Missing model slots remain unchanged.
File, view and target identities must be disjoint. Caller allocation/release
services retain control of temporary storage. Ownership remains with the hair
resource owner; a later failure can retain earlier mutations.

The selected geometry checker now binds all three owned hair models after
PLAY/GEAR headwear selection. The adapter is independent native code: it does
not consume a RAM snapshot or original module at runtime.

## Original target provenance

Original `80041108` acquires `animdata.dat` entry101 into `r13-7BC0` and
relocates it through `8017EB24`. `800418E4` passes that value as argument4 to
`800E14D4`, which stores it at `r13-720C`. `800E00C8` supplies that field as
the target for its `801EB3F4` hair-resource acquisition and later `80161CC4`
calls. Confidence for this complete caller chain is **probable from original
code**; the complete original pool/acquisition path is not executed here.

`8017EBD0` reads the target BE16 count at+6, BE32 name-table pointer at+C and
null-terminated names at+4 within20-byte records. Actual `801C6B38` and its
mapping/remap helpers execute in the comparison below; this use is **verified
within those controlled cases**.

Original `80040DB0` also changes entry101's six-byte records starting at+410
(each triplet ends at+414), and `80041108` writes floats at+10/+14/+18. Those
startup edits are not reconstructed by this milestone. Their write locations
are separate from the count/name table used for binding, as checked in the
original source. The test target is raw-relocated entry101; it does not claim
complete original skeleton initialization or pose equality.

## Reproducible verification

```bash
python3 projects/nfl-street/reconstruction/native_assets/verify_hair_binding.py
```

Game: NFL Street GameCube GNNE69 revision0. DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Original CPU contexts came from
Dolphin2603a. This is an offline unchanged-module oracle, not a new live test.

**Verified native coverage:** 402 regular-roster selections across three
explicit body variants produce1,206 geometry assemblies and bind3,618 hair
models per fresh Windows process. The target is decoded/relocated afresh from
animdata entry101. Every triplet is bound again; no additional allocations or
releases occur. Two fresh native process logs agree. Existing face binding and
namespace-collision rejection remain exercised.

**Verified controlled original behavior:** all831 distinct archive/index
combinations selected by the automatic headwear path match complete mutated
source files and52-byte views. All831 execute the allocating path:2,493
requests per original process, with exact count/width, allocated storage,
release order and repeated no-op results. Repeat calls use a zero target after
binding, matching the original early exit. Original `801C6B38` and nested
mapping/remap helpers execute; only allocation/release services are supplied.
Three contexts14/15/16, twice each in fresh Windows processes, give4,986
whole-file/view comparisons plus4,986 repeated calls and14,958 checked
allocations. Native and original resource identities are equal in each direct
comparison; the native all-roster assembly also uses its independent namespace.

The original decision/field checks remain part of this gate:1,650 original
field extractions,402 headwear decisions and64 mapping inputs per process,
with GEAR query services supplied from fresh original-extracted records.
The separate headwear and explicit-state hair gates still pass.

The private report at
`W:\Daywater\research-temp\nflstreet-hair-binding-20261012\verification.json`
records source/header, executable, input and protected DOL/DLL/runner hashes.
Original files are unchanged. Builds, decoded files and diagnostics stay on W:.
Related model field provenance is recorded in [model binding](model_postattach.md)
and [headwear selection](headwear_state.md).

## Remaining connections

This closes hair skeleton/cluster binding for the tested selected resources.
Full resource pool/descriptor activation, game-specific skeleton position/scale
edits, texture/material ownership, fresh constructor descriptors and actor
creation remain unfinished. Menus and the controllable reference scene work;
the selected geometry is not yet used to start a fresh playable match.
The generic Daywater platform is unchanged.
