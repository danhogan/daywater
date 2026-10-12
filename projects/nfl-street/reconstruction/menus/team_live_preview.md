# Native team window and selected athlete inspection

The Windows team screen now paints original source-selected artwork, labels and
fonts while advancing the reconstructed source scripts. An optional separate
pane draws the head and body identified by a source actor's selected PLAY row.
It consumes the checked factory-description queue into persistent slot owners;
passive updates no longer replay those descriptions.

A separate `team-field-handoff-probe.exe` now executes actual source completion
after both lineups are ready and forwards its notification to coordinator5.
Six repeatable Windows runs request field8 load/activation. This harness supplies
the logical device and transition clock/audio context; it does not add a field
screen to this preview. See [handoff evidence](quick_game_flow.md).

Use `--live` with `team-screen-preview.exe` for the native window, or
`--live-check` for a bounded 60-update window check. `--player-inspection` adds
the combined head/body pane. `--head-inspection` adds only the head. Existing
`--inspect-roster`, `--inspect-confirmation` and `--capture` controls remain.
These are inspection controls. `--source-controls` enables F2 through F7,
which send translated source events 2 through 7 on a supplied logical port 0.
This is an explicit host testing convention, not an original controller mapping
or a claim that full team navigation is complete. Repeated host keydown events
are ignored; one press sends one event. A rejected source event is logged.
`--source-events '2,3,4,5'` provides deterministic sequential replay before
capture. Scripts accept at most 32 comma-separated events in the range 2–7.

Runtime UI image acquisitions now load into a deduplicated lifetime cache
before painting. The host acknowledges the captured resource prefix only after
all acquisitions succeed. Release requests retire from the queue while decoded
images stay resident until shutdown. Other effect lanes retain their separate
owners; this does not reproduce original archive reference counts or audio.

From WSL, `bash projects/nfl-street/reconstruction/menus/run_windows_team_preview.sh`
builds and opens the roster inspection window with the selected athlete pane.
Escape closes it. Pass `--inspect-roster --player-inspection --live-check` to
that script for the automatic 60-update check instead.

## Evidence and confidence

**Verified, native host boundary:** `verify_team_live_preview.py` passes ten
fresh-process runs: startup, roster and confirmation with head inspections,
plus roster and confirmation with combined player inspections, each repeated.
Every window advances 60 source updates and paints successfully. Repeated menu
and inspection captures match. The passive menu BMP is unchanged. The owned
effect-queue acknowledgement checks also pass. Protected DOL, DLL and runner
source hashes remain unchanged. Outputs are private on
`W:\Daywater\research-temp\nflstreet-team-live-20261011`.

**Verified, controlled original resource boundary:** the actual PLAY identity
selects its face archive through the checked HSH lookup. The original bounded
body descriptor/attachment checks establish the TBYP/body-key/variant file join.
See [resource resolver](team_player_resolve.md) and
[body/head comparison](team_player_selected_pair_preview.md).

The [all-row pair check](team_player_all_rows_pair_check.md) repeats loading and
shared-skeleton validation over all 402 supported PLAY identities: 65 body-key0,
175 body-key1 and 162 body-key2 rows. All pass with no unresolved heads or
geometry failures. It renders one representative per body key, rather than
claiming an original pixel comparison for every player.

**Candidate appearance:** the combined pane uses body variant 0 as an explicit
inspection choice and the files' common stored skeleton coordinates. It rejects
models whose face bone transforms/parent chain do not match the body. The
opt-in Y-up camera is a host inspection camera; it adds no relative head/body
attachment transform and leaves existing static renderer defaults unchanged.
Original materials, skin palettes, hair, equipment, animation, camera placement
and runtime actor matrices are not reproduced by this flat pane.

