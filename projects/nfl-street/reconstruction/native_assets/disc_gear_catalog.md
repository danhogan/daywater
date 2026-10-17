# Disc-backed gear catalog

Host composition boundary for NFL Street GNNE69 North America revision 0,
DOL SHA1 `46e3de854998a0f66c9be8c72a1164d22cd29eb4`.
No original boot, emulator or natural match startup behavior is reconstructed.

`nflstreet_disc_gear_catalog_open_disc` reads the pinned DOL through the existing
nod metadata route. `open_dol` accepts already-owned matching bytes. Both enforce
revision and structural section validation, serialize only initialized DOL spans
into the existing 2924-byte GTC4 layout, and open the existing catalog parser.
The source layout is documented by `export_gear_texture_catalog.py`; addresses
and lengths are metadata, and no proprietary payload is compiled into source.

Pass an empty zero-initialized owner. Failure preserves output. Successful open
owns the serialized bytes and catalog views; input buffers may then be released.
Keep the owner alive while its views are borrowed by gear/player/surface/match
consumers. Do not shallow-copy owners. Close after those consumers, with
`nflstreet_disc_gear_catalog_close`; repeated close is harmless. No shared
parser, exporter, live loop or Daywater backend is changed.

Gate from checkout root:

```sh
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_disc_gear_catalog.py
```

`NFLSTREET_GATE_FAST=1` runs context 15 once, 12 cases and no mutants. Full mode
runs contexts 14/15/16 twice, 400 varied consumer/lifetime cases per process,
and seven parallel production mutants. Reports use unique subdirectories of
`/mnt/w/Daywater/research-temp/nflstreet-disc-gear-20261017/`; prior evidence is
never overwritten. The gate checks complete serialized bytes against the
independent private Python DOL export, every parsed field, template access,
actual GEAR record conversion with real database rows and varied retained fill,
failed opens, occupied outputs, source release, independent owners and repeated
cleanup. Context RAM only seeds consumer input variation: this is **not** an
all-RAM oracle comparison or new verification of consumer algorithms themselves.
Mutants alter actual production revision/ownership checks or source offsets.

Integration is additive: open this owner before passing `&owner.catalog` to
`nflstreet_gear_records_build`, color/dynamic-palette creation, player-record
construction, or `nflstreet_player_surface_context.gear`. Retain it through
those borrowers' teardown. The material catalog, initialized resources,
mutable actor allocations and eventual live startup join remain separate work.
