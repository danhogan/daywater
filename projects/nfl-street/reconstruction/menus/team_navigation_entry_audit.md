# Team browsing entry audit — 2026-10-11

Game: NFL Street, GameCube GNNE69-r0; DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`.

## Established boundary

The prepared focus sequence 21 → 22 → 28 belongs to the roster/player subtree.
The tested event2/3 source list has count zero there. This does not establish
that the separate three-team carousel is empty or that this is the team
browsing entry point. The observed manager VM current/link values are
`7/0xffffffff`; its lookup tuple is group0/screen7. See
[native preview evidence](team_live_preview.md).

## Original call edges to pursue

These edges come from cached original decompilation, rather than an exercised
interactive browsing session. Their use as the missing UI lifecycle is
**candidate**.

| Original entry | Bounded role / next dependency |
| --- | --- |
| `8011E9D4`, property `80000004` → `8011CE64` | Updates a three-item team window; `8011D060` consults profile lookup `80127900` and SIVT lookup `801495BC`. The checked no-profile branch does not cover profile-backed eligibility. |
| `8011E9D4`, property `80000006` → `8011D19C` | Releases old STAT rows through `8011D280`, stores the team ID, acquires new rows through `8011D348`, and opens the STAT/GAME table through `8011D458`. |
| Changed-team branch in `8011D19C` | Under its mode and side gates, calls `8011D504`, `8011D628`, `8011D5F8` and `801EE24C(0,5)` to rebuild roster rows/cursor/formation. This produces the player list, rather than the three-team window. |
| Common callback `8010C3BC`, property `F5` | Calls `80005840` and `80004140`; the latter calls `8011B2B8`, `8011BEB0`, `8011EE64(-2,-2)` and `8011F5D8`. `8011EE64` refreshes selected teams through `8011D19C` and other side helpers. |
| Registered game callback property1 → `8011C414` | Initializes the two carousel cursors and three-item windows in the checked default/no-profile branch. |

`80137660` also dispatches `80000004` with operation3, but its caller
`80137854` is gated by transition state and other predicates. That call alone
does not identify the player-facing browse event.

## Required verification

Trace the actual entry7 event scripts from the original activation state to
their high callbacks. Establish which source selection reaches `80000004`
and `80000006`, then compare the complete source and retained effects from
fresh matching inputs. Do not synthesize a list count or assign controller
meanings to events2/3 from the roster subtree alone. The ordinary archive
startup, profile-backed eligibility and complete menu-to-match flow remain
unfinished.

## Deterministic native candidate scan

`team_source_navigation_scan.c` and its Windows build script discover source
nodes with self events2–7 and derive each unique acyclic ancestor path to root0.
Every case starts from copied startup source/screen state, with mutable
bootstrap and mapped DOL state cloned. Database views remain read-only.
The harness rejects ambiguous paths and keeps raw snapshots private on W:.

The first completed scan found 76 node/event candidates in the 317-node source
graph. All immediate focus/input calls succeeded using the supplied dispatcher
context. None changed the committed team IDs or three-team carousel windows.
Node0/event6 produced resource requests, while event7 on nodes0/21/77 produced
transition requests. Fresh repetitions of those four event snapshots matched.
This is **verified native harness behavior**, not original-engine equivalence
or proof that other startup modes and input sequences cannot browse teams.

The separate update checkpoint uses the legacy update VM context
`7/5/0x817c0000`; its service owner must use linked5 as well. It is distinct
from the immediate input context `7/0xffffffff/0x817f2380`. Link requests remain
recorded for a future screen owner rather than claiming field loading works.
With that explicit update-only owner override, all 76 cases complete 60 updates;
none changes the team IDs or carousel windows. The first run's three delayed
rejections disappeared after aligning the harness owner with the legacy VM,
and the immediate event snapshots remained unchanged.
The scan eliminates repetitive manual retries of these same isolated cases;
the missing original entry lifecycle still requires tracing.

Private output:
`W:\Daywater\research-temp\nflstreet-team-source-navigation-scan-20261011\navigation-scan.json`.

## Loaded entry enables browsing

The earlier76-case scan used startup without loaded arguments. That is a
specific prepared state, not the original forward Quick Game entry. Cached
original coordinator code supplies `{1,0x69}` when loading screen7. `801CFD90`
passes them to FFFE with separate argumentFFFFFFFF; FFFC receives0/no arguments.
The native explicit-start API and preview's `--quick-game-start` now reproduce
that frame without changing legacy defaults.

**Verified native execution:** after1000 updates, source events4,4,7 commit
team31 on side0. Repeating4,4,7 for side1 and a further7 gives teams31/31,
lineups7/7 and14 athlete requests, with PLAY318 in the selected-player pane.
Default7,7,7 gives teams1/1, lineups7/7,14 requests and PLAY15. The missing
`80000007` dirty/rebuild service is now connected and independently compared
with original `8011E9AC`. These results establish connected native browsing
and roster construction, not original full-startup/source-pixel equivalence.

The full original load-stage comparison remains separate work. Logical
device/profile inputs, original athlete materials/animation, field selection
and menu-to-match ownership are still incomplete. No physical-button meaning
is assigned to these source event numbers.