Game: NFL Street, GameCube GNNE69-r0. DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`.
No live Dolphin menu comparison or complete interactive team navigation is
claimed. The original registered manager and focus path remain under research.

## Focus integration and sequential input

The screen now executes newly queued source focus requests after translated
input, using the existing source-event context (7/5). The input and focus drain
commit together; a late service failure leaves the source and screen state
unchanged. Older queued prefixes are retained for their host owner.

A separate manager-focus API accepts explicit route IDs and stack ownership.
Native focus for nodes 21 → 22 → 28, using supplied VM values 0/7 and stack
0x817f2380, matches all 253,987 source bytes from the untouched original at
activation and after every selection in contexts 14, 15 and 16: 12 exact
comparisons. These are controlled prepared-source checks, not ordinary archive
startup or a complete sequential gameplay menu session.

The original manager lookup tuple `(group 0, screen 7)` is separate from the
VM fields. A fresh original dispatcher trace reads packed current screen `7`
and linked value `0xffffffff` from the callback row at `0x817f00c0`, with
stack `0x817f2380`. The loaded source record's linked value `5` is separate
again. The earlier 12 comparisons establish those focus source mutations;
they do not establish equivalence of the previously supplied VM identities.

The opt-in `--dispatcher-context` uses the observed current/link/stack values
for translated input and supplied focus. The existing input defaults remain
`7/5/0x817c0000`. `--source-audit`, together with a private W: capture prefix,
writes source bytes immediately before and after successful input, before
any subsequent update. These diagnostics expose unsupported service boundaries
by rejecting input and retaining state; they do not supply missing startup,
transition audio, or field-loading services.

## Direct confirmation comparison with corrected context

**Verified for a controlled prepared-source stage:**
`verify_team_dispatcher_event7.py` compares a fresh native focus21 → focus22 →
event7 run with the untouched original manager, using current7/linkFFFFFFFF,
stack0x817f2380, logical port0 and float input1.0. Mode0 and mode6 each match
all 253,987 source bytes before and after input, without source normalization.
Both change the same six bytes. Mode0 has no transition request; mode6 creates
one 500-tick transition with supplied clock6669/audio-enabled1/context0,
matching the original traced inputs. Audio is retained as an effect, not played.

The checker materializes the exact native-prepared source, fonts and line
plans into a private copy of saved context14. Original manager registration
and focus/event execution run; original relocation/construction are skipped.
This does not verify ordinary archive startup or full interactive navigation.
Earlier unmatched starting states differed in animated text-layout values;
regenerating the original fixture established identical starting bytes.

Report and captures:
`W:\Daywater\research-temp\nflstreet-team-dispatcher-probe-20261011\dispatcher-comparison-verification.json`.
The protected original DOL, DLL, snapshots and fixture inputs remain unchanged.
See [browsing entry audit](team_navigation_entry_audit.md) for the separate
team-carousel entry still requiring a verified lifecycle trace.

`--focus-node 22` or `--focus-nodes '21,22'` exposes supplied focus for diagnostics.
These opt-in overrides do not identify the original startup policy.
`verify_team_source_replay.py` checks two fresh roster windows with events
2 → 3 → 4 → 5, successful image-queue consumption, matching repeated menu/player
captures, and unchanged protected original hashes. Outputs stay on W:.

Those earlier navigation trials used the legacy startup without load arguments.
The original forward Quick Game startup below enables a separate browsing branch.

## Forward Quick Game load arguments

The original coordinator requests screen7 with two arguments `{1, 0x69}`.
Original `801CFD90` forwards them to the FFFE load event with a separate argument
`FFFFFFFF`; the later FFFC activation has argument0 and no arguments. Original
`801D399C` frames the load stack as result, separate argument, loaded arguments,
root and terminal0. The loaded arguments are not repeated on ordinary input.
These call edges are documented original code evidence; the whole loaded startup
has not yet passed an original-module differential comparison.

`nflstreet_team_screen_start_context` now accepts that explicit load context.
The legacy startup wrapper retains its zero-argument behavior. In the standalone
Windows preview, `--quick-game-start` supplies the forward load frame and an
explicit logical device map. After1000 native updates, source sequence `7,7,7`
selects teams1/1 and creates both seven-player lineups. Sequence
`4,4,7,4,4,7,7` selects teams31/31 and creates both seven-player lineups. Each
requests14 athletes. The separate selected-player pane resolves PLAY15 and
PLAY318 respectively. Numeric source events have no claimed physical-button
mapping; event6 leaves this source branch rather than committing these teams.

The third event7 required the missing high callback `80000007`. Its native
service requires one argument and sets that side's dirty/rebuild flags; any
nonzero argument selects side1. The untouched original `8011E9AC` comparison
passes raw arguments0,1,2,FFFFFFFF and checks that only those two RAM bytes
change. Native malformed argument counts reject atomically. This verifies the
callback boundary, not the entire startup or lineup lifecycle.

Oracle provenance: NFL Street GameCube GNNE69-r0, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`, original DLL SHA1
`dd521cc38b0db6bca98a0bb261570a2be5e6e0ed`. The controlled comparison seeds a
private copy of context14 (Dolphin2603a reference) and executes original
`8011E9AC` with r13=`803A9A40`. Its two byte arrays are dirty at
`r13-6E34+side` and rebuild at `r13-6E30+side`; each element is one uint8,
so byte endianness is irrelevant. Seed values50/51 and60/61 become1 only
on the selected side. Every other byte of24 MiB RAM stays unchanged.
Original `8011E460` reads these gates for roster rebuilding. Confidence is
**verified controlled callback behavior**; these deliberately seeded values
are not observations of ordinary live gameplay or complete startup.

