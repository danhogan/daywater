# Complete selected player records

`player_record.c` reconstructs original `800DE000`, the writer of the 0x68-byte
player record at `802AAA60 + slot*0x68`, and the `800DE6C8` side byte. Earlier
work (`gear_texture.md`) covered the PLAY-derived fields. This adds the
team fields, the 24-byte color state used by recoloring (`palette_recolor.md`)
and the side flag, so the record is now fully native. Given the match's GAME PLAY row, all
14 captured records are rebuilt byte for byte. TEAM, LCGP and GEAR rows come
from the on-disc template DB.

## Source trace

Game: NFL Street GameCube GNNE69 revision 0. DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`; contexts from Dolphin 2603a.

- **Caller.** `800DF868` calls `800DE6C8(player, actor+1363, actor+1360,
  actor+135C)` for each match player. The actor object comes from
  `80041E18` (`r13-7BBC` array). `+135C` is the player's GAME DIGP and `+1363`
  the record slot (equal to the player index in the captured match).
- **Side, `800DE6C8`.** Side is 1 when the `r13-6F68` byte (`8010E498`) is
  set; otherwise it is `player > 6` when the `8014C268(2)` word (`8030B1FC`)
  is nonzero, else `player < 7`. The captured match has byte 0 and word 1, so
  players 0–6 get 0 and 7–13 get 1. No meaning is claimed for the inputs.
- **Databases.** Cursor `802AB010` is the GAME database (`EMAG`) PLAY table.
  `8014E800` positions it on DIGP. The team cursor opens TEAM in the STAT
  database (`TATS`, via `8015082C`'s `r8`), and `80285CD4` is the STAT LCGP
  cursor. The STAT tables in RAM are byte copies of `template/00002.bin` rows.
  The captured STAT TEAM table also has a 44th row, a created team from save
  data (`DIGT` 0, `PYTT` 5).
- **`800DE000(slot, -, id, side)`** in order:
  1. `ITGT` of the PLAY row selects the TEAM row with equal `DIGT`.
     `80150A38` copies `1CMT/2CMT/3CMT` to `+5F/+60/+61`. `+3E` = `LGLT`
     (`sth`). `+3C` = `LDTC != 0` when `PYTT` is 5, else 1.
  2. `8014F60C` and the getters write `+00..+0A`, `+0C..+3B` and `+40..+4B`
     (see `gear_texture.md`). `+64` = side.
  3. `8014F9D8`: `+5E` = PLAY `1AHP` (tag at `r13-69A0`) + 0x80, as a byte.
  4. `8014FB4C` looks for an LCGP row with the player's DIGP. When one exists,
     18 LCGP fields fill `+4C..+5D` and the defaults are skipped. Otherwise:
  5. `800DD64C` reads the 18 selectors at `802717BC` (`13 14 F1 F1 13 14 15 13
     14 15 13 13 13 13 13 14 13 14`). Selector 0x13–0x15 copies `+5F..+61`,
     F0 writes 0x75 and F1 writes 0x7F; any other value leaves the byte. Then
     `+62` = 0x75 and `+63` = `+60`.
  6. `800DD6CC` runs only for side 0: `+52` = `+51`, `+62` = `+5F`, `+51` =
     `+50`, `+50` = 0x75 (`800DD6C4` returns 0x75).

Bytes `+0B`, `+3D` and `+65..+67` are not written; `+65` belongs to
`800DE3B8` (`gear_texture.md`).

**Correction to the earlier roster corpus.** The match's player rows are
GAME PLAY rows (renumbered DIGP 0–22, 12 for team 19 and 11 for team 1). In
the captured match their `8014F60C` fields equal template `00002` STAT rows,
not the `db_teams` blocks. That explains the `+30/+34/+38` difference noted in
`gear_texture.md`. The `db_teams` blocks also carry different team colors and
`1AHP` values. The gear and slot corpora still use `db_teams` rows as
controlled inputs; those inputs are not match claims.

## Verification

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus \
  python3 projects/nfl-street/reconstruction/native_assets/verify_player_record.py
```

**Verified against live original state.** For each of the 14 captured
players, the check does the following:

1. Reads the actor's DIGP and slot and the side inputs.
2. Finds the GAME PLAY row.
3. Poisons every byte `800DE000` writes in a copy of the record.
4. Rebuilds it natively.

All 104 bytes equal the live record, using both the disc and the live STAT
TEAM table.

**Verified controlled original comparison, per process.** 4,318 calls of the
actual `800DE6C8` run on the live GAME, STAT and GEAR databases. Only
synchronization is stubbed. The calls are:

- The 14 captured players.
- 23 GAME rows × all 44 STAT teams (ITGT rewritten in the RAM row) × four
  side-input pairs.
- 256 `1AHP` cases.

Every record matches the native one in all 104 bytes. The 3,956 cases with a
disc team give identical native results with the disc and live TEAM tables.
Three contexts repeat twice; native output is deterministic across two fresh
processes.

**Sensitivity check.** Six deliberate faults were each rejected:

- Missing side-0 swap.
- No `1AHP` bias.
- `LDTC` ignored; only the original comparison catches this, through the
  created team.
- Swapped `1CMT`/`2CMT`.
- Wrong `+63` source.
- Shifted side rule.

**Fixture inputs, not native data.** The snapshot supplies these:

- The GAME PLAY table, located through its RAM header at `804B1194`, whose
  schema must equal template `00001`.
- The live STAT TEAM table (`80482FB4`) with the created-team row.
- Actor ids and slots, and the side inputs.

The RAM header layout (records +0, record bytes +C, bits +10, allocated +58,
used +5C, field count +62, fields +64) is a candidate. It is consistent
across three tables and the disc schemas.

**Not modeled; native rejects instead:**

- A matching LCGP row. Its 18 fields are queried through the PLAY cursor's
  handle; the only disc row has DIGP 32767.
- A PLAY `ITGT` with no TEAM row.

Private report:
`W:\Daywater\research-temp\nflstreet-player-record-20261012\verification.json`.

## Remaining connections

- The GAME PLAY table is now built natively from disc STAT data, the
  selected teams and the lineup (`game_roster.md`). The 14 captured records
  also rebuild from that native table. This gate still reads the live GAME
  table as its fixture.
- Save-data created teams.
- `800E2BE8`/`801EBD38` material binding, the slot-1 face/cap choice and
  fresh actor startup.
