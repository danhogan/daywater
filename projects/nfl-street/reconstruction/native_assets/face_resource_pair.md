# Selected face resource pair

`face_resource_pair.c` owns two adjacent decoded `plaface.dat` TMdl entries,
their distinct resource identities, whole-file relocation and 52-byte attached
views. The caller supplies the selected archive index from the separate team
player resolver. The pair owns its decoded buffers; close releases both.

## Original comparison

**Verified in a controlled original-module check:** for selected face index 12
(the team 1 / DIGP 355 lookup), untouched `800E0AC4` calls original
`801EB3F4` on the pool-4 `PLYRMODEL` record. A bounded `801A75B0` service
decodes entries 12 and 13 from the private original archive. Original
`801C5EC8` relocates both complete files, `801C67E4` attaches both views,
and `800E0734(4, any-key, variant 0 or 1)` resolves the respective descriptor.
Each complete relocated file and view agrees byte-for-byte with the native
implementation. The native pair is also compared at its own distinct identity
tokens using original relocation and attachment helpers.

The original descriptor at `802725F8` has entry indices 12 and 13 in its first
two 0x50-byte rows and a zero third index at `+AC`. The checked original buffers
are `80F64660` (51,744 bytes) and `80F5C360` (33,504 bytes); attached views
are at `80272614` and `80272664` in the checked saved contexts. These are
original comparison addresses, not native application constants.

The original saved `r13-7220` value is nonzero (`80B96C40` in context 14).
That value selects an additional original postattach skeleton/cluster binding
stage, which is outside the native pair API. The checker sets it to zero on
its private RAM copy so the comparison covers acquisition, relocation and
attachment only. It leaves original `800E0AC4`, `801EB3F4`, `801C5EC8`,
`801C67E4` and `800E0734` intact. The saved `r13-6234` flag is already zero
in the checked context. No complete face activation or player creation is
claimed by this check.

The private verifier passed saved Dolphin 2603a contexts 14/15/16 twice in
fresh processes: 12 complete-file comparisons, 12 attached-view comparisons
and 12 pool-4 queries. It checks unchanged protected input hashes and writes
its report and builds only on W:.

```bash
python3 projects/nfl-street/reconstruction/native_assets/verify_face_resource_pair.py
```

Private report: `W:\Daywater\research-temp\nflstreet-face-resource-pair-20261012\verification.json`.

## Field evidence

All addresses below describe isolated original-module checks. Words are4 bytes,
big endian; r13 is803A9A40. Test procedure: fresh copied reference RAM, actual
archive decoding, original acquisition/relocation/attachment, whole-file and
whole-view native comparison, repeat in a new process. Confidence is **verified
for these controlled cases**, with optional postattachment explicitly excluded.

| Field | Observed value | Reader/writer and limit |
|---|---|---|
| r13-722C | Descriptor token802725F8 |800E0AC4/801EB3F4; original reference pointer |
| Descriptor+0C/+5C/+AC |12,13,0 |800E0AC4 writes archive indices;801EB3F4 consumes them |
| Descriptor rows+1C |52-byte model views at80272614/80272664 |801C67E4 writes12 section slots;800E0734 selects variant0/1 |
| r13-7220 | Context14 token80B96C40; checker supplies0 |800E0AC4 passes optional input to801EB3F4; zero excludes801C6B38 stage |
| r13-6234 |0 in checked context |801EB3F4 conditional postattach remap; no active-remap equivalence claimed |

## Confidence and limits

Game: NFL Street GameCube GNNE69 revision 0. DOL SHA1:
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. **Verified** here means
the controlled, reproducible original-module comparison above. It does not
imply a fresh live Dolphin startup, runtime animation/material readiness,
postattach skeleton binding, or gameplay state discovery. Cache ownership and
filesystem acquisition remain external to the native pair. The pool-4 third
descriptor has a zero index; this check makes no third-variant asset claim.
