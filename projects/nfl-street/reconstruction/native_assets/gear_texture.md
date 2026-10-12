# Selected gear texture plan

`gear_texture.c` reconstructs original `800DE3B8`, which turns one selected
player's appearance record into the 25 texture slots handed to `800E2E48`.
It joins the record's 12 gear IDs to the fresh GEAR table, then builds
`PLATEX_*` texture keys and `PLADYNCLUT_*` palette keys. It also applies the
fixed-index and color-request paths and the default and slot-0 fallbacks.
Native code reads private archives and a private catalog exported from the
DOL. Nothing at runtime needs a snapshot, the DOL or the original module.

This continues from `selected_appearance_assets`. The face texture
(`800E3104`) remains separate, and so does the cap path (`800E318C`). This
milestone supplies the gear/body texture slots that sit beside the face.

## Source trace

Game: NFL Street GameCube GNNE69 revision 0. DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Comparison contexts came from
Dolphin 2603a; this is an offline module oracle, not a new live test.

`800DE3B8` (callers `800DF5CC`, `80041378`) does the following:

1. It fills two 25-word arrays (indices, palettes) with −1, then seeds
   indices 19–22 with `0x623, 0x57B, 0x624, 0x57C`. The cached decompilation
   splits these arrays incorrectly; the disassembly at `800DE3D4`–`800DE444`
   is authoritative.
2. It sends the `802717CE` channel/value list to `800E217C`.
3. For logical indices 0–11 it runs `8014BC04` (category) and `8014B8A0`
   (search where GRTI=category and GEID=record word `+0C+4*i`). Then
   `8014B918` builds a 112-byte GEAR record, `800DD720` builds texture keys,
   and `800DDB10` produces color lists.
   - Logical 10 also calls `800DDD2C`, which writes slots 16/17 through
     `800DDBE8`. GRSI 11 sets a flag.
   - Logical 0 calls `800DDBE8` with kind byte `+0A` into slot **18**. The
     decompiler shows this as scratch, but the destination is `sp+0x58` (indices[18]) and `sp+0xC0`
     (palettes[18]).
4. `800DD974` fills each still-empty slot that has a default part name as
   `PLATEX_<part>_<part><+40>`. `800DD914` then rewrites slots 19–22 from
   record bytes `+04/+05` after slots 4 and 7.
5. It calls `800E2E48(slot, i, index, palette, 0)` for i=0..24, then
   `800E3020(slot, record+4C)`.
6. If slot 0 is still −1, `800E3218(+44)` returns a fallback index with
   palette `0x7A`, followed by a second slot-0 `800E2E48`.
7. If the flag is set, it sends three requests: `(0,FF)`, `(17,FF)`, `(15,FF)`.
8. It stores record `+65` = 0 when `+46` is 11 or 12 or the flag is set;
   otherwise 1.

`800DD720` key order, after truncating each key at its first space:

- Palette: if GEAR `+64` (GDCI) is nonzero and the name differs from
  `NONE` (case-insensitive), it looks up `PLADYNCLUT_<GBST>` in the `r13-5094`
  table; otherwise the palette is −1.
- Category 6 (logical 6/7): if part flag 10 is set, it looks up
  `PLATEX_<GBST>_<+40 %02d>_HAND` and writes slot 9 or 10.
- Otherwise, for each slot whose selector is not 27 and whose part flag is
  set, it tries keys in this order:
  1. `_HEAVY_AWAY_<part>` (when `+64`==0 and `+4B`==0)
  2. `_AWAY_<part>` (when `+64`==0)
  3. `_HEAVY_<part>` (when `+4B`==0)
  4. `_<part>`

  The first lookup that hits wins; a full miss leaves −1.

The captured contexts register `platex.dat` HSH1 (1,735 hashes) at
`r13-5ACC` and `pladyncl.dat` HSH1 (129) at `r13-5094`. This was checked
byte-for-byte against the archives.

## Field provenance

