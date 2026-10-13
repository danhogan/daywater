# Selected lineups with private bound bodies

`match_bodies.c/h` replaces the 14 visual owners' borrowed body references with
fresh owned body files/views. Each body uses that visual's verified manifest
TBYP and existing explicit variant, plus a caller-supplied skeleton target.
The selection fixture retains variant 0 and uses target resource 0; natural
LOD/variant/target selection remains unverified.

All 14 files are decoded, relocated and bound before any visual pointer changes.
A late failure frees staged bodies and preserves both the original visual data
and output. After success, the owner is copied to caller storage before visual
body pointers are set to its inline file/view fields. Keep that owner at a
stable address while those pointers are used. Close visuals before bodies;
keep skeleton sources alive through actor preparation. Prior shared bodies
are borrowed and are not freed or mutated by this operation.

Per-player logical namespaces exclude existing bodies, face/hair, owned texture
and material spans, all 84 skeleton source resources and their face/hair views.
View and bounded temporary ranges are checked; decoded model extent must fit
the reservation. Temporary allocation errors and incomplete release sequences
reject. These logical tokens are host reservations, not original RAM addresses.

## Verification — 2026-10-13

**Verified native composition** from the actual retained Win32 menu selection:

- 14 private relocated/bound body files/views with 42 released callback allocations.
- All published body pointers reference their corresponding stable owner entries.
- Each file/view equals an independently constructed owned body at the same tokens.
- Five guards: overlapping namespaces, exhausted temporary storage on player 14,
 invalid target, decoded body exceeding its namespace and occupied output.
- Failure checks preserve owner structures and hashes of face/hair files/views
 and existing body bytes, including failures after the first 13 players staged.
- Double close and the earlier visual/skeleton resource gates also pass.

Two fresh Windows processes repeat. Current original body-postattachment evidence
(all 12 type/variant cases in three contexts repeated twice) and selected skeleton
composition must have matching source fingerprints. This does not compare an
entire fresh original actor or run its natural LOD/target policy. Body binding
reorders bone/cluster references; it does not create frame-dependent skinning
matrices, model instances, animation channels or actor state.

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus \
  python3 projects/nfl-street/reconstruction/native_assets/verify_match_bodies.py
```

Private report:
`W:\Daywater\research-temp\nflstreet-match-bodies-20261013\verification.json`.
Protected original DOL/DLL/runner and private input hashes remain unchanged.
Decoded bodies, builds, saved RAM and logs stay on W:; no game bytes are embedded
in these owners.

## Remaining

Create actor/model instances and skinning matrix palettes from this owned setup,
then feed the existing native player-draw/renderer and the running menu's match
recipient. The rendering milestone currently consumes captured actor state.
The menu-selected setup now owns geometry and materials; it is still a loading
check, not visible interactive fresh match startup.
