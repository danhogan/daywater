# Selected texture palette recoloring

`palette_recolor.c` reconstructs the original dynamic-palette step, `800D9C0C`.
After a slot texture is copied into its destination, a palette request names
a `pladyncl.dat` entry. That entry's operations recolor palette record 0 using
the player's 24 color-state ids. With this step, the owned slot textures of
all 14 players in the captured match equal the live game's memory byte for
byte. Native code reads the archives, the template DB and the private GTC4
catalog. Nothing at runtime needs a snapshot, the DOL or the original module.

## Source trace

Game: NFL Street GameCube GNNE69 revision 0. DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`; contexts from Dolphin 2603a.

- **Color table, `80148A88` (boot).** It queries template DB tables `CPAL`
  and `CPHR` with the 13 field tags at `8028575C` (an id plus 12 color bytes).
  CPAL id *n* fills `8030A898 + 16n`; CPHR id *n* fills entry
  `(n+0x80)&0xFF`. Each 16-byte entry holds four RGB levels at +0/+4/+8/+C in
  the order of the `80271494` level names (PRIMARY, SHADOW, MIDTONE,
  HIGHLIGHT); the fourth byte of each is not written. The data has 128 CPAL
  and 12 CPHR rows, which gives 140 entries.
- **Op lists, `800D9AF0` → `800D99E8` → `800D8918`/`800D8680` (boot).** Each
  `pladyncl.dat` entry is a text document `<dynclut version><operations><op .../>`.
  Each `<op>` becomes 8 bytes: kind (u32), range start, range end, color, level.
  - `colornum` (or else `colorname`, matched case-insensitively against the
    24 names at `8027132C`, 15 bytes each) gives the color.
  - `level` (or else `levelname`, one of the four names) gives the level;
    the default is 0.
  - `range` is either `a` or `a-b`, parsed with `8017CFE8`. Attribute text
    is read into 15-byte buffers.
  - `type` selects the kind: `set` gives 0 with a color or 1 with `palindex`;
    `blendfrom` gives 3 or 5; `blendto` gives 4 or 6; `blend` gives 2;
    `mult` gives 7.
  - A reversed range is swapped, and blendfrom/blendto swap with it.

  The table pointer is `r13-5B48`, with one `{count, ops}` pair per entry.
- **Composition, `800D9C0C(record0, entry, 802ABE6C+player*24)`.** It builds
  24 entries from the color state (`80148D60` copies a color-table entry for
  each id). Then `800D9A40` runs each op through `800D9948` on a copy of
  palette record 0 (`+2` format, `+4` size, `+8` data). Finally `800D8A9C`
  flushes the palette data (`801F0BD0`) and calls `8020380C`, which writes
  GX FIFO padding. The color state is record `+4C..+63`, copied by `800E3020`
  during `800DE3B8`. The live value equals the record for every captured
  player.
- **Operations (`800D9948` dispatch).** Format 1 is RGB565. Format 2 is
  RGB5A3, and when writing it keeps the destination entry's alpha bit and
  4-bit alpha. Other formats write nothing.
  - 0 sets a color to every entry in range; 1 copies palette entry *c*.
  - 2 blends the range ends.
  - 3 blends from the color to the end entry; 4 from the start entry to the
    color.
  - 5 and 6 are the same as 3 and 4, but use palette entry *c* instead of a
    color.
  - Blends compute `(a*(n-k)+b*k)/n` per channel with signed division,
    clamped to 255.
  - 7 multiplies palette entry *c* by 2×color/255 cumulatively and writes the
    running value. It uses byte +6 as both the color id and the palette index.

## Verification

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus \
  python3 projects/nfl-street/reconstruction/native_assets/verify_slot_textures.py
```

**Verified against live original state.** These all come from the captured
match:

- The native color table equals the 140 live `8030A898` entries.
- Native parsing equals all 129 live `r13-5B48` op lists (722 ops).
- All 350 slot destinations of the 14 captured players are byte-identical to
  live memory, 70 of them after recoloring.

The data uses kinds 0, 1, 2, 3, 4 and 6; no blend has a zero-length range.

**Verified controlled original comparison, per process:**

- 3,072 `800D9948` cases: all 8 kinds × formats 1, 2 and 3 × 128 random
  palettes, colors and ranges, including no-op reversed ranges.
- 4,163 compositions by the actual original `800D9C0C`, inside the full
  slot pipeline for 819 players. Final destination bytes match, and so do
  85,244 cache flushes in per-slot order (palette flushes included).

The native-built color table and op lists are installed for the original. The
three contexts repeat twice.

**Sensitivity check.** Four deliberate faults were each rejected:

- Swapped kind-6 weights.
- 5A3 alpha bit taken from the new color instead of the old entry.
- Missing range-reversal kind swap.
- Shifted level index.

**Not modeled:**

- The original's behavior for unknown op types (left as allocator contents).
- Color ids ≥ 24 (a null-pointer read).
- Zero-length blends (division by zero).
- Missing color-table fields (stale query items).

Native code rejects each of these instead; none occurs in the data. The
`8020380C` GX FIFO padding is a host GPU concern.

## Remaining connections

The selected players' texture memory is now complete and matches the
original. Still open:

- The color state and side flag are now native (`player_record.md`); this gate
  still feeds them from captured records. The match GAME database that
  supplies player rows is not yet built natively.
- `800E2BE8`/`801EBD38` material binding of the destinations.
- The slot-1 face/cap choice for roster rows.
- Fresh actor startup and drawing with these textures.
