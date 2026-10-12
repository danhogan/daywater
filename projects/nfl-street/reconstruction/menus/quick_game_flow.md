# Quick Game routing — 2026-10-10

## Result and boundary

The original menu route is now connected in an independent Windows C verification:

`main Play It -> Quick Game -> coordinator0/2 -> Quick Game coordinator0/5 -> controllers0/6 -> teams0/7 -> field0/8 -> coordinator finished`

The original joined route checker uses **source script routing with supplied
selection-screen completions**. A newer `controller-start-probe.exe` join now
executes controller creation, activation, timers and translated source input,
then forwards its actual delayed completion and linked5 request to the source
Quick Game coordinator. It requests team7 load/activation without a supplied
controller completion; team/field recipients and match creation remain open.
It does not display or execute the controller/team/field screens, choose real
rosters, load the chosen field, or create a match. `quick-route-check.exe` loads
three original owned UI files and font3. It does not load the original DLL,
Dolphin, ModernGekko, an ISO or a RAM snapshot. Its menu context is an explicitly
bounded inspection context; this is not a fresh original front-end boot.

The [local controller callback](controller_selection.md) now reconstructs
profile cycling, duplicate resolution, mapping and confirmation requests. Its
local source initialization and device refresh are also checked against the
original across three contexts: 21,504 refreshes, 10,752 initializations and
43,008 actual screen callbacks. Device/profile responses are supplied and profile
requests are captured. Windows device discovery, main-window integration and team/field selection
remain unfinished. The older routing join still supplies selector completions;
the newer controller join uses the executed source completion. See
[controller source screen](controller_screen.md) for the original dispatcher
comparison and independent diagnostic controller rendering. The strict build and three
context/regression run is automated by
[`verify_controller_checks.py`](verify_controller_checks.py); use `--no-build`
to reuse existing check executables. See
[controller initialization/device refresh](controller_devices.md) for evidence.

Existing original menu and native scene launchers are unchanged. No arbitrary
team/field picker or reference-player scene is presented as original Quick Game.

## Reconstructed source route

- Main mode0 owner confirm17 enters Play It (mode1).
- Mode1 owner confirm13 is the source Quick Game row. The same node is Features
  in mode0; row identity must include the current mode.
- Quick Game executes property80000003, storing raw word7 through original
  `8011B0E0` at `r13-6E74`, then its timed exit broadcasts property1 with raw
  arguments `6C,FFFFFFFF` and requests activation of the supplied linked
  coordinator. Its source file is group0/entry1.
- Group0/entry2 consumes the notification and reactivation, requesting
  group0/entry5. Packed identity `5` means group0/entry5. Packed identity
  `50000` means group5/entry0 (Options); these are different destinations.
- Group0/entry5 activationFFFE sets owned source root+70 to68, loads controller
  screen6 with argument0, requests ordering displacement-2 and activation.
- Forward notification1 from source state68 changes it to69: release6,
  load7 with arguments `1,69`, ordering-2; reactivationFFFC activates7.
- Forward from69 changes it to6A: release7, load8, ordering-2;
  reactivationFFFC activates8.
- Back notification2 from6A returns69, releases8 and loads7 with `0,69`.
  Back from69 returns68, releases7 and loads6 with argument0.
- Forward from6A releases8 and sets source state1. This is a coordinator
  completion state, **not native match creation**.
- Back from68 sets64; reactivation releases the current screen and requests
  outer coordinator2 with arguments `FFFFFFFF,6C`. This extra wrapper check is
  native source execution; it was not added to the differential sequence.

Source screen names come from their original text. Source numeric states remain
opaque outside these tested branches. `dispatcher_notify` and `reactivate`
retain Options wrappers. `quick_setup_event` uses the actual event table in
entry5. Unsupported events and late request-capacity failures retain file,
service context and flags together.

VM opcode72 queues original kind8, a signed **loaded-record ordering move**.
It does not assign a link. Native request property8 retains destination, source
identity and raw displacement. The subsequent [loaded-screen ordering join](screen_order.md) now applies these
moves to supplied owned records and preserves rate/continuation owner references.
Recipient lifecycle, loading and visible selectors remain required. The initial
source comparisons below captured these effects rather than applying them. Original reader: `801D4CDC -> 801D463C`.

## Evidence

