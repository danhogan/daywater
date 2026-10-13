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

## Native match-record entry point

`nflstreet_selected_player_visual_from_match` receives a native GAME roster and
row, the exact borrowed STAT database used to build it, and an explicit original
side input. `source_rows` resolves the original STAT row; DIGT/DIGP select its
geometry, while `player_record_build` builds its full record from the GAME row
and STAT TEAM/LCGP tables. The record supplies color state and gear decisions to
the surface. No captured record or reference color fixture is substituted.

The roster now retains its source PLAY view as borrowed provenance. The helper
rejects another database buffer, altered schema/counts, an invalid GAME row or
an unsupported side value. Source bytes must remain unchanged and alive while
the roster is consumed. TEAM/LCGP are parsed from the same database; callers
cannot accidentally supply another team's tables.

## Verification

**Verified native composition, GNNE69 revision 0.** Run:

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus \
  python3 projects/nfl-street/reconstruction/native_assets/verify_selected_player_visual.py
```

Each fresh Windows process now checks **508 STAT players × three explicit body
variants = 1,524 joined assemblies**, with no named-face fallback. It builds 480
controlled team-pair rosters (including repeated pairs as source rows change
teams), then uses the exact `source_rows` mapping to construct a complete player
record and select geometry for every tested row. Side input is explicitly 0 in
this corpus; this does not establish the runtime's side/controller mapping.

Geometry manifests, processed face/hair bytes and views, borrowed body
identities, complete texture destinations and material instances equal the
separate established owners using those same complete records. Nine cross-owner
token collisions reject before upload; rejected upload, invalid row, side 256
and a byte-identical but different source buffer preserve output. These are
**13 explicit guards**. Every occupied output rejects and close repeats.

The GAME-roster original gate was rerun after adding borrowed source provenance.
It still matches the 23-row captured table and all 14 captured records, with
87 original loads per process across three saved contexts repeated twice; 86
controlled pairs produce 2,032 native complete player records per process.
The visual gate requires current source-matched face, surface and GAME-roster
reports. This is native ownership/record composition, not a new original player
rendering or live gameplay comparison. Two fresh visual processes agree;
protected original and private input hashes remain unchanged.

Private report:
`W:\Daywater\research-temp\nflstreet-selected-visual-20261013\verification.json`.

## Correction to the earlier controlled-color corpus

The earlier 402-row corpus joined local `db_teams` geometry with a controlled
native color context. It did not establish selected-team uniforms. The current
entry point and gate replace that limitation with complete native GAME records
and geometry resolved from their actual source STAT rows.

Private file inspection shows why the earlier direct handoff failed:

- Template `00002` STAT PLAY has 508 rows, 68 fields and an 88-byte stride.
  It contains roster data, not just an empty scaffold.
- `db_teams/00001` PLAY has 12 rows, 65 fields and an 84-byte stride. It lacks
  GAME fields PHDC/ITGT and uses narrower widths for four gear fields.
- Player 19 has local DIGT 1 in that fallback file, but STAT DIGT/ITGT 22;
  player 337 has STAT DIGT/ITGT 1. Local fallback membership cannot replace
  actual STAT membership.

These are verified static file observations for the private revision. The
original `801E1628` source and existing original-load gate distinguish copying
an already loaded STAT team from the separate `801E0110` fallback load, which
the existing gate intentionally rejects. Reconstructing that absent-team loader
is still unverified. Do not silently treat fallback tables as loaded STAT rows.

## Remaining connections

- Connect the native menu's selected teams/lineup to these roster/visual inputs.
- Attach geometry to owned skeletons and actor registries at startup.
- Submit materials with original matrix routing and OnePass/TwoPass TEV behavior
  to the independent renderer.
- Replace reference scene actors and connect match scheduling/gameplay.

A selected player has not been drawn by this gate; a fresh playable match is
not claimed. Custom/team-career rows and the absent-STAT-team fallback branch
remain outside this corpus.
