# Original local controller selection — 2026-10-10

## Result

The native controller callback now owns the checked local selection rules:
cycle profile slots in either direction, resolve duplicate choices, skip
unavailable profiles, retain the original special labels, remap selections when
the profile list changes, read/write side assignments, and confirm them.
Confirmation records the original ordered binding/profile/audio requests.
It does not execute those requests against a match or profile database.

`controller_select.c` reconstructs 8011B368,8011BB74,8011BC10, the **local**
branch of 8011B87C and 8011B728. `controller_callback.c` plugs those services
into the owned VM memory interface for actual group0/entry6 callback 8011BC68:
properties 80000002/03/04/05/07/09/0C/0D. Source property 04 returns raw 99/100;
they remain opaque. Property 0D guards ports beyond 3 instead of reproducing
the original unbounded store. Unsupported callbacks reject explicitly.

**The controller screen is not visible or activated yet.** Its source creation,
device refresh, input dispatch, fade/completion and connection to coordinator 5
remain required. The routing verifier still supplies selector completions.
The subsequent [device initialization milestone](controller_devices.md) adds
local initialization80000001, supplied-response refresh80000006 and retained
profile requests8000000B. The nonlocal8011B87C branch remains unsupported. Profiles, ordering and logical
controller handles are supplied; no fresh profile bootstrap or USB mapping is
claimed. The selected team/field and fresh native match remain unfinished.

## Original comparisons

Confidence: **verified controlled local selection and callback behavior**.
NFL Street GameCube GNNE69 revision0, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`; Dolphin2603a saved contexts 14/15/16.
These are original-module calls with explicit service inputs, not fresh live
menu navigation or full original gameplay.

Across those three contexts:

- 14,403 terminating profile cycles match all four slots, side words, mapping
  bytes and the complete 20-byte output buffer (including unchanged tail bytes).
- 15,000 profile remaps,15,000 set operations and15,000 reads match the original.
- 15,000 confirmations match the stored raw choice and every ordered request:
 801E02FC reset, eligible801E02B4 side/handle pairs,8014C20C(2,choice), then
 8013DA04(AB,32,0). Requests are captured at those original service boundaries.
- 11,952 actual 8011BC68 callbacks match complete 1,024-byte owned buffers,
 selection state and commit requests. This includes array/string descriptors and
 the real callback result-pointer ABI; it does not execute source UI scripts.
- 597 cycle cases with supplied inconsistent/unavailable choices do not return
 within the original dispatch budget. Native cycling rejects these atomically
 after 32 attempts. This is a bounded safety difference, not exact execution of
 an infinite original loop; no fresh original startup state is inferred from it.

Inputs cover ports 0–5, slots 0–6, supplied sides 0/1/FFFFFFFF, mappings -1..3,
profile counts 0–4 and directions -1/0/1/INT32_MIN/INT32_MAX. Names and profile-list
updates are supplied at 801273B4/80127798; local queries 80009E5C/80009E6C return 0
and 80127394 receives a supplied count. Special labels come from untouched
reference RAM. Their formatting executes in the original module.

An initial mismatch exposed an assembly detail: resolving a duplicate against
a different side exits the duplicate scan even when it clears that other slot.
The native implementation now follows original 8011BACC -> 8011BAEC. Decompiler
control flow alone was insufficient for that case.

Malformed slots, short text capacity, an invalid second VM output pointer and
unsupported initialization retain owned state/output. Two valid VM pointers
receive the original raw 99/100 results. Strict Windows compilation passes.

## Controlled memory evidence

All records below use the title/revision/DOL hash and reference build above.
ABI r13=803A9A40 is supplied. Tests write controlled state, call the untouched
original routines, and compare all relevant bytes plus captured service effects.
Addresses follow the original instructions; no live UI-manager pointer chain
or fresh-menu default is claimed.

| Address | Primitive / size / endian | Observed test values | Readers / writers | Confidence and limitation |
| --- | --- | --- | --- | --- |
|80282590 + port*4, port0..3|uint32,4bytes each,BE|slot0..6; cleared slot4; wrapped0/6; remapped slot index|8011B368,8011BB74,8011BC10,8011B87C|verified controlled selection slots; profile IDs are mapped through the four signed bytes, not interchangeable with slots|
|802F506C + port*4|uint32,4bytes each,BE|raw0/1/FFFFFFFF and callback 0D writes|8011B87C,8011B728,8011B840,8011B854|verified local conflict/commit operands; no complete side enum or gameplay controller binding claimed|
|r13-6E60=803A2BE0 + slot|int8,1byte each|supplied-1..3; old mappings retained or cleared after supplied list reorder|8011B368 calls80127798;8011BB74/8011BC10/8011B87C read|verified controlled profile-slot mapping; native profile database bootstrap remains open|
|r13-6E5C=803A2BE4|uint32,4bytes,BE|raw commit choices0..4999; retained in profile request|8011B728 stores;8011B698 initialization is not reconstructed here|verified controlled confirmation store; raw words are test inputs; they have no inferred game enum meaning|

## Reproduce

Build `build_windows_quick_game_checks.sh` with a private W: output directory.
Run `controller-select-original-check.exe` with the untouched original DLL and
one private 24MiB reference context. All three runs, executable and verification
metadata are private under:

`W:\Daywater\research-temp\nflstreet-controller-selection-20261010`.

This checker loads original code solely as an oracle. The native selection and
callback implementation contains no Dolphin/ModernGekko dependency. Existing
Windows launchers and original DLL/generated sources are unchanged.
