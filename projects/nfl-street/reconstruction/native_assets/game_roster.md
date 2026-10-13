# Match GAME PLAY table

`game_roster.c` builds the match's GAME database PLAY table, the rows that
`800DE000` reads for each player (`player_record.md`). It uses the on-disc STAT
tables, the two selected teams and the lineup. The live GAME table of the
captured match is rebuilt byte for byte from disc data. All 14 captured
player records follow from it.

## Source trace

Game: NFL Street GameCube GNNE69 revision 0. DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`; contexts from Dolphin 2603a.

The game keeps its data in databases driven by SQL-like statement strings
(`801B4510`): STAT (`TATS`, template `00002`), GAME (`EMAG`, empty layout in
template `00001`) and others. Match start `801EA480` calls
`801E3828(0x07000000)`, `801E3720(0x04000000)` and then
`801E05A4(0x07000000, 0x03000000)`:

- **Team loader.** `801E05A4` → `801E1C84` → `801E1628` walks the GAME
  selected-teams table MTTS (`declare … from 'MTTS' order by 'UNLS'`).
  - Before the load, each row has DIGT = DIOT = the STAT team id, and AMDT
    holds a loaded-tables mask. The loader copies only tables that are not
    yet in AMDT.
  - With flag 0x01000000 and DIGT = DIOT, the team gets the next free GAME
    TEAM id (0, then 1), written back to MTTS DIGT.
  - PYTT 5 (created) teams are skipped here.
  - A team missing from STAT is loaded from `db_teams.dat` (`801E000C`,
    `801E0110`).
- **Copy, `801E78C4(…, from=DIOT, to=new id, 'TATS', 1, mask, flags)`.**
  The tables are `8029516C`: THCD, PLAY, TEAM, keyed by DIGT. For one team,
  each table's rows go through a temporary table, are renumbered and
  inserted.
  - **Renumbering, `801E7154`.** A per-database map gives each source DIGP a
    new id the first time it is seen: first free slot, linear probing, ids
    from a prepared list. The DIGT key maps to the new team id.
  - **Order.** The team's THCD rows are visited before its PLAY rows, each in
    ascending source DIGP. Players without a THCD row are therefore numbered
    after the rest of their team: team 12's DIGP 88 and 123, team 15's 10 and
    team 17's 432.
  - **Storage.** PLAY rows are stored in ascending source DIGP.
  - **Fields.** Copied by tag into the GAME schema. STAT's `LILP` bit is
    dropped; GAME TEAM adds `TEFT` = 0. ITGT, DIOP and the lineup flag GILP
    are copied.
- **Lineup.** The menu has already written GILP into the STAT rows: 1 for
  the seven side-0 starters and 2 for side 1. Apart from those 14 values, the
  live STAT PLAY rows are byte-identical to the disc rows; the save's created
  team adds seven appended rows. The GILP writer is not traced here, so the
  native API takes field overrides from the caller.

## Native API

`nflstreet_game_roster_build` takes:

- STAT PLAY, THCD and TEAM tables.
- The GAME schema (`nflstreet_db_schema_open` on template `00001`).
- The two team ids in MTTS UNLS order.
- STAT field overrides (the lineup GILP values).

It returns an owned `nflstreet_team_table` view of the GAME PLAY rows, with
the source row of each. `nflstreet_player_record_build` consumes it directly.
These are rejected:

- A team without exactly one STAT TEAM row.
- A PYTT 5 team.
- A GAME field without a matching STAT field.
- An override that matches no copied row.
- More rows than the GAME table allocates (110).

Only the verified two-team case is accepted.

## Verification

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus \
  python3 projects/nfl-street/reconstruction/native_assets/verify_game_roster.py
```

**Verified against live original state.** From disc STAT data, the live
MTTS team order (19, 1) and the 14 live lineup values, the native table
equals the live GAME PLAY table in all 23 rows (88 bytes each). The 14
captured player records rebuild from it byte for byte.

**Verified controlled original comparison, per process.** There are 87 runs
of the actual `801E3828`/`801E3720`/`801E05A4`: the live pair plus 86
controlled pairs. In the controlled pairs, every disc team is paired with
the next and the seventh-next team. The lineup is set on each team's first
seven players, and one player's DIOP is rewritten. In every run:

- The resulting GAME PLAY table equals the native one byte for byte (2,032
  rows in total).
- MTTS DIGT becomes the team slot.

The original also rebuilds the live GAME TEAM, THCD and MTTS tables byte for
byte, but this is observed in exploration only; native code builds just the
PLAY table. Three contexts repeat twice.

**Sensitivity check.** Four deliberate faults were each rejected:

- PLAY-only numbering (no THCD pass); only the original comparison catches
  this.
- Team id instead of slot in DIGT.
- Ignored lineup overrides.
- Rows stored in source storage order.

**Supplied state.** Each original run restores the pristine snapshot and sets
the inputs:

- MTTS rows (DIGT = DIOT = IRDT = team, PYTT/DIGC/DIGD/NSID from the team,
  AMDT = 0).
- STAT lineup and DIOP values.
- GAME PLAY and THCD emptied (`used` = 0). The clear before a fresh match is
  not traced.

The `db_teams.dat` open is marked done, and `801E0110` would fail the check.
Synchronization returns. Everything else, including the statement
interpreter, temporary tables, renumbering and inserts, runs in the original
module.

Private report:
`W:\Daywater\research-temp\nflstreet-game-roster-20261012\verification.json`.

## Remaining connections

- The menu's lineup writer (GILP) and team selection into MTTS. The native
  menus already retain both choices.
- GAME TEAM/THCD rows natively, created teams (save data) and `db_teams.dat`
  teams.
- `800E2BE8`/`801EBD38` material binding and fresh actor startup.


## Borrowed source provenance — 2026-10-13

The host roster retains `source_play`, the exact borrowed source table used to
build it. Source bytes/schema must remain unchanged and alive until the roster
and its consumers are done; close does not free that source. The selected visual
match entry point uses this view plus `source_rows` to reject another database
and resolve original identity for geometry. This is host lifetime metadata, not
an original GAME structure field. The original roster gate was rerun after the
change and retained all prior output comparisons.
