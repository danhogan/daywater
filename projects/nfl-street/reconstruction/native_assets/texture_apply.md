# Original face image/palette application

`texture_apply.c` reconstructs original `80164CE4` for processed MMAPv2 files.
Destination name-record0 supplies the image-copy count, first image and palette
selector. Source name-record0 supplies its first image and palette selector.
Each copy uses the destination's declared payload size, then issues the original
ordered address/size cache notification. Palette data is copied after the images.
Header/table/name metadata remains unchanged.

`nflstreet_face_texture_apply_template` owns a freshly decoded/relocated
PLATEX_TEMPLATE_FACE destination. The selected appearance owner can retain that
destination through `nflstreet_selected_appearance_assets_apply_face`. The
existing geometry/face-resource open API still works without application.
Replacement checks disjoint namespaces before upload callbacks, preserves the
old host destination on failure and releases the old destination only after a
successful replacement. Callback effects themselves are not rolled back.

## Evidence and confidence

Game: NFL Street GameCube GNNE69 revision0. DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Oracle contexts came from
Dolphin2603a; offline original-module comparisons, no new live emulator test.

The fields below are **verified within the real selected-face/template cases**.
Offsets are relative to a processed MMAP or its indicated record, not universal
native addresses. All multi-byte fields are big endian.

| Field | Primitive/size | Original reader | Interpretation |
|---|---|---|---|
| Header+14 | BE32 reference |801C588C | Twelve-byte name/selection records |
| Record0+2,+4,+8 | BE16/2 each |80164CE4 | Destination copy count; first image; palette selector |
| Header+18 / +1C | BE32/4 each |801C58C0 /801C5958 | Image16-byte / palette12-byte tables |
| Image row+8,+C | BE32/4 each |80164CE4 | Copy size and payload reference |
| Palette row+4,+8 | BE32/4 each |80164CE4 | Copy size and payload reference |
| Cache call arguments | Two u32/4 words |801F0BD0 call sites | Destination identity and byte count, in copy order |

The tested template performs two image copies and one palette copy, hence three
cache requests per application. Reapplication repeats all copies and cache
requests; it is not a no-op. No animation/mipmap interpretation is assigned
solely from the record count. Namespace identities are caller-supplied.

Preflight bounds all planned source/destination spans before copying. Native
out-of-range rejection is defensive behavior, not a claim about the original
malformed-input path. Source/destination aliases beyond the tested independent
files, other MMAP versions and zero-length payload cases remain outside scope.

## Verification

```bash
python3 projects/nfl-street/reconstruction/native_assets/verify_texture_apply.py
python3 projects/nfl-street/reconstruction/native_assets/verify_face_texture.py
```

**Verified controlled original application:** all402 regular-roster selections,
including15 template fallbacks, match the complete destination file after
application and reapplication. Actual original `80164CE4`, record/table readers
and memory-copy routine execute. Cache flush801F0BD0 is supplied by a recorder;
its ordered address/size requests match the native host service exactly.
The original GPU/cache effects are not executed or claimed verified.

Each original process checks402 destination files plus402 reapplies and2,412
ordered cache requests. Contexts14/15/16 run twice in fresh Windows processes:
2,412 initial whole destinations,2,412 reapplies and14,472 cache requests.
Earlier named lookup/cache-record and whole-MMAP relocation checks remain part
of the same original run, and their separate gate still passes.

**Verified native joined ownership:** two fresh processes each construct1,206
geometry/face-texture assemblies, bind prepared skeleton targets, retain1,206
applied face destinations and issue3,618 ordered host cache requests. Three
existing texture namespace guards still pass. Two new replacement guards check
source-namespace collision and rejected upload; the prior destination structure
and all its host bytes remain unchanged. No DOL, original module or snapshot is
supplied to this joined native executable.

Private report:
`W:\Daywater\research-temp\nflstreet-texture-apply-20261012\verification.json`.
Sources, executables, archives, DBs, private skeleton catalog and protected
DOL/DLL/runner hashes are recorded and unchanged. Payloads, decoded pixels,
builds and diagnostics remain private on W:.

## Limits and continuation

This verifies direct selected-face image/palette application, not the optional
palette composition/tint helpers or material/GPU binding. Body/clothing and
other selected texture keys, remaining composition, original pool/descriptor
activation and fresh actor creation remain unfinished. Menus and the reference
scene still work; a fresh selected match is not playable. Continue with the
remaining texture inputs below800DD720 and the material/palette consumers, then
connect owned appearance resources to checked player creation. Daywater's generic
platform is unchanged.
