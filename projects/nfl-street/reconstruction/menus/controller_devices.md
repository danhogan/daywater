# Controller initialization and device refresh — 2026-10-10

## Result and boundary

The native controller callback now matches the local source initialization
branch and device refresh. It owns the original eight device-status flags and
ten logical binding entries, clears disconnected selections, fills available
profile slots, resets side assignments after a change and retains the source
forced-selection rule. The initialization branch remaps existing profiles,
optionally forces a refresh and returns the original raw initial-choice word.

`controller_devices.c` reconstructs original `8011B430` and the local
`8011B698` branch. `controller_callback.c` connects source properties
`80000001`, `80000006`, `8000000B` and numeric `8` to owned screen/VM memory. Property
`80000006` writes the change result before its four presence words, preserving
the original behavior when the destinations overlap. Property `8000000B`
retains `801278B4(FFFFFFFF, 0/1)` requests; other values do nothing.

**Device responses and profile services are supplied.** These are logical
bindings, not verified Windows USB/gamepad mappings. Profile-mode and activation
requests are captured rather than applied to a profile database. The local
Queries `80009E5C`/`80009E6C` receive 0; the other original initialization
branch is still unsupported. Original status2 is the tested equality operand;
no complete hardware-status enum is inferred.

This extends the [local controller selection](controller_selection.md).
Across three reference contexts, 21,504 refreshes, 10,752 initializations and
43,008 actual screen callbacks match the original. The visible controller
screen still needs class construction, source creation and activation, input
routing and its timed completion. Quick Game routing continues to supply
selector completions. Team/field selection and native creation of the selected
match remain unfinished. Existing launchers are intact.

## Evidence

Confidence: **verified controlled local initialization/device refresh and callback
behavior**. Game: NFL Street, GameCube GNNE69 revision0. DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Reference RAM: Dolphin2603a saved
contexts14/15/16. Each comparison writes explicit test inputs and invokes the
untouched original module; this is not fresh live menu navigation.

Across the three contexts:

- 21,504 refreshes compare every presence byte, four slots, four sides, four
  signed profile mappings, ten logical bindings and the full ordered effect log.
- 10,752 local initializations additionally compare the raw choice store and
  pointed output, remapping and profile-mode requests.
- 43,008 actual `8011BC68` callbacks compare complete 1,024-byte owned buffers
  and all those states, and check that both implementations return success.
  Properties `80000001`/`80000006`/`8000000B` and numeric `8` are covered.
  Numeric `8` writes FFFFFFFF through its third argument, then writes raw99/100
  through its second argument for those choices; both alias orders are checked. Refresh
  output-pointer overlap and property `8000000B` values 0/1/2 are included.

Inputs include all 256 eight-bit presence patterns, unchanged/inverted previous
flags, forced/ordinary refresh, slots 0–6, raw sides 0/1/FFFFFFFF, rotated or
partly unavailable profile mappings, active ports 0–3/FFFFFFFF, opaque query
values 0/1/2 and profile-mode query 0/6. The original status query receives
raw 2 when present and supplied 1/FFFFFFFF otherwise.

`verify_controller_checks.py` automates the strict Windows build, execution
against all three private contexts, selection/routing regressions, pre/post
baseline hashes and private verification metadata. `--no-build` reuses the
existing check executables. The original executes its own controller routines.
Observation boundaries:
`80199E58` polling, `8018013C` slot requests, `80199BCC` supplied status,
`80129AA4` binding requests, `80129ACC` owned logical binding queries,
`80129AB8` supplied active port, `80127798` supplied profile mapping,
`8010C390`/`8010B60C` supplied opaque modes, `80127910`/`801278B4` retained
profile requests and local `80009E5C`/`80009E6C`=false. The original `8011B0D8`
mode read executes unchanged. No original DLL/source is patched.

The automated verification passes the strict Windows build and all three
contexts, including the local-selection and Quick Game routing regressions.
Capacity exhaustion after a partial internal request capture and malformed
previous flags reject atomically. Effect logs are bounded; the caller must drain
and clear them between updates.

## Controlled memory evidence

| Required field | Evidence |
| --- | --- |
|Game/revision/hash|NFL Street GameCube GNNE69 revision0; DOL SHA1 above|
|Reference build|Dolphin2603a contexts14/15/16; controlled original-module runs|
|Address|Supplied ABI r13=803A9A40; r13-5A40=803A4000, eight consecutive bytes|
|Primitive/size/endian|Eight boolean bytes,1byte each; byte order is irrelevant for individual bytes|
|Procedure|Seed each presence pattern and unchanged/inverted previous flags; call original8011B430 with force0/1, then local8011B698 and actual callback8011BC68; compare complete state and ordered requests|
|Observed|Each byte becomes1 exactly when its supplied status query returns2, otherwise0. A changed byte or forced refresh returns raw1 and updates the checked selection/side state; unchanged ordinary refresh returns0|
|Readers/writers|8011B430 writes/tests the eight-byte array;8011B7CC calls refresh and returns four logical-binding presence words|
|Confidence|verified controlled source status flags and change detection|
|Limit|Supplied responses; no live hardware observation, universal controller mapping or full status enum. Ten bindings are owned service records, not a newly discovered live pointer chain|

Other slots, sides, mapping and choice fields retain the evidence records in
[controller selection](controller_selection.md); the choice initializer now
matches local8011B698 too. Query mode1 stores raw choice1; other tested local
modes retain the supplied prior word. These modes have no inferred enum labels.

Private executable, logs, hashes and verification metadata:
`W:\Daywater\research-temp\nflstreet-controller-devices-20261010`.
Build with `build_windows_quick_game_checks.sh`. Run
`controller-devices-original-check.exe` with the private original DLL and one
private 24 MiB context. Native selection/device code has no original-module,
ModernGekko or Dolphin runtime dependency.

The source screen creation invokes initialization and refresh in one transaction.
The bounded effect capacity is128 so both ordered request sequences fit; callers
consume the log between updates. Saturated logs reject work instead of dropping
requests. This change and numeric callback8 pass the same three-context pipeline.
