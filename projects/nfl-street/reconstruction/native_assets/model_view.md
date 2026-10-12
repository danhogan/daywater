# Fresh base-body resource views

`model_view.c` owns original `801C67E4`, `801C6644` and their directory lookup
behavior for independently decoded raw TMdl files. `body_resource_pool.c`
connects the existing verified body selector and TERF decoder to this owner,
creating all nine base-body views and retaining their decoded bytes. The public
catalog accepts explicit key/variant requests; TBYP 7 aliases key 2. It does not
choose a gameplay variant automatically.

No saved actor fields, decoded model bytes or proprietary tables are embedded
in the implementation. Model bytes come from the caller's local archive. Each
decoded resource receives a separate opaque identity in a caller-supplied
namespace. Close releases all decoded allocations. Open failure releases
partial allocations and leaves the output object unchanged.

## Raw encoding and ownership

The 52-byte view stores the directory's encoded words unchanged. These words
are raw section offsets, not namespace-base-plus-offset pointers. A bounded
chunk accessor returns the corresponding host byte span alongside its resource
identity. This is sufficient for raw resource inspection and later native
parsers; it does not establish relocated pointers usable by the existing
player-binding pipeline.

Attachment clears words +2C/+30, then performs the original twelve tag lookups
in order. A first nonempty matching row wins; empty rows are skipped. Missing
tags preserve earlier view contents. Word +1C is untouched. A direct lookup of
an unknown tag reports success when a nonempty row exists but writes no slot.
Fresh catalog views start with zeroed storage. The bounded API validates header,
directory and chunk spans before attachment writes. The subsequent relocation
milestone extends it to processed directories using the supplied resource token;
original malformed-input behavior is not claimed.

## Original comparison

**Verified controlled behavior:** 432 complete attachments and 1,728 direct
tag applications, across contexts 14, 15 and 16 with two fresh Windows processes
each. Every process decodes all nine actual plamodel entries through the native
body binder. Cases cover zeroed/prefilled output and original/missing-first-tag/
empty-first-row/duplicate-row directories. Controlled variants mutate copies,
not source archive files. All 64 output bytes, including the 12-byte guard
after the 52-byte view, and direct application status agree.

The original DLL executes `801C67E4`, `801C6644`, `801C5F68` and `801C5EAC`
with each decoded file in isolated memory. No attachment or lookup function is
stubbed. Nine fresh catalog views match the unmodified original attachments;
all three TBYP-7 alias queries select the checked key-2 views.

```bash
python3 projects/nfl-street/reconstruction/native_assets/verify_model_view.py
```

Private report:
`W:\Daywater\research-temp\nflstreet-model-view-20261012\verification.json`.
Builds, logs and diagnostics remain on W:. Strict Windows compilation uses
`-Wall -Wextra -Werror`. Sources, transitive headers, executable, archive and
reference RAM are fingerprinted. Original DOL, DLL, runner, archive and RAM
hashes are unchanged. No baseline source or DLL was replaced.

## Field evidence

NFL Street GameCube GNNE69 revision 0; DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Reference RAM came from Dolphin
2603a; decoded model bytes come from the original private plamodel TERF. Tests
are offline original-module comparisons, not fresh live startup.

| Field | Type/size/endian | Procedure/observation | Confidence |
|---|---|---|---|
| TMdl +08/+0A | Two BE16 fields | Read count/directory offset in each decoded file; original lookup and bounded accessor agree | Verified controlled lookup |
| Directory row +00/+04/+08 | Three BE32 words; stride16 bytes | Tag, encoded section offset and nonempty length; compare original-first-match behavior on actual and modified copies | Verified controlled lookup |
| View +00/+04/+08/+0C | Four BE32 words | Info/Extn/Geom/Clst encoded words transferred unchanged | Verified controlled attachment |
| View +10/+14/+18/+20 | Four BE32 words | Skel/Swap/Matl/Txfm encoded words transferred unchanged | Verified controlled attachment |
| View +24/+28/+2C/+30 | Four BE32 words | Inst/Rint/DInf/Fade transfer; +2C/+30 cleared before lookup | Verified controlled attachment |
| View +1C and bytes after +33 | Preserved bytes | Whole-span comparison of fresh/prefilled output and guard | Verified preservation |

Reader/writer evidence is `801C67E4 → 801C6644 → 801C5F68/801C5EAC`.
Tag labels identify source sections; they do not establish every section's
runtime layout, material semantics or animation interpretation.

## Remaining boundary

The subsequent [model relocation](model_relocate.md) now processes all nine
owned body assets and attaches their namespace references. The raw gate above
still passes after that extension.

The fresh catalog replaces reference-state extraction for these raw body views.
It is not the complete original resource-pool registration or a relocated
selected-player model. Next work is original relocation/chunk handlers,
face/hair/texture and material ownership, fresh constructor descriptors and
connection to player creation/rendering. Original menus and the controllable
reference scene still work; fresh selected-match startup remains unfinished.
