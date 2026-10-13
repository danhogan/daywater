# Selected match body palettes

`match_body_palettes.c` connects the exact [cluster kernel](cluster_palette.md)
to the 14 independently owned [selected bodies](match_bodies.md). It reads each
player's current private body file/view, builds all matrices in temporary storage
and publishes the complete output only after every player succeeds. Callers may
rebuild into the same output each frame. Model geometry is not modified.

Confidence: **verified native ownership/composition and transactional failure
behavior**. The underlying arithmetic has a separate untouched-original gate.
The aggregate check is not a full original actor-startup comparison.

## Reproduction

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_match_body_palettes.py
```

Private report: `W:\Daywater\research-temp\nflstreet-match-body-palettes-20261013\verification.json`.
The verifier converts the actual retained Win32 menu selection to a private QMS1,
loads both selected lineups from disc STAT/GAME data and repeats in two fresh
Windows processes. All earlier geometry, material, fresh skeleton and body
binding checks execute in the same fixture.

Fourteen owned body views, each with stored and perturbed model-row angles,
produce **4,564 matrices**. Every palette matches an independent kernel build;
matrix zero retains the supplied player root. Geometry hashes remain unchanged.
Three late failures on player 13 (nonfinite root, short pose and missing view)
preserve the entire prior output. Reports check current prerequisite source
hashes, private inputs, repeat logs and protected baseline hashes.

## Explicit inputs and limits

The fixture supplies synthetic translated/rotated roots, stored/perturbed angles
and the original polynomial coefficient bits. These roots are not verified
formation positions. Body variant 0 and skeleton target 0 remain explicit
fixture choices. Palette output is native float storage; publication into original
model-instance token layouts is not implemented here.

Fresh selected actors, head/hair and attachment scaling, the full `80162760`
caller, natural animation/root selection and a running menu-to-match recipient
remain unfinished. Captured actors already render through the native GX path;
this milestone does not claim freshly selected players are visible or playable.

Keep body owners at stable addresses while their visual links are in use.
Close visual owners before body/skeleton owners. All originals, derived catalogs,
builds and diagnostics remain private on W:.
