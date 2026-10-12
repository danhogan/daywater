# Loaded-screen ordering in Quick Game — 2026-10-10

`order.c` now applies the original `801D463C` loaded-screen move instead of
retaining opcode72 requests as diagnostics only. The independent Quick Game
route verifier applies its requests to supplied owned records.

The move swaps adjacent complete20-byte loaded records until the signed
requested displacement is exhausted or a list edge is reached. Each swap
also moves the active index and updates animation-rate and suspended-callback
owner references. This preserves which screen those references belong to.
There is no file loading, rendering, readiness or activation in this function.

## Evidence

**Verified controlled original-module ordering**: NFL Street GameCube GNNE69
revision0, DOL SHA1 `46e3de854998a0f66c9be8c72a1164d22cd29eb4`.
Dolphin2603a saved contexts14/15/16 supply original reference RAM. The unchanged
original DLL executes801D463C and its original801D3828 identity lookup.

Across three contexts, **4,320 complete comparisons** cover:

- supplied loaded counts0/1/4/16;
- active slots0/1/3/FFFFFFFF;
- enabled/empty rate and continuation lists;
- found and missing screen identities;
- negative, zero and positive moves, clipping at list boundaries;
- INT32_MIN and INT32_MAX displacements;
- external owner references and untouched opaque words.

Each case compares all16 supplied loaded rows, all64 supplied13-word rate
records, all8 supplied10-word continuation records and the active index.
Inactive records are compared too. Logical addresses only identify owned rows;
the native code never dereferences game RAM or host addresses.

**21 additional joined comparisons** apply seven actual Quick Game ordering
requests in each context: initial controllers, forward to teams and field,
Back to teams and controllers, then forward again. These requests are produced
by the actual group0/entry5 source script. Native request application and original
ordering agree. Bounds and wrong-request rejection leave state unchanged.

The independent Windows `quick-route-check.exe` also passes after adding actual
ordering application. Its loaded records are explicitly supplied, as are the
controller/team/field completion notifications. Source load/readiness and
activation requests are retained; this does not execute the visible selectors
or create a selected match. Existing native scene and menu launchers are unchanged.

## Structure evidence and limitations

These are original offsets exercised in supplied scratch managers, **not a
newly discovered live-game pointer chain**:

|Owned field|Original offset / type / size / endian|Evidence|
|---|---|---|
|Loaded row identity|row+4 /uint32 /4 /big endian|801D3828 compares its two16-bit halves;801D463C swaps all five words|
|Active index|manager+30 /uint32 /4 /big endian|801D463C adjusts either participating slot, preserves unrelated/sentinel values|
|Loaded count/list|manager+38,+3C /uint32,pointer /4 each /big endian|Supplied scratch list base817E0000, counts0/1/4/16|
|Rate owner|rate+1C /pointer /4 /big endian|Original loops manager+54 count /+58 list, stride34; reference swaps tested|
|Continuation owner|continuation+18 /pointer /4 /big endian|Original loops manager+60 count /+64 list, stride28; reference swaps tested|

Confidence: verified for controlled ordering with valid supplied owned arrays.
Original heap/list allocation, recipient construction, device input, visible
selectors, fresh front-end boot and selected roster/field startup remain open.
No new live Dolphin session or manual keyboard test is claimed.

Private builds, logs, hashes and `verification.json` are on W: beneath
`Daywater/research-temp/nflstreet-menu-order-20261010`. The original DOL, DLL and
runner source hashes remain unchanged. Build with
`build_windows_quick_game_checks.sh` and run `order-original-check.exe` with
original DLL, reference MEM1 and original group0/entry5 source file paths.
