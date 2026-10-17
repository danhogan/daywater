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

## Remaining integration

The retained match currently borrows its resource inputs. A subsequent bounded
owner should retain these catalog sources through `match_visuals` and
`match_skeletons` teardown, then exercise those aggregate constructors with the
new inputs. No existing retained-match entry point or live loop has switched
inputs in this slice. Mutable globals, allocation pools, actor initialization
and the remaining startup owners still require their planned gates.

The RAM-derived animation-set inspection catalog used by explicit
`NFL_LIVE_HOST_LOCOMOTION` is a different NSC1 format. It remains outside the
fresh startup contract. It must not be substituted for the skeleton catalog.
