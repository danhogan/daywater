# Owned selected face textures and original request state

`face_texture.c` owns the named face-texture plan, decoded raw MMAP, processed
MMAP and inspection image zero. `selected_appearance_assets.c` joins this owner
to selected face/body/hair geometry. Caller namespaces must keep the texture
separate from model files and the shared body catalog; rejection releases new
allocations and preserves output. Body variant remains explicit.

This path needs local DAT archives, original DB records and the prepared private
skeleton catalog. It does not need a DOL, snapshot, original DLL or Python lookup
catalog at runtime. Plane-zero pixels are inspection data, not proof of the
original material/palette choice or GPU output.

## Original selection and request behavior

Original `800E3104` formats `PLATEX_FACE_<name>` with its37-byte formatter limit.
`800E2FD4` trims the first space and queries the PLATEX archive through actual
`801A8088`. A miss queries `PLATEX_TEMPLATE_FACE`; a second miss remains-1.
`800E2928` registers platex.dat and stores the handle at r13-5ACC.

Preview update `800DF5CC` takes the named branch when its cached name is nonempty;
its other branch uses `800E318C` and a cap selector. This new owner supports the
named path for the 402 regular-roster rows. The cap path and full preview caller
activation are not reconstructed by this milestone.

`800E2E48` leaves a24-byte record unchanged for index-1, or when force=0 and both
index/palette already match. Otherwise it writes index at+0, palette at+10,
byte1 at+0C and byte0 at+0D, preserving every other byte. These literal updates
are verified. The interpretation as a pending texture request is **probable**
from the later `800E27F8` loading/flag-clearing path, not a newly verified full
request lifecycle.

## MMAP processing and field provenance

The native processor reconstructs original `801C5610` for MMAP version2. It
rebases table references, image payload references (zero-size images retain a
zero pointer), palette payload references, names and the optional extra record,
then sets bit0 at header+6. Repeating a processed call is a no-op. A processed
buffer cannot be retokenized; its owner retains the original identity.
Untested empty-image and nonzero-extra branches remain **probable** from
original code. Other versions, formats and malformed spans reject defensively instead of
claiming equivalence to all original unsupported-format branches.

Game: NFL Street GameCube GNNE69 revision0. DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Comparison snapshots came from
Dolphin2603a; offline module oracle, no new live emulator integration.

| Field | Primitive/size and byte order | Original reader/writer | Confidence |
|---|---|---|---|
| Request+0,+10 | Signed index/u32 palette,4 each, BE |800E2E48 | Verified in controlled record comparisons |
| Request+0C,+0D | u8/1 each |800E2E48,800E27F8 | Verified literal writes; lifecycle role probable |
| MMAP+4,+6 | BE16 version/flags |801C5610 | Verified v2 and bit0 relocation behavior |
| MMAP+C,+E,+10 | BE16 name/image/palette counts |801C5610,801C588C,801C58C0,801C5958 | Verified within selected files |
| MMAP+14,+18,+1C,+20,+24 | BE32 references |801C5610 | Verified processed table/name/extra references |
| Image16-byte row+8,+C | BE32 size/reference |801C5610,801C58C0 | Verified image payload routing |
| Palette12-byte row+4,+8 | BE32 size/reference |801C5610,801C5958 | Verified palette payload relocation |
| Extra record+4 | BE32 reference |801C5610 | Probable from original code; nonzero branch presence not separately counted |

Request offsets are relative to one caller-owned record. Original reference
records are at802799EC + slot*600 + channel*24; the check controls slot2/channel1.
These original addresses are not native runtime inputs. Observed request values
include index-1/no-op, paletteFFFFFFFF, and flag writes1/0; unrelated bytes stay
identical. Resource/record evidence and input hashes are private on W:.

## Verification

```bash
python3 projects/nfl-street/reconstruction/native_assets/verify_face_texture.py
```

**Verified native data coverage:** all402 regular-roster rows load a face
texture;15 use the original template fallback. Image-zero inspection decoding
produces13,172,736 pixels per fresh native process. This pixel count is native
coverage, not an original pixel comparison. The joined owner passes1,206
geometry/face-texture assemblies across three explicit body variants, with
prepared face/hair skeleton binding and request-record writes. Three intentional
texture collisions with face/body/hair namespaces reject and preserve output.
Two fresh standalone processes and two joined processes agree.

**Verified controlled original behavior:** each selected name exercises initial
request, unchanged repeat and forced rewrite, comparing the complete24-byte
record. A missing-name fallback and direct negative-index no-op are also checked:
1,208 cases per original process. Actual formatting, space trimming, HSH1 lookup
and request logic execute; only resource synchronization/diagnostics are supplied.
The hash descriptor/table are installed from the fresh original archive, not a
Python selector map. This does not execute complete archive registration.

All388 distinct selected MMAP files match the original complete processed bytes
and repeat no-op calls. Original `801C5610` and its nested helpers execute without
relocation stubs. Contexts14/15/16 repeat twice in fresh Windows processes:
7,248 request/selection cases and2,328 whole-file comparisons plus2,328 repeats.

Private report:
`W:\Daywater\research-temp\nflstreet-face-texture-20261012\verification.json`.
It records all source/header, executable, archive, DB, skeleton-catalog and
protected DOL/DLL/runner hashes. Inputs remain unchanged. Original assets,
pixels, builds and logs stay private on W:; none are embedded in committed code.

## Remaining appearance work

This supplies the first owned selected appearance texture alongside geometry.
Body/clothing/other texture keys, palette selection and composition, texture
application to original material bindings, resource-pool/descriptor activation
and fresh actor creation remain open. Menus and the controllable reference scene
still work; the new owner does not yet make a fresh selected match playable or
establish selected-player draw fidelity. Daywater's generic platform is unchanged.
