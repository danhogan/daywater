# Owned disc slot-material catalog

Host preparation/composition boundary for NFL Street GNNE69 revision 0, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. The source-backed addresses,
record sizes, pointer-following rules and SMC1 order come from unchanged
`export_slot_material_catalog.py`. Confidence is verified only when the full gate
report matches current source/input hashes. This is not original boot execution,
emulator integration, or a fresh-match startup claim. No new RAM field is inferred.

`nflstreet_disc_slot_materials_from_dol` requires the pinned revision and builds
owned SMC1 bytes, then opens the unchanged existing material catalog parser.
`nflstreet_disc_slot_materials_from_disc` uses the existing nod DOL reader first.
Every followed span is checked against initialized DOL sections; wide address
arithmetic prevents wrapping. Names need a terminating byte at index 1..15,
settings terminate within 64 entries, descriptors are capped at 64 with exactly
one texture, shader names must match and stage counts are 1..8. These are format
bounds; the exact public revision pin remains stronger than structural checks.

Initialize `nflstreet_disc_slot_materials owned = {0}`. Call either constructor
before existing slot-material binding/startup consumers; replace the borrowed
SMC1 view with `&owned.catalog` in `nflstreet_slot_materials_bind`,
`nflstreet_slot_materials_configure` and `nflstreet_actor_materials_hook`.
Constructors reject nonempty destinations and leave every output field untouched
on failure. The input DOL can be freed immediately on success. Keep `owned` alive
while any consumer borrows material/shader/descriptor names; call
`nflstreet_disc_slot_materials_destroy(&owned)` afterwards. Destroy clears the
struct and is repeatable. Do not shallow-copy ownership. No shared/live consumer
is changed by this standalone adapter.

Gate (from checkout root):

```sh
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_disc_slot_materials.py
```

Private output: `/mnt/w/Daywater/research-temp/nflstreet-disc-materials-20261017`.
The independent Python exporter supplies the whole-byte reference. Three saved
contexts, each in two processes, seed 201 varied consumer cases per process;
the contexts supply randomized test seeds, not an original-module oracle.
Each case verifies pinned DOL ownership after source overwrite, exact bytes,
all catalog views, existing configure writes and actor hook fields/stages.
Additional structural fixtures vary original settings and descriptor pointers
against explicit independent serialized reference edits. Fourteen malformed
structural fixtures are exercised by test-only inclusion of the private builder;
the public API always rejects modified revisions before extraction. Seven
production mutations are required to fail comparisons. Fast mode runs context
15 once, 11 cases and no mutants. These checks complement rather than replace
the existing original-module slot-material reconstruction gate.
