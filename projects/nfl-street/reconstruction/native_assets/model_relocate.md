# Independently relocated base-body resources

`model_relocate.c` reconstructs original `801C5EC8`, its registry dispatch and
`801C5D94/801C5EAC` directory arithmetic. `model_chunk_relocate.c` owns the
section-specific pointer and state writes. `body_resource_pool_process`
connects these owners to all nine independently decoded body assets and refreshes
their complete model views. No saved model bytes or reference-actor fields are
copied into the native resource catalog.

Each resource uses its caller-owned namespace identity in BE32 pointer words;
these are not host pointers or original RAM addresses. The bounded chunk
accessor translates processed directory words into host spans using the same
identity. Model-view attachment preserves the encoded words exactly. Raw open
and processed pool operations remain distinct, and decoded allocation ownership
stays with the catalog until close.

## Owned source behavior

The dispatcher preserves a file whose header byte+0F is1. Otherwise it processes
unmarked directory rows, selects the first matching registry tag, checks the
row's version byte, rebases a nonzero offset by the resource identity, and calls
the registered handler. Unknown tags or version mismatches get a zero directory
word. Empty matching sections retain their old word. The row and header
processed bytes become1. Default registration tags/versions/callback presence
match all15 source entries from `801C6904`.

The native callbacks own section pointer writes and secondary source operations,
including Geom's literal grouping comparisons/entry-byte updates and Swap's
postprocessing. Actual base-body files exercise Info, Skel, Swap, Clst, Geom
and Matl. Other implemented handlers are source-derived and **probable** until
their own branches are compared. Lite and MAnm callbacks remain unsupported
and reject a matching nonempty section; the default metadata does not imply
their implementation. No renderer, GPU submission or animation execution is
performed by relocation.

Malformed spans and failed handlers return0. Earlier processed chunks remain
mutated; no pool rollback is claimed. Bounds guards are a native API constraint,
not a reconstruction of original malformed-input behavior.

## Original comparison

**Verified controlled behavior:**216 complete relocated-file comparisons,
216 repeat no-op calls and216 resulting model views, plus54 independently
processed pooled assets. Each of three saved contexts runs twice in fresh
Windows processes. A process covers all nine actual decoded base-body entries
with original, unknown-first-tag, first-row-version-mismatch and empty-first-row
directories. Controlled mutations affect memory copies, not the private archive.

The untouched original DLL executes the actual dispatcher and its actual
registered callbacks, including original Geom/Swap subhelpers, without replacing
them with supplied services. Each entire output file agrees byte-for-byte, not
just selected pointers. Repeat calls leave both outputs unchanged. Original
attachment agrees with native processed views. The joined pool comparison uses
the same distinct resource identities as native processing and compares each
entire processed file and its view. Original attachment uses a temporary lookup
copy; it does not execute rendering or dereference every relocated substructure.

The existing raw-view gate also passes again:432 full attachments and1,728 tag
applications. These are initialization checks, not fresh gameplay frames.

```bash
python3 projects/nfl-street/reconstruction/native_assets/verify_model_relocate.py
python3 projects/nfl-street/reconstruction/native_assets/verify_model_view.py
```

Private report:
`W:\Daywater\research-temp\nflstreet-model-relocate-20261012\verification.json`.
All decoded assets, builds and diagnostics remain on W:. Native/checker source,
transitive headers, executable, input archive and saved RAM are fingerprinted.
Protected DOL, original DLL, runner, archive and RAM hashes are unchanged;
baseline files were not replaced. Compilation passes `-Wall -Wextra -Werror`.

## Field confidence and provenance

NFL Street GameCube GNNE69 revision0; DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Saved RAM came from Dolphin2603a;
body bytes are freshly decoded from private plamodel TERF entries1..9.
This is offline original-module verification, not fresh live startup.

| Field | Primitive/size/endian | Test observation and reader/writer | Confidence |
|---|---|---|---|
| TMdl header+0F | Byte,1 | Original/native become1; repeat call changes no bytes;801C5EC8 | Verified for checked bodies |
| Directory+04 | BE32,4 | Nonzero offset becomes namespace identity+offset; unsupported tag/version becomes0;801C5D94/801C5EAC | Verified controlled transfer |
| Directory+0C/+0F | Bytes,1 each | Version selects dispatch; processed flag becomes1; altered version and repeat cases match | Verified controlled dispatch |
| r13-5688/-568C,r13=803A9A40 | Two BE32 words,4 each | Saved count15 and registry token806EEC20; all tag/version/callback-presence rows match native metadata | Verified reference input;801C5D34/801C6904 |
| Registry rows | Three BE32 words;stride12 | Tag,version,callback token; read from untouched saved input for comparison | Verified reference registration |
| Skel/Swap/Clst/Geom/Matl payload writes | Mixed BE words and bytes | Entire actual files match, including geometry grouping and Swap postprocessing; respective registered handlers | Verified within the nine-body cases |

Section labels identify the source handlers. They do not establish complete
materials, skinning, animation or draw behavior. DInf's unusual section-relative
arithmetic is implemented from disassembly but is outside the checked body
coverage; no verified general DInf claim follows.

## Remaining work

This removes raw pointer relocation as a blocker for the nine base-body views.
Fresh face/hair/texture/material resources and constructor descriptors are
still needed before these resources can replace supplied player bindings.
Original scene rendering, fresh selected-player creation and full match startup
remain unfinished. Current menus and controllable reference players still work.
