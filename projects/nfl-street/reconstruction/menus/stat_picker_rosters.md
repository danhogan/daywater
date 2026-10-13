# Native picker uses loaded STAT rosters

## Fix

The native preview bootstrap previously used each `db_teams` fallback block for
its player list even when that team already existed in STAT. The saved picker
selection consequently had only 12/14 ids belonging to its selected STAT teams.

`team_screen_probe.c` now retains fallback blocks for catalog metadata but uses
an owned copy of the loaded STAT database for roster ids, preview asset requests,
labels and weighting when PLAY contains that team. Absent teams retain the
existing fallback path. Per-roster ownership and teardown stay unchanged.

`team_preview_roster_from_db` now searches for exactly one matching TEAM row in
a multi-team database instead of requiring the entire TEAM table to have one
row. Missing or ambiguous team identity rejects. Single-team fallback parsing
continues to use the same validation and projection.

This follows the original loader distinction already documented in
`../native_assets/selected_player_visual.md`: existing STAT membership is
copied to GAME; absent teams use a separate fallback loader. It does not
reconstruct or verify that absent-team loader's full lifecycle.

## Evidence

**Verified native picker/membership and controlled original fallback regression,
GNNE69 revision 0.** Run:

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus \
  python3 projects/nfl-street/reconstruction/menus/verify_team_field_picker.py \
  --output-dir /mnt/w/Daywater/research-temp/nflstreet-stat-picker-20261013 \
  --live-window
```

The gate repeats native default, forward and reverse team/field selection
sequences, retaining seven players and both formations. Existing lineup checks
cover 320 team/side/weighting-mode combinations. Back navigation creates no
selection. The actual Win32 event/timer/paint loop confirms field slot/key 1,
finishes source shutdown and consumes its notification queues. Manual keyboard
input was not tested.

The newly emitted Win32 selection is passed to the roster-bridge verifier:
**14/14 player ids match STAT membership**, replacing the old 12/14 result.
That snapshot selects mirrored teams 1/1. Subsequent original copy/id checks
now permit this route, and the bridge creates both native GAME lineups with
distinct ids. No fresh match starts.

Fallback regression still parses all 32 private blocks / 402 ids. Original
`8011D628` projection matches 64 side/team comparisons per saved context in
three contexts; database iterators are supplied and class3 rendering is not
executed. The bridge also repeats its original `8014FD50` scalar comparison.
No live Dolphin or full original UI startup comparison was performed.

Private reports:

- `W:\Daywater\research-temp\nflstreet-stat-picker-20261013\verification.json`
- Its `roster-bridge\verification.json`, fingerprinted by the picker report.

Reports include transitive source fingerprints (with actual include directories),
private input hashes and protected DOL/DLL/runner hashes. A new output directory
preserves the earlier picker captures. The legacy 12/14 snapshot remains a
separate negative regression; the reusable bridge verifier accepts a selection
path, expected membership and private output directory.

## Remaining

Connect
the validated real menu selection to the match owner, then create/attach/render fresh
actors. The independent scene still uses reference players. Field confirmation
and a coherent selected-player list are not a playable match.
