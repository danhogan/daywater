# Original attachment matrix scaling

`attachment_palette_scale.c` reconstructs four scaling blocks inside original
`80162760`, using explicit original model-slot identities. It keeps scratch and
staged destination matrices separate. These slots' appearance roles are not
inferred from their offsets.

Game: NFL Street, GNNE69 revision 0; DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`; reference captures Dolphin 2603a.
Confidence: **verified for these controlled caller blocks and disjoint matrix
areas**, not the complete actor caller or natural slot selection.

All blocks use the float at actor +0x1358, PPC `fmuls`, and skip translation
components 3/7/11. Scaling runs for supplied LOD inputs below 2.

| Original model slot | Selection | Matrix area and copy |
|---|---|---|
| +0x5A8 | Cluster indices >=1, row byte0 ==13 | Scale scratch, then copy. LOD>=2 skips both. |
| +0x788 | One cluster: index0; otherwise indices >=1 with byte0 ==13 | Scale destination at +0x1030; subsequent scratch copy overwrites it when areas are disjoint. |
| +0x968 | One cluster: index0; otherwise indices >=1 with byte0 ==13 | Scale scratch, then copy. |
| +0x698 | One cluster: index0; otherwise every row with byte0 ==0 | Scale scratch, then copy. |

The assembly confirms that +0x968's single-cluster path writes E0000000. The
+0x788 asymmetry is retained. Aliased original destination/scratch layouts have
not been tested; the native API requires disjoint arrays. Counts 1..255 are
supported: original multi-cluster cursors wrap to eight bits, so 256 is rejected.
Nonfinite input/result and malformed lengths reject without committing either
array. Matrix generation, root/angle selection and role assignment are external.

## Reproduction and evidence

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_attachment_palette_scale.py
```

Private report: `W:\Daywater\research-temp\nflstreet-attachment-palette-scale-20261013\verification.json`.
Three saved contexts, repeated in two fresh Windows processes, execute the
untouched original caller blocks with supplied rows and matrix contents. Each
process checks 480 runs: four slots, four LOD inputs, five cluster counts
(1/2/7/23/255) and six scale inputs, including non-exact floating products.
All **27,648 matrices** match bit for bit in both scratch and final destination.
Two failure guards preserve output. The fixture provides E0000000 external
memory, returns from model generation and performs the requested scratch copy;
it does not emulate locked-cache hardware or claim whole-caller RAM equality.

An initial exploratory fixture entered slot5A8 after its LOD gate and exposed
an incorrect unconditional-copy assumption. The gate now starts before that
branch and confirms preservation at LOD>=2. No original source/module changed.
Source/input/executable/log and protected baseline hashes are recorded.
