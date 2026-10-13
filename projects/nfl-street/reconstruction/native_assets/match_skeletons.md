# Fresh face/hair skeleton binding for selected lineups

`match_skeletons.c/h` composes the verified skeleton startup and face/hair
postattachment stages for all 14 simultaneous `match_visuals` owners. Each
player gets six independently owned initialized source resources. The fixture
uses target index 0 for faces and 5 for hair, as in the preceding binding checks.
These are explicit fixture inputs. It does not establish natural actor target
selection or claim that the original allocates six resources per player.
Duplicating the source resources here is a native host isolation strategy.

## Transaction and lifetime

Binding mutates model skeleton rows, clusters and name-table fields. The host
therefore clones each player's face/hair buffers privately, stages all 84 source
resources, and binds every clone before replacing any live visual geometry.
A failure closes staged copies and skeletons, preserving both caller outputs
and every existing geometry byte. A successful commit replaces the face/hair
buffers; bind before publishing their pointers to an actor or renderer.
Surfaces, material storage and borrowed body geometry remain unchanged.

Per-player logical namespaces contain source skeletons, embedded view identities
and a temporary allocator arena. Namespaces cannot overlap each other, existing
face/hair/texture/material resources or borrowed body resources. Decoded skeleton
sizes are checked against the reserved namespace and view/allocator ranges.
These are host identities, not recovered RAM addresses.

The bounded allocator tracks live blocks and releases all temporary storage.
It also records failed allocation attempts. This matters because the original
postattachment stage can clear the name table and return success when its first
map allocation fails. The host transaction requires complete binding and rejects
that fallback without committing its private mutations. This strict host failure
contract is not claimed to reproduce the full original caller's failure policy.

Keep the skeleton owner alive while preparing/using the bound visual state.
Close visuals before skeletons. Skeleton close can repeat. Body skinning, actor
allocation, animation channels and the player draw's actor/instance state are
separate consumers; they are not produced by this owner.

## Verification — 2026-10-13

**Verified native composition** from the actual retained Win32 menu selection:

- 14 selected players, 84 fresh initialized resources, 28 face views, 42 hair views.
- 210 temporary callback allocations and 210 releases.
- Every bound file/view, fresh skeleton file/token and base vector equals an
 independent construction through the previously verified stages.
- Five guards: overlapping namespaces; exhausted temporary storage on player 14;
 invalid target; decoded skeleton exceeding its namespace; occupied output.
- Failure checks compare owner structures and hashes of all geometry files/views
 and borrowed body bytes, including failures after the first 13 players staged.

The gate requires current source-matched original skeleton-startup, face-binding
and hair-binding evidence, plus the selected aggregate visual proof. It repeats
in two fresh Windows processes and rechecks the six prior visual-owner guards.
No new full original 14-player startup, body-binding comparison, live Dolphin
integration, GPU pixels, sanitizer or actor creation is claimed. The original
DOL/DLL/runner and all private inputs remain unchanged.

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus \
  python3 projects/nfl-street/reconstruction/native_assets/verify_match_skeletons.py
```

Private report:
`W:\Daywater\research-temp\nflstreet-match-skeletons-20261013\verification.json`.
Game revision and original evidence procedures remain documented in
`skeleton_startup.md` and `hair_postattach.md`; the gate fingerprints the
original face-bound-resource report and its source directly.

## Next connection

The new `player_draw` submission reconstruction supplies the rendering target.
Body copies/skinning, skeleton-to-actor constructor inputs, model instances,
matrix palettes and the running menu match recipient still need joining before
these selected players can be drawn and animated. The independent scene still
uses reference actors. This loading/binding fixture is not playable startup.
