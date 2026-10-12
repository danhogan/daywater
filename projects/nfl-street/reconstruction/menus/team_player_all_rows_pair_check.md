# Selected PLAY body/head geometry coverage

`verify_team_player_all_rows_pairs.py` exercises the native resolver and
body/head inspection path for every nonzero-`DIGT` PLAY row enumerated from
`db_teams/00001.bin` through `db_teams/00032.bin`. This matches the existing
catalog procedure; it does not assume a PLAY row's `DIGT` equals its DB
filename. The prior resolver check covers 402 actual rows and 2,416 original
HSH1 lookup results. The body binding check establishes the bounded
TBYP-to-PLYRBODY key and descriptor variant mapping.

Two independent runs covered all 402 unique `(team_id, DIGP)` identities.
Both runs decoded a body and selected head for every row, and the shared-pose
gate passed for every pair:

| Result | Count |
| --- | ---: |
| Body key 0 | 65 |
| Body key 1 | 175 |
| Body key 2 | 162 |
| Body decode failures | 0 |
| Head decode failures | 0 |
| Unsupported TBYP values | 0 |
| Unresolved head branches | 0 |
| Shared-pose mismatches | 0 |

The run rendered only one representative pair per body key. These are flat
inspection captures; the combined candidate uses stored common coordinates
with no extra placement transform. The original menu's body variant selection,
actor attachment, live animation, hair/equipment, materials, and rendered
pixels remain unverified. The shared-pose gate supports a candidate geometry
composition, not original-runtime equivalence.

The native checker, TSV rows, representative images, and hash report remain
under `W:\Daywater\research-temp\nflstreet-team-player-loader\all-rows`.
