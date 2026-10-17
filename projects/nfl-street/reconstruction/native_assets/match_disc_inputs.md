# Retained match with owned disc catalog inputs

`match_disc_inputs.c` adds a host composition owner around unchanged retained
menu `match_visuals_open` and `match_skeletons_bind` constructors. Confidence is
candidate unless the private full report matches current source hashes, then
verified within this composition boundary. No original boot, emulator, actor,
rendering, all-RAM execution or gameplay claim is added.

## API and lifetime

`nflstreet_match_disc_inputs_open(iso, session, visuals[14], bindings[14], &out)`
requires `out == NULL`. It owns pinned disc gear/material/skeleton catalogs,
a second private animdata archive, reconstructed gear/color/dynclut tables,
14 visuals and 84 initialized skeleton resources. The duplicate animdata input
is an explicit bounded ownership choice: the existing skeleton source API stays
unchanged. Gear and material views/name pointers remain valid through visual
teardown. Caller-provided derived tables/catalog pointers are replaced locally;
gear records/colors come from the retained session STAT using the new catalog,
and dynclut comes from the common borrowed dynamic archive. All players must
share one stable body pool and dynamic archive.

The session, resolver and its resources, stable body pool, headwear, local hair,
textures/dynamic archives and flush opaque prerequisites remain borrowed and
must outlive this owner. Caller contexts are copied, not modified. Body/geometry
identity namespaces, record sides, body variants and binding target indices
remain explicit caller inputs, without inferred original semantics.

Const getters expose visuals, skeletons, gear and material catalogs. They
return NULL for a NULL owner. Close the opaque owner once with
`nflstreet_match_disc_inputs_close`: visuals first, skeletons next, then derived
tables and catalog/archive inputs. All exposed pointers become invalid. Close
accepts NULL. Never shallow-copy the owner or free an exposed view separately.

Both constructor stages finish before any real flush callback executes. An
outer queue retains each player's original callback/opaque pair and notification
order, because `match_visuals_open` itself drains callbacks before subsequent
binding. Failed construction preserves the output and releases staged owners.
If a real callback rejects, earlier real callbacks may already have external
side effects; those effects cannot be rolled back. Such failure still preserves
the output and frees staged resources.

## Gate

The gate uses the actual retained-menu QMS1 fixture and independent exported
NSC1/GTC4/SMC1 inputs to construct a reference through the unchanged APIs.
Disc-backed inputs must match complete visual data, material strings/fields,
all skeleton resource bytes/metadata and vector/view tokens. Snapshots seed
allocator fills only; production APIs receive no saved RAM or generated catalog.

Full mode runs contexts 14/15/16 twice in fresh Windows processes. Each runs
three full aggregate comparisons for explicit body variants 0/1/2 (42 players,
252 bound resources), 201 cheap GEAR record comparisons with varied dirty fill,
350 player/slot catalog-name lifetime queries, one occupied-output guard, three
late rollbacks (visual, binding, callback) and one namespace-conflict guard.
Ordered callback traces include each player identity to verify distinct opaque inputs.
These cheap counts are separate from full aggregate counts. Fast mode uses
context 15 once, one full comparison and 11 cheap record cases, no mutants.
Eight production mutants target catalog/derived-table wiring, constructor side,
binding, deferred callbacks, per-player callback opaque identity and occupied-output validation. Crashes/build
failures never count as catches; comparison exit 4 and diagnostic are required.
Recursive quoted includes and compiled C sources, provider files, private inputs,
prior stage reports, executable and mutation evidence are hashed.

```sh
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 -B projects/nfl-street/reconstruction/native_assets/verify_match_disc_inputs.py
```

Private builds, logs, fixture and reports stay under
`/mnt/w/Daywater/research-temp/nflstreet-match-disc-inputs-20261017/`.
The report records a unique run directory. No existing consumer, shared gate
or live code is changed. Other borrowed prerequisites and mutable match startup
remain outside this slice.
