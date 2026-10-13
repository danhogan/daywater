# Joined selected-player visual resources

`selected_player_visual.c/h` owns selected face and hair geometry, borrows the
processed body variant, and joins these with all 25 selected texture slots and
their 45 material instances. It uses `selected_player_geometry` and
`player_surface`, preserving their existing behavior and lifetimes. This is a
host ownership type, not a recovered original object layout.

The input completed player record belongs to the match/player-record owner and
must describe the supplied team/player identity. The surface's face name comes
from the geometry manifest. Archives, body pool and catalogs remain borrowed.
The join rejects overlap of its destination, scratch or reserved material-token
ranges with either face file, any hair file or any borrowed body asset **before
calling an upload callback**. Open stages resources and publishes only on full
success; occupied output rejects. Close releases the surface and geometry and
can be repeated. Token identities are caller-chosen namespaces, not discovered
RAM addresses. Successful callback side effects cannot be rolled back.

## Verification

**Verified host assembly, GNNE69 revision 0.** Run:

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus \
  python3 projects/nfl-street/reconstruction/native_assets/verify_selected_player_visual.py
```

Each fresh Windows process checks 402 disc roster rows × three explicit body
variants = **1,206 joined assemblies**. Geometry manifests, processed face/hair
bytes and views, borrowed body identities, complete texture destinations and
material instances equal the separate established owners. There are 45 named
face-template fallbacks. Nine cross-owner token collisions reject before any
upload; a rejected upload preserves output; occupied-output rejection is tested
for every assembly and close repeats.

The gate checks current source fingerprints of passed face-texture, player-
surface and GAME-roster evidence. Those constituent gates contain original-
module/live-state comparisons. This new gate adds native ownership composition,
not a new original rendering or gameplay comparison. Protected original hashes
and private input hashes remain unchanged; two fresh process traces agree.

Private report:
`W:\Daywater\research-temp\nflstreet-selected-visual-20261013\verification.json`.

## Scope of player records

The corpus uses a **controlled native color context**, generated from a native
GAME roster and complete player record using the disc template. Each selected
roster row then supplies its gear/player fields through the existing getter
reconstruction. It does not assert that this context contains that selected
team's final colors or skin palette. No captured RAM is required by this check.

Attempts to feed local `db_teams` PLAY rows directly into the complete match-
record path, or equate them with template STAT PLAY rows, failed input checks.
Direct local-table-to-GAME conversion also failed the existing roster builder's
input restrictions. These experiments are not a verified loader interpretation.
Do not equate those database namespaces or silently substitute a reference
record when connecting the runtime. Fresh selected-team ingestion and the
completed record handoff need their own original-backed verification.

## Remaining connections

- Feed the join completed records from the actual native menu/team/lineup path.
- Attach the geometry to owned skeletons and actor registries at startup.
- Submit model materials with original matrix routing and OnePass/TwoPass TEV
  behavior to the independent renderer.
- Replace reference scene actors and connect match scheduling/gameplay.

A selected player has not been drawn by this gate; a fresh playable match is
not claimed.