Record = original `802AAA60 + slot*0x68`; GEAR = 112-byte `8014B918` output.
All multi-byte values are big-endian. Four-letter PLAY names follow the
native manifest spellings (for example IKSP = constant `494B5350`); GEAR
names follow `headwear_state.md`.

| Field | Type/size | Source | Readers | Confidence |
|---|---|---|---|---|
| Record `+00..+0A` | u8 ×11 | `8014F60C` PLAY tag constants by offset: 4C554C50, 4C574C50, 524F4850, 50544550, 54415250, 4F544150, 544C5250, 4F544C50, 44465450, 44425450, 43444850 | `800DDB10` (+00/+01 bits), `800DD914` (+04/+05), `800DDD2C` (+08/+09), `800DE3B8` (+0A) | Verified: 402 rows against original `8014F60C` |
| Record `+0C..+3B` | u32 ×12 | `8014F60C` PLAY query tags at `80285C2C` | `8014B8A0` GEID key per logical index | Verified (same) |
| Record `+40` | u16 | `800DE000`: `8014FC3C` (IKSP) + 1 | `800DD720` HAND key, `800DD974` defaults | Verified: original getter and `sth` |
| Record `+42` | u16 | `8014EC90` (PLHT) | Not used by this plan | Verified getter |
| Record `+44` | u16 | `8014ECB8` (PHAR; 0x3F→0x0C) | `800E3218` | Verified getter, including mapping |
| Record `+46` | u16 | `800DDEBC` headwear state | `+65` decision | Verified: original `800DDEBC` |
| Record `+48` | u8 | `800DDF44` (GRTI=1, GEID=`+10`; GRSI 1D..21 → 1..5, else 0) | Not used by this plan | Verified: original `800DDF44` |
| Record `+49/+4A/+4B` | u8 | `8014EC68` PLEX / `8014F050` PJEN / `8014EC40` TBYP | `+4A` → `800DDBE8` kind 1; `+4B` HEAVY test | Verified getters |
| Record `+3C`, `+3E`, `+4C..+63`, `+64` | u8, u16, 24 B, u8 | `800DE000` team/STAT queries and caller side flag (`800DE6C8`) | `800DDBE8` kinds 2–5, `800E3020`, AWAY test | Captured values; writer not reconstructed |
| GEAR `+00/+04/+08` | u32 | GPKY / GRTI / GEID | Search key | Verified: 282 rows × 2 fills |
| GEAR `+0C`, `+2D` | 31 B + NUL | GRNM / GBST text (type 0, width 248), truncated at first space | `+2D` is the key name | Verified (same) |
| GEAR `+4E..+5D` | u8 ×16 | Part flags (tags at `80285968`) | `800DD720` selectors | Verified (same) |
| GEAR `+60/+64/+68/+6C/+6D` | u32/u32/u32/bool/bool | GRSI / GDCI / zero / GRHB≠0 / GRHF≠0 | `800DDB10`, `800DDD2C`, `800DDA20` | Verified (same) |

Missing PLAY tags read as zero. `801BFFDC` returns status 9 before writing,
and `80149614` returns its zero-initialized slot. The on-disc PLAY table has
no CDHP, so `+0A` is 0 for every roster row. The captured records also show
0 there. Tag spellings are not given gameplay meanings.

**Draft corrections.** An earlier uncommitted draft had three faults that its
own oracle couldn't catch, because the native code and the oracle hook shared
the same modeling:

- It fed record `+40` from CFRP; the original reads IKSP.
- It wrote GEAR text fields without the type-0 terminator.
- It covered only `800DD720`, so it missed the slot 18–22 seeding.

The new oracle fills every query item with the original copy routines, so
the terminator behavior now comes from original code.

**Candidate observation, not a gate:** four of the 14 captured records
(three from team 19 and one from team 1) match their on-disc PLAY rows in
every player-derived field except words `+30/+34/+38`. Original `800DE334`
rewrites record query words after creation, which is consistent with that
difference. The match-time writer is not traced here.

