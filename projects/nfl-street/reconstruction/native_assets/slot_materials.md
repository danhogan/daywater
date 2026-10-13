# Selected slot material binding

`slot_materials.c` reconstructs the step that attaches each player's 25 owned
slot textures to named materials. Original `800E2BE8` builds a GX texture set
for every slot destination and records it in the per-player binding table
`80276C6C`. Actor startup then applies the default material settings
(`80164DD8`) and hooks the bound textures onto the 45 player materials
(`801C7528`/`801EBBA8`/`801EBC78`). For all 14 captured players, the native
texture sets equal the live game's objects except for fields written later
while drawing. All 630 live actor material instances match the native
hookup.

## Source trace

Game: NFL Street GameCube GNNE69 revision 0. DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`; contexts from Dolphin 2603a.

- **Binding table `80276C6C`.** 14 players × 25 slots × 32 bytes; it ends
  exactly at the request records `802799EC`. Each entry holds a DOL material
  name pointer (HAIR, HEAD, TORSO, SHOULDERPAD, BICEP, BICEPSWEATBAND, ELBOW,
  FOREARM, WRIST, LEFTHAND, RIGHTHAND, THIGH, KNEE, UPPERSHIN, LOWERSHIN, SHOE,
  DECALFRONT, DECALBACK, DECALHAT, TATOOBICEPL/R, TATOOFOREARML/R, HAT,
  GLASSES) and four zero words. `+14`, `+18` and `+1C` are written by binding.
- **`800E2BE8`** (match creation, `80041A08`) creates the `8017B570` loader
  pool (`r13-5AC8`; not modeled). It then calls `801EBD38(destination, entry,
  6, 0)` for each player (`r13-71F0`) and slot. `801EBD38` writes the
  following:
  - `+1C` = destination.
  - `+18` = a 0x28-byte texture set (`8018B7B8`).
  - It runs `801C8E80` on the set.
  - `+14` = set `+24`.
- **`801C8E80`** first relocates the MMAP with `801C5610`, a no-op on owned
  destinations. With n images (`+0C`) and p palettes (`+10`), it allocates in
  this order:
  - the set;
  - entries (n × 0x10, set `+20/+24`);
  - texture objects (n × 0x2C, `+10/+14`);
  - palette objects (p × 0x28, `+18/+1C`);
  - a name table (n × 8, `+00/+04/+08`; bytes `+0C/+0D` = 0).

  A count of 0 allocates nothing.
  - **Palettes, first (`801C8BF8`/`801C956C`).** Flush data for entries×4
    bytes, where entries = 256 if the size exceeds 0x7F, else 16.
    - Format 6: word 1, then a 16×16 `GXInitTexObj`.
    - Otherwise: word 0, then `GXInitTlutObj` (`80205DF4`), then `+24` =
      size.
  - **Entries and names (`801C8B48`).** Each entry holds: the MMAP name
    record (0 for version 0), the set, the image index, the row's palette
    index and first halfword, and 0xFFFF. The name table pairs each name
    record with its entry.
  - **Texture objects (`801C93D4` → `801C9464`).** In order:
    1. Store mips, width, height and size.
    2. Flush the `GXGetTexBufferSize` bytes (`80205530`).
    3. Run `GXInitTexObj` (`80205754`), or `GXInitTexObjCI` (`802059C8`) for
       formats 8/9, with REPEAT wrap and mipmap when mips > 1.
    4. Apply LOD twice: min filter 5 (3 for CI), mag 1, bias 0, max LOD
       mips−1 (`801C926C` → `GXInitTexObjLOD` `80205A10`).
    5. Set wrap REPEAT on axes with a power-of-two size, else CLAMP
       (`80205BA4`).
- **`80164DD8(table, 0)`**, called from actor startup `801639C8` on the
  player's table. It walks the default list `8028DD90`, one entry per
  material:
  - The slot is found by name (`801EBB44`).
  - A set clamp byte (only the three DECAL materials) makes every texture
    object CLAMP/CLAMP (`801C9208`).
  - Then `801C9348` reapplies LOD with the entry's filters and bias. Every
    entry uses min 3, mag 1 and bias −1.3, which gives the live bias field
    0xD7.
- **Draw-time writes.** `801C7AD4` loads a texture for drawing:
  `GXLoadTlut`, `GXInitTexObjTlut` and `GXLoadTexObj`. It writes the register
  ids into texture-object words 0–3, the TLUT name at `+18` and the TMEM
  offset into the palette object. These are not part of binding.

The SDK float constants are 16, ±4/3.99, 32 and 0/10, and the min-filter
map at `r13-6090` is `0 4 1 5 2 6`. They are used exactly (single-precision
products, truncating conversion).

- **Material hookup in actor startup `801639C8`.** All three body models'
  views get `+18` = `8028DD10` from `801C6AA0`, which remaps the body
  materials by name (`player_model_binding.md`). That is the DOL list of 45
  material descriptors at `8028D4A0`, each with a material name (eyeL, bicepL,
  …), a shader name (OnePass or OnePassTX) and one texture name (HEAD,
  BICEP, …). The shaders are set by `801EBE14`: `r13-5638` = "OnePass" (stage
  "base", index 0) and `r13-5634` = "TwoPass" ("base" 0, "over" 1). The steps
  are:
  1. `801EBDD4` allocates the 45-pointer array at actor `+FEC`.
  2. `801C7528` creates an instance per descriptor (`801C718C`). Its stage
     index word comes from `801CCB50`: −1, since the `r13-63F8` list is null.
  3. `801EBBA8` finds each descriptor's texture name in the binding table
     (`801EBB44`) and calls `801EBC78` twice. The first call puts the slot's
     first texture object and, when the first entry has palettes, its palette
     object into stage "base" (`801C9088`/`801C90AC`/`801C705C`). The second
     sets the OnePass shader (`801C7150`).
  4. Eight explicit `801EBC78` calls follow, for materials 10/11, 19/20,
     14/15 and 23/24 with tattoo slots 19, 20, 21 and 22. When the tattoo slot
     has a texture, the shader becomes TwoPass. `801C6E28` rebuilds the stage
     array: stage 0 is kept and stage 1 is zeroed. Stage "over" then gets the
     tattoo texture and palette.

## Native API

The private catalog is `slot-materials.smc1` (from
`export_slot_material_catalog.py`). It holds the 25 slot names and the
25-entry default settings list.

- `nflstreet_texture_set_build` is `801EBD38` for one destination. It takes an
  allocation service that returns tokens and their current bytes, because the
  original reads and keeps allocator contents in some fields:
  - set `+0E/+0F`;
  - texture objects `+22/+23`;
  - palette objects `+0E..+23`;
  - bits 21–23 of palette word `+8`.
- `nflstreet_slot_materials_bind` runs it for the 25 slots
  (`800E2BE8`).
- `nflstreet_slot_materials_configure` runs `80164DD8` with the catalog list.
  A list name without a slot rejects.

- `nflstreet_actor_materials_hook` gives the final 45 instances: material
  and texture names, shader (OnePass or TwoPass), stage count and stage
  entries {texture object, palette object, index}. A descriptor without a
  slot rejects, because its startup-registry state is not modeled. The
  catalog also carries the 8028DD10 list and both shader definitions.

## Verification

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus \
  python3 projects/nfl-street/reconstruction/native_assets/verify_slot_textures.py
```