Confidence: **verified controlled original-module routing**, NFL Street
GameCube GNNE69 revision0; DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`.
Dolphin2603a contexts `position-neutral-14/15/16` supply reference RAM.

`quick_main_original_check.c` executes untouched original VM/scheduler/class/common
callbacks for Play It and Quick Game: six controls and960 complete scheduler
updates across three contexts. Complete50,434-byte screens, rates, entry flags,
mode word, ordered focus/resource/help/audio and linked activation requests match.
Font/static archive lookup is supplied; resource/audio/help I/O is captured.
The linked coordinator identity2 is explicitly supplied in the comparison.

`quick_outer_original_check.c` also checks the previously bounded outer
coordinator's actual Quick Game notification and reactivation:18 complete
source-file/VM/context comparisons and42 ordered requests across the same
three contexts and two supplied query134 values. Group0/entry5 is requested.

`quick_setup_original_check.c` executes original801D16A0 for16 sequential events
at each of two supplied opaque query134 values, in all three contexts:
**96 complete1,257-byte file,256-word VM stack,PC,SP,flags and service-context
comparisons;174 ordered load/readiness/ordering/activation requests**. This covers
forward through field, Back to teams/controllers, forward again and completion.
Every native event wrapper also matches its directly checked VM result. A late
ordering-request capacity failure rolls back atomically. Actual queue handling,
file loading, readiness, gamepad input and selection-screen callbacks are not
part of these comparisons. Original queue801D4B3C is an observation boundary.

`quick_route_check.c` independently joins actual main broadcast/link outputs to
outer notification/reactivation and entry5 activation. Only controller/team/field
completion notifications are supplied; their choices are not synthesized.
The Windows joined route passes, now with ordering applied to supplied records.
No manual keyboard or visible setup capture is
claimed. Existing source Options dispatcher and Play It/Features/Back comparisons
pass again in context14; the complete Windows menu build passes strict warnings.

## Controlled mode-word evidence record

|Field|required evidence|
|---|---|
|Game|NFL Street, GameCube GNNE69 revision0; DOL hash above|
|Reference build|Dolphin2603a saved contexts14/15/16; original module comparisons, not a fresh live menu capture|
|Address|Supplied r13=803A9A40, field r13-6E74=803A2BCC; this test address follows the source store and supplied ABI context|
|Type / size / endian|uint32 /4bytes /big endian|
|Procedure|Load each supplied context, execute source mode0 Play It confirm17, advance160 ticks; execute mode1 Quick Game confirm13, advance160 ticks; compare after every step|
|Observed|All three initial saved-context values are0. Raw7 is written during Quick Game confirmation and retained through the linked handoff; initial values are from saved match contexts, not fresh menu defaults|
|Writer / reader|8011B0E0 property80000003 raw store /8011B0D8 direct read|
|Confidence|verified controlled write of7 during this exact source menu branch|
|Limit|Do not infer a comprehensive game-mode enum or a match-start event from this word. Native field/roster creation remains separate.|

## Match startup work still required

The original field callback `8011F4AC` property80000002 calls `8011F170`.
When `8010C390` (r13-6F88) returns1, it calls `801211D4`, resolves the field byte
from `r13-6E10 + selected index(r13-6E04)`, calls80027270, obtains both side records
through8003F7F0, then801EA480 /8014C148 /800040F0, and frees the field cache.
Confidence: **probable static call chain**, not a native startup implementation
or verified full original match transition. These services must become native
before coordinator completion can start a selected match.

The original field UI cache has eight205-byte records allocated by8011F0C8;
8011F1F0 resolves names/descriptions/unlock state and updates the selected index.
Team UI callbacks dispatch through8011E9D4. Their database, selected player
records, team appearance, controller assignment and actual field loading remain
required. The existing native two-reference-player scene is not that roster.

The forward team load frame is now connected independently in the native
team preview: FFFE argumentFFFFFFFF/argc2 `{1,69}`, followed by FFFC
argument0/argc0. Both browse directions and both seven-player lineups execute
through source scripts with selected-athlete resource resolution. Six fresh
Windows runs pass repeated captures. The later team completion join below now
requests field8; the older route checker above still supplies those completions.
See [team preview](team_live_preview.md).

### Actual team completion to field requests — 2026-10-11

**Verified native source execution with supplied host boundaries:**
`team_field_handoff_probe.c` executes the loaded team source, both seven-player
lineups, a fourth confirmation event and subsequent timers. Its actual
property1 completion broadcast is forwarded to coordinator5, which requests
field8 load and activation. No focus, roster count or team completion is forced.

`verify_team_field_handoff.py` passes six fresh Windows processes: default
teams1/1, reverse browsing31/31 and forward browsing3/3, repeated twice each.
Each retains14 athlete requests, one transition, one completion broadcast and
one linked5 request; output hashes repeat within each case. The preceding
controller completion, one logical device, clock12345, audio0 and context7
are supplied. This is separate from the incomplete full original startup
comparison. The field recipient and match construction are absent.
Private report: `W:\Daywater\research-temp\nflstreet-team-field-handoff-20261011\verification.json`.
Protected DOL, original DLL and runner source hashes remain unchanged.

The same six-run gate now retains both native fourteen-word active formation
arrays. Every word belongs to its side's seven selected players, and the
arrays repeat within each team case. These are the native inputs needed by
the later field commit, not a full original startup comparison. Existing
`team_bootstrap_static.c` reads the two seven-byte request orders from the
hash-checked DOL (`803A1E94/803A1E9C`); bounded original formation/activation
checks already cover their consumption.

Static source tracing connects team changes through `8011D19C -> 8011E460 ->
8003F6B8 -> 8004278C`, which ranks roster records into fourteen slots.
`8011F170` later copies those two arrays through `8003F7F0` and passes them
to `801EA480` with selected field/session state. Confidence for this complete
call chain is **probable static evidence**; fresh match creation and full
registered YALP/DIGP query behavior remain unverified.

The native [field-cache reader](field_cache.md) now reproduces the original
cached branch of `8011F1F0` in24 controlled comparisons. Database/profile cache
population, visible field source activation and selected-field ownership remain
separate requirements.

## Private evidence and reproduction

Outputs, logs and build products:
`W:\Daywater\research-temp\nflstreet-quick-game-20261010`.
`verification.json` records hashes, checks and limitations. The original DLL and
runner were never edited; their baseline hashes remain unchanged.

Build with `build_windows_quick_game_checks.sh`; its default output is private W:.
Original comparisons require the private original DLL, a supplied reference RAM
context, the corresponding original screen file and font3 for the main check.
`quick-route-check.exe` accepts main0/1, outer0/2, setup0/5 and font3 paths in that
order. It is a routing verifier, not a user-facing game launcher.

## Actual team completion joined to a native field recipient

`team_field_picker_join_probe.c` now consumes the actual team completion
through coordinator5, constructs source entry8, runs its FFFE/FFFC lifecycle,
settles180 native timer updates, and executes source events5/4/5 with60 updates
apiece. Its owned root+90 BE32 cursor and cache-read index agree at1/0/1.
Final source event7 produces exactly one native transition request, one timed
completion and one link effect to5. This is source-driven native integration;
it is not a complete original field lifecycle comparison or physical PAD mapping.

`quick_match_selection.c` preserves both actual seven-player lineups and
fourteen-word formations alongside the selected UI slot and DOL field key.
Unavailable cache slots, incomplete lineups and duplicate formation members
reject atomically. The result is a native match-input record. Original
`8011F170`'s source shutdown request is now captured through actual FFFD
execution: exactly one slot1/key1/owner1 request and one cache-release request.
The join does not execute `801EA480` fresh match construction.

**Reproduced native integration:** `verify_team_field_picker.py` passes six
fresh Windows processes: teams1/1,31/31,3/3, each repeated twice, selecting
UI slot1/raw key1. Per-case output traces agree. Preceding controller completion,
logical port mapping, no-profile IVID context, statistic0, clock and static
archive availability are explicit supplied inputs. Protected originals and
private source/template hashes remain unchanged. Confidence: **verified native
integration**, with the original field cursor interpretation **probable source
semantics** pending a full original VM/lifecycle comparison.

Private report:
`W:\Daywater\research-temp\nflstreet-team-field-picker-join-20261011\verification.json`.

## Original selected-input transfer

The separate [bounded original commit check](field_commit.md) executes
original `8011F170`, `80027270` and `8003F7F0` in three saved contexts.
It matches the native slot1/key1 and both complete fourteen-word formations
at the `801EA480` call boundary. Cleanup/settings/init/free services are
captured, and the native arrays and owner mode1 are supplied. Confidence:
**verified controlled transfer**, not full original match startup.
`verify_field_commit.py` repeats this comparison and checks protected hashes.

The next implementation boundary is documented in
[fresh match startup contract](fresh_match_start_contract.md): original
`801EA480` queries the startup context, resolves both sides, transforms two
ordered fourteen-word arrays and calls the session recipient. Those later
services require their own reconstructed ownership and comparisons.
