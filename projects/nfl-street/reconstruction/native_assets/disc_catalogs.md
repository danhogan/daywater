# Disc-backed constants consumers (plan row 03)

The three existing immutable DOL catalog formats now have additive, owned
disc-backed inputs. Confidence is **verified within the host byte preparation
and native consumer composition boundaries below**, for NFL Street GNNE69
North America revision 0, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`.

| Input | Owner and existing consumer | Full gate evidence |
| --- | --- | --- |
| Skeleton NSC1 constants and animdata | `skeleton_disc.c` copies the four initialized DOL spans and owns the archive; `nflstreet_skeleton_disc_startup` feeds unchanged `skeleton_startup.c`. | Six fresh processes, 201 cases/process, 1,206 complete resources/process, seven production mutants caught. |
| Gear texture GTC4 | `disc_gear_catalog.c` builds an owned 2,924-byte catalog; its view feeds unchanged gear record/template consumers. | Six fresh processes, 400 consumer/lifetime cases/process, complete exporter-byte equality, seven production mutants caught. |
| Slot materials SMC1 | `disc_slot_materials.c` follows bounded initialized DOL pointers into owned bytes; its view feeds unchanged parser/configure/hook consumers. | Six fresh processes, 201 cases/process, complete exporter-byte equality, fourteen malformed fixtures/process, seven production mutants caught. |

Each full gate uses contexts 14/15/16 twice. Snapshots only seed test inputs;
the production adapters consume the owner's disc/DOL, without exported catalogs
or saved RAM as runtime inputs. Consumer comparisons use independent private
Python exports. These results do not extend the prior original-module proofs
of consumer algorithms or establish original boot execution.

The public constructors enforce the full DOL revision pin. Gear/material owners
reject occupied outputs and preserve output on failure. Their input DOL may be
freed after construction; their catalog views and names remain borrowed until
owner teardown. Skeleton startup outputs own all six resources independently
of the source, so the source may close after successful construction. See the
individual module documents for exact APIs and failure contracts:

- [Skeleton source](skeleton_disc.md)
- [Gear catalog](disc_gear_catalog.md)
- [Material catalog](disc_slot_materials.md)
- [Pinned DOL loading and host memory preparation](disc_constants.md)

Private full reports, all on W:

- `research-temp/nflstreet-skeleton-disc-20261017/verification.json`
- `research-temp/nflstreet-disc-gear-20261017/run-56f2107c2888/verification.json`
- `research-temp/nflstreet-disc-materials-20261017/verification.json`

The manager also links and opens all three adapters together in the private
`research-temp/nflstreet-disc-catalogs-manager-20261017/` check. Its scope is
coexistence and ownership, not retained-match or live gameplay acceptance.

## Verified retained aggregate join

The [retained-match input owner](match_disc_inputs.md) now retains all three catalog
sources and derives GEAR records, colors and dynclut tables from the retained
session and new catalog. It calls unchanged `match_visuals_open` and
`match_skeletons_bind`, keeps borrowed material names alive, and closes visuals
before skeletons and input owners. Real callbacks are queued until both stages
succeed; a rejecting real callback can leave earlier external side effects.

The full native composition gate passed in six fresh Windows processes. Each
compared three complete aggregates against independent exported-catalog inputs:
42 players, 252 bound resources, 350 catalog/name queries and 201 varied GEAR
record cases. Late visual, binding and callback failures, namespace conflicts,
occupied outputs and distinct per-player callback contexts are checked. All
eight production mutants were caught. The existing visual and skeleton baseline
gates also passed before the join. Evidence is under
`research-temp/nflstreet-match-disc-inputs-20261017/run-dd254489cd3e/` on W:.

Row 03 data ownership is complete for the current catalog consumers within
this byte/composition boundary. Session, resolver, stable body pools, headwear
and other asset archives remain explicit borrowed prerequisites. Mutable globals,
allocation pools, actor initialization and other startup owners still require
their planned gates. Final live entry belongs to rows 28/29; the live loop has
not switched inputs here.

The RAM-derived animation-set inspection catalog used by explicit
`NFL_LIVE_HOST_LOCOMOTION` is a different NSC1 format. It remains outside the
fresh startup contract. It must not be substituted for the skeleton catalog.