## Verification

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus \
  python3 projects/nfl-street/reconstruction/native_assets/verify_gear_texture.py
```

The gate exports the private catalog from the hash-checked DOL. It builds
both programs, runs the native check twice, and runs the original comparison
twice in each of contexts 14, 15 and 16. Logs must be identical within each
pair, and protected/private input hashes must not change.

**Verified native coverage, per process.** The corpus is all 402
regular-roster PLAY rows in two team contexts, taken from captured records 0
(side 0) and 7 (side 1). It also includes the 14 captured records verbatim
and 112 controlled variants of them, for 930 plans in total.

- 282 GEAR records, 124 of them joined.
- 16,429 slot indices across 23 slots; slots 16/17/18 are written
  664/535/98 times.
- 3,889 palettes, 930 slot-0 fallbacks and 26,538 color requests.

The controlled variants set bytes `+00/+01/+08/+09/+0A`. They exist to drive
the slot-18 and channel-list branches; they are not roster claims.

**Verified against live original state:** for the 14 captured players
(captured record *i* is player *i*), the native plans equal the live `800E2E48`
records (`802799EC + player*600 + slot*24`) in the same snapshot. That covers
index and palette in all 336 non-face slots, including the slot 0 fallback
and the seeded slots 19–22. The live −1 values come from `800E341C`
cancellation (see `slot_textures.md`), and they coincide with plan −1.

**Verified controlled original comparison, per process:**

- 282 GEAR records × 2 prefill patterns against original `8014B918`.
- 402 PLAY rows: original `8014F60C`, six `800DE000` getters, `800DDEBC` and
  `800DDF44`, totalling 12,462 field comparisons.
- All 256 `800DDBE8` kinds × 4 argument sets.
- All 65,536 `800E3218` inputs.
- 930 complete runs of the actual `800DE3B8`. Every `800E2E48` argument
  (24,180 slot writes, in order) matches, and so do the `800E3020`
  arguments and ordering. All 26,538 `800E217C` requests match in order,
  as do 11,160 GEAR searches and the final `+65` byte. The rest of the
  record stays unchanged.

The original HSH1 lookup, formatting and string routines execute. The three
contexts repeat twice each.

**Sensitivity check.** Eight deliberate faults were each rejected by the
oracle:

- The three draft faults.
- Swapped `800DDBE8` kind-5 palette.
- Dropped logical-0 slot 18.
- Swapped GRHF/GRHB.
- Swapped GRSI 0xD bits.
- Swapped `800DD914` byte order.

**Supplied services.** The `801B1AC8` table search is supplied from fresh
rows, but each item is filled by the original `801C4388`/`801B94C8` with the
`801BFFDC` terminator. Synchronization (`801AF944/9CC/57C`) returns.
`80041E18` returns a controlled object. The `800E217C`, `800E2E48` and
`800E3020` effects are captured as arguments only; color state, request
records, `800E2EB8/800E2EFC` refreshes and GPU behavior are not executed
here. `face_texture.h` already reconstructs the literal `800E2E48` record
write.

Private report:
`W:\Daywater\research-temp\nflstreet-gear-texture-plan-20261012\verification.json`.
It records sources, builds, the catalog hash, the two archives, the GEAR
block, 32 roster blocks, the snapshots and repeated logs. All assets,
catalogs and diagnostics stay on W:.

## Remaining connections

Native code now knows which texture/palette entry each selected player's 25
appearance slots request. Still open:

- The `800DE000` team/STAT queries (`+3C/+3E/+4C..+63`), which come from
  captured values here.
- The side-flag source (`800DE6C8`).
- `800E217C` color-state application.
- `800E2E48` request records feeding `800E27F8` acquisition and
  `80164CE4` application for these slots.
- Material binding, and the fresh actor startup that consumes all of it.

No selected-player draw or playable match is claimed.
