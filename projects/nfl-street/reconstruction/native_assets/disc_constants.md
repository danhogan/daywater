# GNNE69 rev0 DOL preparation (row 03 bounded slice)

This is host memory preparation, not a reconstructed GameCube loader or an
original-module startup comparison. Evidence level: candidate unless the private verification report records a full
pass for the current source hashes; then verified within the byte contract below. GNNE69 rev0 main.dol SHA1 is
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`.

`nflstreet_disc_read_dol(iso, &bytes, &size)` uses nod's documented
`nod_partition_meta(...).raw_dol` route outside the FST, pins the full DOL SHA1,
and returns an owned copy (caller frees). It shares the existing disc reader's
nod provider; `disc_files.c` remains unchanged. Returned metadata is copied
before nod handles close. Failure leaves outputs untouched.

`nflstreet_dol_parse_header` reads the 256-byte DOL header: 7 text then 11 data
entries, offsets at 0, addresses at 0x48, sizes at 0x90; BSS at 0xD8/0xDC and
entry at 0xE0. Structural parsing rejects truncated input, overlapping file or
guest sections, sections inside the header and guest-address overflow. It does
not identify the revision; use the disc helper or explicit revision check first.
The layout agrees with [Dolphin DolReader.h](https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/Core/Boot/DolReader.h).

`nflstreet_dol_load_sections` validates all descriptors, source and target bounds
before any writes, zeroes the BSS envelope then overlays initialized sections.
The pinned DOL has an unaligned BSS byte count and initialized data inside that
envelope. Rejecting BSS/data overlap or requiring aligned sizes would reject the
real image. This zero-then-overlay order is an explicit host convention, with
initialized data taking precedence; it makes no claim about original boot order.
Source/header/target must not alias. Callers may use structural loading on other
images, but the disc helper accepts only the pinned revision.

## Gate

From repository root:

```sh
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_disc_constants.py
```

`NFLSTREET_GATE_FAST=1` selects context 15, one Windows process, 11 cases and no
mutants. Full mode selects contexts 14/15/16, two fresh processes each, 201 cases
per process and eight parallel production mutations. Every case compares all
24 MiB, including untouched dirty bytes, with an independent reference formed
from raw fields of the pinned exported DOL; no stack range is excluded.
Seventeen invalid structural/revision/forged-header inputs are also checked.
Build failures/crashes never count as caught mutants; comparison exit 4 is
required. Stable results, private/provider hashes and source hashes are recorded.

Private evidence directory:
`/mnt/w/Daywater/research-temp/nflstreet-disc-constants-20261017/`.
Inputs are the pinned ISO, independent `nflstreet-native-windows-local/game/sys/main.dol`,
contexts `position-neutral-{14,15,16}.mem1`, and the existing nod provider.
Snapshots seed tests only; runtime DOL preparation reads no snapshot.
No original routines, Dolphin integration, render, or live game execution is tested.

## Integration and remaining row 03 work

Before consumer initialization, read the DOL, parse it, then load into a caller-owned
24 MiB buffer with guest base `0x80000000`; free the DOL after loading. The loader
is a standalone input owner and has not been joined into the live app.
Replacing each required private constants catalog with a disc-derived consumer
view remains a separate slice. This does not construct mutable match state,
handle relocation or emulate DVD timing. No shared files need changes here.