Binding runs inside the slot-texture gate, on the destinations that gate
builds (`slot_textures.md`).

**Verified against live original state.** All 350 texture sets of the 14
captured players match, including the binding-table words and names, the
set, entries, texture objects, palette objects and name tables. Three
exclusions apply:

- Bytes the original leaves as allocator contents, found by building twice
  with fills 0x00 and 0xFF.
- 1,036 register-id bytes, 56 TLUT-name bytes and 315 TMEM-offset bytes from
  the draw-time `801C7AD4`.
- Live addresses are used as allocation tokens.

**Verified controlled original comparison, per process.** These runs use the
actual `801EBD38` and `80164DD8`:

- **20,475 selected texture sets** (819 players × 25), each with fills 0x00
  and 0xFF. Every block byte, allocation token, size and flush order matches,
  as do the binding-entry words.
- **513 synthetic unrelocated MMAPs**, which the original relocates itself.
  They have 0–3 images and 0–2 palettes, all GX texture formats, odd and
  power-of-two sizes, 1–4 mips and palette formats 0/1/2/6 around the 0x7F
  threshold, plus the empty set.
- **819 default-settings runs** on the selected sets.
- **64 runs with random settings lists** (1,312 entries): min filters 0–5,
  both mag filters, biases beyond and at ±4/3.99, the flag bytes and
  anisotropy.

- **819 runs of the original hookup sequence** on fresh 45-entry arrays.
  It runs `801EBDD4`, `801C7528`, `801EBBA8` and the eight tattoo calls with
  the snapshot's shader registry. Every instance's shader, stage count and
  stage words match. The variants rotate per player: as-is, empty tattoo
  slots (→ OnePass), palette-less entries and rotated slot names. The live
  check also covers all 630 instances of the 14 captured actors (112 of them
  TwoPass).

Three contexts repeat twice.

The selected data has exactly one image and one palette per set; the sweeps
cover the rest.

**Sensitivity check.** Fourteen deliberate faults were each rejected. Five
are in the hookup (palette condition, swapped tattoo slots, a stage dropped
on rebuild, index 0 instead of −1, TwoPass for an empty tattoo). The other
nine are:

- Mip-chain size.
- Flipped wrap rule.
- Palette 0x7F threshold.
- Palette-word allocator bits.
- Format-6 palette path.
- Rounded LOD bias.
- Missing CI filter remap.
- Swapped clamp/edge bytes.
- Missing decal CLAMP.

Private report:
`W:\Daywater\research-temp\nflstreet-slot-textures-20261012\verification.json`.

## Remaining connections

- Drawing: the OnePass/TwoPass shader (TEV) setup and the draw path that
  consumes these instances (`801C7AD4` and its callers).
- The `8017B570` loader pool.
- Fresh actor startup owning these states end to end.

No selected-player draw or playable match is claimed.
