# Original weighted cluster palettes

## Verified scope

`cluster_palette.c` reconstructs the weighted body-cluster matrix arithmetic in
original `801CA4E8` and `801CA720`. It consumes an owned, processed TMdl model,
its attached view, optional signed-short angles in model skeleton-row order,
a supplied 3x4 root matrix and six original polynomial coefficients. It produces
native float matrices without loading RAM, Dolphin or the original DLL.

Game: NFL Street, GameCube GNNE69 revision 0; DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Reference captures: Dolphin 2603a,
position-neutral contexts 14, 15 and 16. Confidence: **verified for the tested
helper arithmetic and output paths**, not full actor orchestration.

Relocated skeleton and cluster pointers are resolved relative to the caller's
model token. Interpolation uses the original full signed difference, low-32-bit
multiply, arithmetic shift by 12 and low-16-bit angle conversion. The unsigned
weight is not restricted to 4096. Rotation uses the original paired polynomial
and explicit fused multiply/add order; it does not call host sine/cosine.
Malformed data, insufficient angles and nonfinite matrices reject without
changing output or count. Input geometry is read-only.

## Original execution gate

Run with the menu verification helpers on the Python path:

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_cluster_palette.py
```

Private evidence: `W:\Daywater\research-temp\nflstreet-cluster-palette-20261013\verification.json`.
The gate builds a separate Windows oracle and checks three saved contexts,
repeated in two fresh processes each. Per process, 12 body type/variant models
(TBYP 0/1/2/7, variants 0/1/2), 20 root/pose cases and three output paths produce
720 original calls and **88,320 complete matrices, bit for bit**. Cases include
stored angles, signed-short extremes, weights 0/1/2048/4096/65535 and translated,
rotated, scaled and sheared roots. Direct output, scratch-copy and scratch-only
paths agree. Three failure guards preserve prior output/count.

The original module supplies initialized polynomial coefficients. The fixture
supplies the root stack and model destination descriptors. The cache routine
returns; the scratch copy performs the requested-byte copy. This does not verify
cache hardware behavior or the full original memory state. Reports pin source,
private inputs, executable, logs and untouched DOL/DLL/runner hashes.

## Remaining caller work

`80162760` still owns actor position/scale, head and attachment transforms,
variant/LOD choices and destination publication. This kernel does not reconstruct
that entire caller. Natural clip selection and pose conversion into model-row
angles also remain separate. Existing tolerance-based `pose_clusters` behavior
is preserved. See [selected body composition](match_body_palettes.md).