Run the loaded source window with:

```bash
bash projects/nfl-street/reconstruction/menus/run_windows_team_preview.sh \
  --quick-game-start --source-controls --player-inspection --live
```

The pane remains an untextured candidate stored pose. Original materials,
animation, complete physical navigation, field handoff and fresh match startup
remain unfinished. Supplied profile/device services and retained transition
requests are explicit host boundaries.

`verify_team_loaded_flow.py` passes six fresh Windows processes: two default
flows and two flows for each browsing direction, each followed by60 source updates and60
successful paints. Menu/player captures match within each pair; PLAY15 and
PLAY318 player captures differ. Protected DOL, original DLL and runner-source
hashes remain unchanged. This is **verified native integration** under the
supplied services, not original pixel or complete-startup equivalence.
Private report:
`W:\Daywater\research-temp\nflstreet-team-loaded-flow-20261011\verification.json`.

Opposite source sequence5,5,7,5,5,7,7 selects teams3/3 with seven players on
each side,14 athlete requests and PLAY39. PLAY39 and PLAY318 share body key0,
entry1 and15,823 triangles in this flat inspection, so their matching pixels
do not establish identical original appearance. PLAY15 uses key2/entry7 with
15,524 triangles. The verifier checks those resource bindings as well as
selected IDs and repeated pixels.

The legacy live-preview gate still passes all ten fresh-process captures,
including the unchanged passive menu hash; the existing two-process sequential
source-replay gate also passes. The opt-in loaded frame preserves the prior
inspection paths.

## Original loaded-startup checkpoint

The isolated `team_start_args_original_check.c` builds with strict warnings.
Its final Windows run exits6 during FFFE, before FFFC or any successful
whole-source comparison: original `801495BC`, called from `8011D060`, requests
SIVT from context `802F507C` with MAET/DIGT headers and32 parsed rows, but
the preceding cursor selection leaves `FFFFFFFF`. The next dependency is
the `801509C8` → `8014947C` tag-filtered lookup. No row is substituted to
make this stage pass.

The controlled fixture supplies no-profile bytes/prior IDs, query mode1,
common mode6, logical port0, loaded row7/link5, constructed source/fonts and
actual MAET numeric fields. Original relocation/construction and lower database
ownership are outside this comparison. The one-record `80149AF0` response is
source-backed and supplied; it has no separate differential proof here.
The original manager's external-step hook defaults unset, and the earlier
prepared-source event7 comparison still passes both modes after isolation.

Private checkpoint log:
`W:\Daywater\research-temp\nflstreet-team-start-args-20261011\catalog-windows-root-final.log`.
This is an **incomplete original integration check**, distinct from the passing
native Windows flows above.
