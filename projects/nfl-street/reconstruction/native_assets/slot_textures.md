# Owned selected slot textures

`slot_textures.c` turns a selected player's 25 texture-slot requests (from
`gear_texture.md`, plus the separate face request for slot 1) into owned,
relocated texture destinations. It follows the original boot-to-match
pipeline. Native code reads the private archives and the private GTC3
catalog. Nothing at runtime needs a snapshot, the DOL or the original module.

## Source trace

Game: NFL Street GameCube GNNE69 revision 0. DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`; contexts from Dolphin 2603a.

- **`800E2958(count)`** runs once, guarded by `r13-71F8`. For each slot it
  reads the template request index from the initial `802799EC + slot*24`
  record (DOL data, now exported in GTC3) and its decoded size (`801A824C`).
  It stores that size in `802ABFBC[slot]`. Each player's destinations are laid
  out consecutively in one `8018B7E4` allocation:
  `capacity = size + 32 - (size & 31)`, so an aligned size still gains 32.
  The resulting pointers go in `802AB8F4[player*25+slot]`. Records get
  `+4` player, `+8` slot, pending `+0C`=1, `+0D`/`+0E`=0 and palette −1. The
  largest template size becomes the loader scratch size (`r13-71EC`).
- **`800E2AF0`** (boot) decodes each template into player 0's destination
  (`801A75B0`), copies it raw to other players, then relocates every
  destination (`801C5610`).
- **Match creation `80041A08`** calls `800E2BE8` (material binding of the
  destinations, not reconstructed here), then `80041378` per player. That
  builds the record and runs `800DE3B8`, the face (`800E3104`) or the cap
  (`800E318C`); both of the last two write only slot 1. Then come
  `800E2CAC` and `800E341C`.
- **`800E2CAC`** collects pending slots, sorts them (`801AC670`), allocates
  scratch, and calls `800E27F8` for each. A request identical to the previous
  one reuses the scratch contents. `800E27F8` does the following:
  1. Decodes the requested entry into scratch and relocates it there.
  2. Runs `80164CE4(destination, scratch)`.
  3. For slot 0, calls `800E34B8`, which is empty.
  4. If the palette is not −1, runs `800D9C0C` on destination palette
     record 0 (`801C5958(dest,0)`) with the player's 24-byte color state at
     `802ABE6C`.
  5. Clears `+0C..+0E`.
- **`800E341C`** cancels requests that are still pending and not in flight,
  setting index/palette −1. This explains −1 values seen in live records
  after earlier front-end previews; it does not change destination bytes.

In a fresh boot-to-match, every slot is still pending from `800E2958`, so a
slot whose request is −1 reloads its template with palette −1. The `800E2E48`
early return ignores any palette supplied with −1.

## Verification

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus \
  python3 projects/nfl-street/reconstruction/native_assets/verify_slot_textures.py
```

**Verified against live original state.** For all 14 players in the captured
match:

- Native plans equal the live `800E2E48` records in all 336 non-face slots
  (this check is in `verify_gear_texture.py`).
- All 350 destination tokens and the 25 template sizes equal the live
  `802AB8F4`/`802ABFBC` values.
- All 350 slots are byte-identical to the live destinations once recoloring
  (`palette_recolor.md`) runs; 70 of them are recolored. Before recoloring
  existed, 281 matched, and the other 69 differed only inside palette
  record 0's data.

**Verified controlled original comparison, per process.** 819 players are
compared: 14 captured, 1 controlled, and 402 roster rows × 2 captured team
contexts. Each one runs the actual original pipeline:

- `800E2958`: layout, sizes, record fields and scratch size.
- `800E2AF0`: every template destination.
- `800E2E48`: the 25 requests.
- `800E2CAC`/`800E27F8`: 18,860 loads, 85,244 `801F0BD0` flushes in
  per-slot order (palette flushes included) and 4,163 original `800D9C0C`
  recolorings, with exact arguments,
  final destinations and cleared records.

The controlled player puts palettes on −1 slots, and both sides ignore them.
Three contexts repeat twice. Native results are deterministic across two
fresh processes.

**Sensitivity check.** Skipping −1 slots is rejected. The capacity rule for
32-aligned sizes and stray palettes on −1 slots cannot occur in the real data,
which has no aligned template sizes and no −1 slots with palettes. The
controlled player covers the second; the first remains a statically traced
formula.

**Supplied services.**

- Archive acquisition is supplied: `801A824C` size and `801A75B0` decode come
  from fresh native decoding.
- Allocations return the compared tokens.
- Synchronization helpers are stubbed to return.
- `800D9C0C` runs in the original, using the native-built color table and op
  lists installed in RAM; its `8020380C` GX FIFO padding is counted only.

Relocation, sorting, the `80164CE4` copy and cache-flush requests all execute
in the original module.

Private report:
`W:\Daywater\research-temp\nflstreet-slot-textures-20261012\verification.json`.

## Remaining connections

- `800D9C0C` palette composition is now reconstructed in
  `palette_recolor.md`. With it, all 350 live slots are byte-identical, and the
  slot gate runs the original composition instead of recording it. The color
  state (record `+4C..+63`) still comes from captured team records.
- **`800E2BE8`/`801EBD38`** material binding of the destinations.
- The face/cap choice for slot 1 for roster rows.
- Fresh actor startup that consumes the result.

No selected-player draw or playable match is claimed.
