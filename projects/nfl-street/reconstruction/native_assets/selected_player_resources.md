# Selected roster to processed model resources

`selected_player_resources.c` joins the existing independently loaded PLAY
resolver to the new face-pair owner and processed base-body catalog. It resolves
one exact team/DIGP, loads and processes the selected two adjacent face models,
and borrows one explicit body variant from the shared catalog. The resource
output retains the manifest, decoded face bytes, complete model views and
namespace identities. It requires no emulator, saved RAM, original module or
Python lookup catalog at runtime.

Face data is owned until close. Body assets/views are borrowed; their pool must
outlive the resource object. Namespace ranges must not overlap. Invalid player,
unsupported body variant or overlapping identities reject without changing
caller output, releasing any newly decoded face buffers. This API does not
choose original gameplay variants, acquire a selected hair model, bind materials
or textures, or construct an actor. The separate `face_postattach` adapter now
binds both owned face models to a freshly loaded SKL1 target.

## Native real-data coverage

**Verified native assembly coverage:** all402 regular-team PLAY rows from32
private DB blocks succeed for each of three explicitly selected body variants:
1,206 assemblies and2,412 bound face models in each fresh process. Entry95
from `animdata.dat` is decoded/relocated afresh; every pair makes six temporary
allocations/releases and a repeated bind makes no additional requests. Both
fresh Windows process logs agree. The checked face indices equal the separately
verified manifest index and its adjacent entry; each processed file has bounded
Info/Geom spans. The body choice agrees with the verified key/variant archive
formula, including the existing TBYP-7 alias. Three rejection checks validate
unchanged output and ownership/namespace behavior.

These are native data-coverage checks, not1,206 original gameplay comparisons.
Regular roster lookup and base-body routing have their existing controlled
original evidence. The new [face acquisition comparison](face_resource_pair.md)
executes the original selected index12/13 path in saved contexts14,15,16, twice
each:12 complete files,12 model views and12 pool-4 selections match. Root
independently rebuilds and repeats this gate. Archive decoding is supplied;
the optional original postattachment input is controlled to zero. It does not
prove complete face activation or all402 original acquisition paths.

```bash
python3 projects/nfl-street/reconstruction/native_assets/verify_selected_player_resources.py
python3 projects/nfl-street/reconstruction/native_assets/verify_face_resource_pair.py
python3 projects/nfl-street/reconstruction/native_assets/verify_face_bound_resource.py
```

Private reports:
`W:\Daywater\research-temp\nflstreet-selected-resources-20261012\verification.json`
and `W:\Daywater\research-temp\nflstreet-face-resource-pair-20261012\verification.json`.
Source/header, executable, archive and DB-block hashes are recorded. Protected
original DOL, DLL and runner hashes and all private inputs remain unchanged.
Decoded models, builds and diagnostics stay on W:; no assets are embedded in
source. The generic Daywater platform is unchanged.

## Confidence, provenance and remaining work

NFL Street GameCube GNNE69 revision0; DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Original-module reference snapshots
came from Dolphin2603a. Confidence is **verified for the native assembly
coverage and bounded original face comparison described above**. Source field
types, exact addresses/readers and limitations are recorded in
[roster resolution](../menus/team_player_resolve.md),
[body routing](../menus/team_player_loader.md), [relocation](model_relocate.md)
and [face acquisition](face_resource_pair.md).

An early comparison exposed the optional original `801C6B38` postattachment
stage selected by the saved input at r13-7220. With that stage active, the
result differs from the relocation-only resource view. It is now implemented and verified in a
[connected original acquisition comparison](model_postattach.md):12 complete
files/views and12 pool queries with binding active, using a fresh target.
The earlier relocation-only gate still passes. These checks preserve the
reference material-remap input of zero; full original appearance activation
remains outside their scope. Hair/texture/material ownership, fresh constructor descriptors,
resource-to-actor binding and original draw behavior also remain unfinished.
Menus and the controllable reference scene still work, but these assembled
resources have not yet replaced reference-player resources in a fresh match.
