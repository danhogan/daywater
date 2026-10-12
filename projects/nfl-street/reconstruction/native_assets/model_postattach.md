# Face model postattach binding

`model_postattach.c` implements original `801C6B38(view, target_skeleton)` in
a bounded GameCube token namespace. The 52-byte attached view supplies `Clst`
at `+0C` and `Skel` at `+10`. If either is absent, the function returns without
mutating the model. Otherwise, it maps names from 16-byte source skeleton rows
to the target `SKL1` name table, remaps four-byte cluster records, copies the
6-byte and 16-byte skeleton arrays through temporary allocations, remaps the
signed parent halfword at row `+0C`, releases the three allocations in original
order, and clears `Skel+4`.

The namespace contains only caller-provided memory spans and allocation
storage. The allocator callback receives original `(count, width)` requests:
`(source_bones, 2)`, `(source_bones, 6)`, `(source_bones, 16)`. It and the
release callback let the caller control storage and observe the release order.
All reads and writes are checked against supplied spans. Invalid input returns
zero. The native source strings are compared as bounded, null-terminated bytes
with the same exact matching relation as original `strcmp`.

## Controlled original comparison

**Verified** for the controlled cases below: native `face_resource_pair_open`
decodes actual `plaface.dat` entries 12 and 13, relocates each whole TMdl, and
attaches both 52-byte views. Original `801C6B38` and the native binding then
run against independent copies of each saved MEM1 context. Both use a fresh target decoded from `animdata.dat` entry95 and relocated by
[skeleton_resource.c](skeleton_resource.md), at checker identity `81400000`.
No target skeleton bytes are borrowed from the snapshot. The checker compares
each complete mutated source file and attached view, allocation counts and
sizes, every allocated buffer, and release order. It intercepts only original
allocation and release; the binding and its other helpers execute in the
preserved original DLL.

Three saved Dolphin 2603a contexts (14, 15, 16) pass twice in fresh processes:
12 whole-file comparisons and 12 whole-view comparisons, plus12 repeated calls
with a zero target after the source name table has been cleared. Those repeated
calls preserve files/views and make no further allocation requests. The verifier records
hashes of protected inputs and checker sources, then checks they are unchanged.

```bash
python3 projects/nfl-street/reconstruction/native_assets/verify_model_postattach.py
```

The private report and executable are under
`W:\Daywater\research-temp\nflstreet-model-postattach-20261012`.

Game: NFL Street GameCube GNNE69 revision 0. DOL SHA1:
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. These comparisons establish
postattach binding behavior for the selected source and target inputs. The connected acquisition comparison below adds the actual original load path;
no live Dolphin runtime behavior is claimed.

## Connected acquisition with binding enabled

**Verified controlled path:** `verify_face_bound_resource.py` executes original
`800E0AC4` → `801EB3F4` → relocation → attachment → `801C6B38`, with
`r13-7220` pointing to the freshly loaded entry95 target. Original allocation,
binding helpers and pool-4 lookup execute; only archive decoding is supplied.
The material-remap input `r13-6234` stays zero, as in the reference snapshots.
Native complete face files/views match the acquired entries12/13 in contexts
14/15/16, twice in fresh Windows processes:12 acquired files,12 views,12 active
binding calls and12 pool queries. A second pair comparison uses an independent
native token namespace. Original-module and native allocation addresses differ;
whole model results match under the corresponding namespace identities.

```bash
python3 projects/nfl-street/reconstruction/native_assets/verify_face_bound_resource.py
```

Private report:
`W:\Daywater\research-temp\nflstreet-face-bound-resource-20261012\verification.json`.
This proves the selected acquisition path with fresh skeleton relocation, not
all original roster branches, material/texture binding or playable match startup.

## Field provenance and confidence

All fields below are big endian and **verified for the controlled inputs above**.
View offsets are relative to an attached52-byte model view, not global RAM:

| Field | Type/size | Reader/writer | Checked interpretation |
|---|---|---|---|
| View+0C / +10 | u32/4 each |801C6B38,801C67E4 | Clst / Skel references |
| Skel+0,+4,+8,+C | u32/4 each |801CD18C,801CD224,801C6B38 | Source count, names,16-byte rows,6-byte rows |
| Clst+0,+4 | u32/4 each |801CA368 | Four-byte record array and count |
| Cluster record+0 | u8/1 |801CA368 | Remapped skeleton index; unmatched predecessor becomesFF |
| Skeleton16-byte row+C | signed16/2 |801CD224 | Parent index,FFFF absent |
| SKL1+6 / +C | u16/2, u32/4 |8017EBD0 | Target count and20-byte name table |
| Target name row+4 | null-terminated bytes |8017EBD0 | Exact name comparison |

Names absent after binding make a repeat call a no-op even with a null target.
Invalid spans/indices are rejected by the native API. First map-allocation
failure follows the original name-table clearing path; later failures can retain
earlier mutations. These failure branches are defensive/static evidence,
**probable**, and are not covered by the successful real-data comparisons.
