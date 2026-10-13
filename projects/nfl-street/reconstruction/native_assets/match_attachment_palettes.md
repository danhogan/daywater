# Owned selected face and hair palettes

`match_attachment_palettes.c` generates matrices from the 14 selected players'
private, bound face and hair models. The two face files and three hair files are
processed separately, retaining per-part counts. Absent resources stay absent.
Each supplied part pose contains root, optional model-row angles, original scale
slot, LOD and scale. All outputs commit together after complete success.

Confidence: **verified native composition, source preservation and rollback**.
The [cluster arithmetic](cluster_palette.md) has an original body-model gate;
the [scaling blocks](attachment_palette_scale.md) have an independent original
caller-block gate. This aggregate check does not compare the complete original
face/hair actor startup or prove automatic role assignments.

## Reproduction

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_match_attachment_palettes.py
```

Private report: `W:\Daywater\research-temp\nflstreet-match-attachment-palettes-20261013\verification.json`.
The actual retained native menu selection produces **70 owned face/hair models
and 442 matrices**. Two fresh Windows processes match separate per-part kernel
and scaling composition, preserve geometry hashes and exercise two late failures
on player13. All earlier selected body, skeleton, visual and body-palette checks
run in the same fixture. Reports pin current prerequisite evidence, inputs,
sources, logs and unchanged original baseline hashes.

The fixture uses stored model angles, synthetic translated roots, scale1.125,
LOD player-index modulo3 and scale-slot part-index modulo4. These are explicit
coverage inputs, not a discovered face/hair-to-actor-slot mapping, original
formation positions or animation policy. The fixture initializes destination
matrices from scratch before applying the independently verified disjoint-area
scaling behavior. Natural aliasing/copy policy remains open.

## Next connection

Actor/model-instance construction, original slot assignment, head root transform,
pose mapping and destination token publication still need to feed the native
draw. The full `80162760` caller, sleeve/hand transforms, variant eligibility and
natural frame scheduling remain unfinished. Captured actors already draw on the
native field; fresh selected actors are not yet visible or playable.

Keep original assets, prepared catalogs, builds and diagnostics private on W:.
Keep body owners at stable addresses and close visual owners before resources.
