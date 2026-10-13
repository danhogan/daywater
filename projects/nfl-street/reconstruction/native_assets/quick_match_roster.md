# Quick Game selections to native GAME roster

`quick_match_roster.c/h` consumes the existing `nflstreet_quick_match_selection`
output. It validates seven distinct STAT players for each selected team and
both formation halves as permutations of those seven. It builds GAME rows and
preserves selection/formation order as GAME row indexes and newly assigned ids.
The retained field slot/key remain menu inputs; this does not load a field.

The result owns its GAME roster and borrows the unchanged source STAT database.
It can supply `selected_rows` to `selected_player_visual_from_match`. It does
not allocate actors or start a match. Open requires an empty/zero-initialized
output; failure releases staged allocations and preserves output. Close clears
ownership and can repeat. Source player ids must be unambiguous across copied
teams; ambiguous overrides reject. Mirrored teams are supported: both copies share source rows but receive distinct GAME ids.

## Original lineup bit operation

**Verified controlled scalar behavior, GNNE69 revision 0.** `8014FD50` reads
GILP through `80149614`, modifies bit 0 when its second argument is zero and
bit 1 otherwise, and passes the result to `8014982C`. Selection sets that bit;
deselection clears it, preserving every other bit. The native helper exposes
this literal operation on uint32 values.

The original menu source corroborates the side convention: `8011C950` calls
the setter with side 0/1 for each of seven selected ids; `8011D504` clears that
side's bit while iterating its table cursor. These source observations do not
constitute a complete menu-caller replay. The native bridge updates the selected
team's rows with these bit decisions through GAME-copy overrides; it does not
write STAT or emulate database cursor state.

## Verification

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus \
  python3 projects/nfl-street/reconstruction/native_assets/verify_quick_match_roster.py
```

Every process checks:

- 387 input selections constructed through the existing menu selection builder:
  43 teams × three partners (including themselves) × three seven-player/order variants.
- 5,418 selected GAME-row mappings and 10,836 formation mappings.
- GAME GILP values for all copied rows and unchanged source STAT bytes.
- 392 rejection checks for the corrected capture (393 for the legacy capture), including occupied output, duplicate/wrong-team players,
  duplicate formation and invalid field key. The corrected retained snapshot builds both lineups; legacy wrong-team membership rejects.
- 8,192 lineup-bit cases: 1,024 prior words, sides 0..3 and selected false/true.

The original checker dispatches actual `8014FD50` with explicit getter/writer
services, requiring exactly one GILP read and one correctly valued write. It
repeats across three Dolphin 2603a saved contexts twice; the plain native checker
also repeats. The getter/writer are supplied: no original database mutation or
live emulator integration is claimed. Current GAME-roster stage evidence is
required for the native table-copy implementation. Baseline DOL/DLL/runner,
private inputs and source fingerprints remain unchanged.

Private report:
`W:\Daywater\research-temp\nflstreet-quick-match-roster-20261013\verification.json`.
The private QMS1 file in that directory is generated from a retained native menu
selection JSON, not a new public asset or game save.

## Historical native picker gap

The retained `field-picker-window-selection.json` selects teams 1/1. Only six
of each side's seven selected ids belong to STAT team 1 (12/14 total). That historical bridge rejected the unsupported mirror route; the current
bridge rejects this old snapshot for wrong-team membership even with mirrors
supported.

The current preview bootstrap in `menus/team_screen_probe.c` loads every
`supplied_rosters` entry and its `roster_databases` from `db_teams` fallback
blocks even when a loaded STAT roster exists. Those blocks are not identical to
STAT membership (see `selected_player_visual.md`). Fix that owner to use actual
STAT rows for loaded teams and retain fallback handling only for its verified
case. Gate entry point is `menus/verify_team_field_picker.py`; the join source is
`menus/team_field_picker_join_probe.c`. Existing UI behavior was not changed by
this milestone. Preserve old captures before regenerating them; this bridge
report currently fingerprints the retained selection.

Remaining at that earlier checkpoint: mirrored copying and real menu handoff.
The new proof below closes mirrored copying; actor creation/rendering remains.
No playable menu-to-match flow is claimed.


## Picker correction — 2026-10-13

The loaded-STAT roster fix in `../menus/stat_picker_rosters.md` supersedes the
membership mismatch for newly generated menu selections: the Win32 capture
now has 14/14 matching ids. The older 12/14 capture remains a negative regression
and is not overwritten. The subsequent proof below verifies mirrored-team copying and enables it.
To check another private capture, the bridge verifier accepts `--selection`,
`--expected-membership` (12 for legacy, 14 for corrected) and `--output-dir`.

## Mirrored picker route — 2026-10-13

The mirror restriction is removed after original GAME copying/id reassignment
matched all 43 same-team selections. Both GILP bits are computed on each source
row before its two copies are made; the sides can select different seven-player
sets. The bridge corpus now covers identical and different mirrored lineups,
checks global uniqueness of GAME ids and preserves both formation orders.
The corrected Win32 teams 1/1 snapshot now creates 14 selected GAME-row mappings.
The earlier 12/14 snapshot remains a rejected membership regression.

Current private proof:
`W:\Daywater\research-temp\nflstreet-mirror-quick-match-20261013\verification.json`.
It fingerprints the corrected capture and current original GAME proof. This is
a bridge consuming the emitted selection, not yet a menu-controlled actor factory.
Next: pass this owned roster through the running match owner, attach fresh
selected resources/skeletons and draw those actors. No playable match claimed.
