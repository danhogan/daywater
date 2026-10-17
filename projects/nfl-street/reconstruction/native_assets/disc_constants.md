# DOL Section Loading + BSS Zeroing (Gate 03)

## Scope

Native reconstruction of GameCube DOL section loading for GNNE69 rev 0 (DOL SHA-1 `46e3de854998a0f66c9be8c72a1164d22cd29eb4`). Implements:

- `nflstreet_dol_parse_header`: Validates and parses the DOL header (7 text sections, 11 data sections, BSS, entry point)
- `nflstreet_dol_load_sections`: Loads all sections into a target MEM1 buffer at their linked addresses, zeroes BSS
- `nflstreet_dol_verify_revision`: SHA-1 verification against the pinned GNNE69 rev 0 baseline

All bounds checks enforced:
- Section offsets within DOL image (≥ 256, past header)
- Section sizes 4-byte aligned, addresses 4-byte aligned
- No overlapping sections (text/text, text/data, data/data, BSS/loaded)
- BSS does not overlap any loaded section
- Rejects truncated sections (extending past DOL image)
- Rejects wrong revision (SHA-1 mismatch)

## Verification Contract

**Gate script**: `verify_disc_constants.py`

**Private evidence directory**: `/mnt/w/Daywater/research-temp/nflstreet-disc-constants-20261017/`

**Private inputs**:
- `/mnt/w/Daywater/research-temp/nflstreet-GNNE69.iso` (DOL extracted from ISO, SHA-1 `813c4511ba3135059a13de424555647e8252254b`)
- `/mnt/w/Daywater/research-temp/position-neutral-{14,15,16}.mem1` (24 MiB MEM1 snapshots)
- ModernGekko recompiled module: `/mnt/w/Daywater/research-temp/nflstreet-native-windows-local/gGNNE69_recomp.dll`
- nod prebuilt: `/mnt/w/Daywater/research-temp/nflstreet-live-20261013/build/_deps/nod_prebuilt-src/`

**Fast mode** (`NFLSTREET_GATE_FAST=1`):
- One context (15), one repeat, 10 edited cases, no mutants

**Full mode**:
- Three contexts (14, 15, 16), two fresh Windows processes each
- 200 edited cases per context (201 total including saved-as-is)
- 7 parallel mutants built and run in separate directories
- All mutants must fail with comparison exit code 4 (not crash/build failure)

**Comparison**: All 24 MiB MEM1 RAM compared byte-for-byte outside the declared oracle stack `[0x80f46f00, 0x80f47000)`. Return value and all RAM must match.

## Native API

```c
// dol_sections.h
int nflstreet_dol_parse_header(const uint8_t *dol, size_t dol_size, nflstreet_dol_header *header);
int nflstreet_dol_load_sections(const uint8_t *dol, size_t dol_size,
                                 const nflstreet_dol_header *header,
                                 uint8_t *target, size_t target_size,
                                 uint32_t target_base);
int nflstreet_dol_verify_revision(const uint8_t *dol, size_t dol_size);
```

## What Is Not Modeled

- Original DOL loader entry point (not exposed in this revision; the game uses a different startup path)
- DVD async loading, streaming, or DVD timing
- DOL relocation (sections load at linked addresses; no relocation performed)
- Partial section loading or demand paging
- Multiple DOLs (only main.dol)

## Integration

Call `nflstreet_dol_parse_header` then `nflstreet_dol_load_sections` during early startup, before any consumer initializes. The target buffer should be the full 24 MiB MEM1 span at base `0x80000000`. The function zeroes BSS automatically.

No existing shared files modified. New files:
- `dol_sections.c` / `dol_sections.h`
- `disc_constants_original_check.c`
- `build_windows_disc_constants_check.sh`
- `verify_disc_constants.py`