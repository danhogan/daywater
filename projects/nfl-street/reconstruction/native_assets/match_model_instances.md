# Selected owned model instances and matrix publication

`match_model_instances.c` joins current selected body/face/hair views and their
native matrices into owned original-layout draw objects. Each part owns a
240-byte instance, copied52-byte model view, 12-byte palette descriptor and
big-endian float matrix buffer. Tokens are explicit host namespace identities.
The span resolver provides bounded access to those owned buffers.

Part order is body, face0/1, hair0/1/2. Missing hair stays absent. Material-table
tokens are **borrowed caller inputs**; this owner does not serialize the existing
semantic material owners or infer actor model slots. Its palette descriptor uses
supplied matrix count and direct destination, with scratch token zero; it does
not reproduce original frame allocator/capacity rounding or locked-cache policy.

Confidence: **verified native composition, serialization and rollback**. The
[instance initializer](model_instance.md) independently matches actual original
execution. Aggregate startup is not a complete original actor comparison.

## Reproduction

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_match_model_instances.py
```

Private report: `W:\Daywater\research-temp\nflstreet-match-model-instances-20261013\verification.json`.
Two fresh Windows processes load the actual retained menu selection, own all
prior visual/skeleton/body/palette layers and produce **84 model instances,
84 copied views/palettes and 2,724 serialized matrices**. Checks follow every
model/palette/material token link and compare every serialized float word.
Counts match each current model's Clst chunk. Three failures (overlapping host
namespaces, late nonfinite matrix, mismatched model/matrix count) preserve the
entire prior output. Invalid and crossing span lookups reject. Geometry hashes
stay unchanged. Earlier owner and rollback checks run in the same fixture.

The fixture supplies separate per-player namespaces and synthetic material-table
tokens. Those material tables are not populated; no draw is claimed. Root/pose,
scale-slot and LOD inputs remain explicit as documented in
[attachment palettes](match_attachment_palettes.md). Copied model views retain
links into borrowed owned asset files. The visual/resource owners and supplied
material tables must remain alive; callers must reserve namespaces that do not
collide with those external sources. Output may move because links resolve via
tokens, not host pointers.

## Remaining draw connection

Instances now have owned model and matrix destinations. They still need actual
material-table objects, source-selected group lists, verified actor-slot mapping,
head/attachment roots and complete actor/frame state before drawing freshly
selected players. The full original caller and natural animation eligibility are
unfinished. Captured actors already render on the native field; a fresh selected
match is not playable or visible yet.

All assets, prepared catalogs, builds and diagnostics remain private on W:.
No original DLL/DOL/runner source is modified; no external push is performed.
