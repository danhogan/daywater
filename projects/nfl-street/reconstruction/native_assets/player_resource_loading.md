# Remaining fresh-player resource ownership

The connected factory and player/model-binding owners accept resource views
through explicit callbacks. Their original-module checks still supply those
views from reference state. The next loading milestone is to build the same
views from independently decoded assets and the selected roster.

## Reuse existing work

`menus/team_player_resolve.c` already selects an exact PLAY row by DIGT/DIGP
and resolves face and roster-hair keys. `menus/team_player_body_binding.c`
already decodes a bounded selected base-body entry from the plamodel TERF
archive. Its controlled original checker verifies all nine entries:
normalized TBYP key 0..2, variant 0..2, index `1 + 3*key + variant`, with TBYP
7 aliasing key 2. The menu's actual variant choice and complete resource
registration are still open. See [selected-player loading limits](../menus/team_player_loader.md).

These APIs identify and decode resources; they do not construct every pool
record, relocated model view, material/palette binding or animation resource
needed by a fresh match actor. The captured `NAP1` render profile also remains
a reference profile, not a selected-roster appearance loader.

## Original acquisition routes to preserve

**Probable static-source contract, not a fresh native loading verification:**
`800E0AC4` acquires adjacent indices from the pool at r13-5AD8, registered as
plaface by `800E09A8`, then calls `801EB3F4` with `PLYRMODEL`. It is not the
plamodel base-body acquisition route. Its cache and resource operations remain
services in the owned model-binding initializer.

`800DFBE4` initializes the three `PLYRBODY` records rooted at 802AB1E8.
`800E0734` selects resource pool records and delegates to `801EB6C4`.
For raw pool selectors 4 and 5 it ignores the key argument and uses the single
records at 802AB29C and 802AB2B0; the variant still selects a descriptor. Other
selectors have their own strides and limits. Do not replace these calls with
a universal body-key lookup.

`801EB6C4` returns record+10's descriptor-array token plus `variant*50+1C`,
only when `0 <= variant < record[+0C]`. These are hex offsets. Model binding
copies the resulting 13-word view, owns index-buffer creation/remapping, and
still supplies actual archive acquisition and rendering operations.

The source listings are cached locally under DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Confidence for the route facts above
is **probable** from static call and table readers; selected-body entry mapping
has its separate verified controlled checker. No new verified live RAM field
or fully loaded player is claimed here.

## Acceptance for the next milestone

Use a selected roster row and original local archives to create bounded resource
views without copying them from saved RAM. Compare acquired records, constructor
descriptor, allocated actor state and relevant bindings to the original path.
Keep decoded assets, source tables, captures and comparisons private on W:.
Then connect those owned resource views to the existing creation API and scene.
