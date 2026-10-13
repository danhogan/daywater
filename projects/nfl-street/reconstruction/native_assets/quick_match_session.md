# Native Quick Game roster ownership after menu confirmation

`quick_match_session.c/h` is a native host owner. It copies the loaded STAT
DB, builds the verified GAME roster from the selection and empty GAME schema,
and retains both seven-player lineups, formation orders and field slot/key.
All borrowed STAT views in the roster point into that owned copy. The temporary
schema buffer is only used during open; GAME descriptors/rows are owned by the
roster. Close frees the roster before its STAT source and can repeat.

This is host architecture, not a claim about original session allocation or
801EA480 scheduling. The roster copy/id/lineup semantics retain the separately
verified original evidence in `game_roster.md` and `quick_match_roster.md`.

## Native menu connection

`menus/team_field_picker_join_probe.c` now creates this owner after field FFFD
confirmation, before writing the retained selection snapshot. The owner lives
through `team_probe_main` teardown: it no longer borrows the menu's freed
bootstrap database or roster buffers. After teardown the host reads all 14
selected identities and 28 formation identities from the owned STAT mapping,
then closes the owner. This prepares an explicit lifetime boundary for a future
match recipient; the current inspection process still exits after validation.

**Verified native composition and lifetime checks.** Default teams 1/1, reverse
teams 31/31 and forward teams 3/3 repeat in fresh Windows processes. The actual
Win32 input/timer/paint self-test also retains its committed owner. Back creates
neither a selection nor an owner. Occupied output, duplicate-player selection
and invalid schema reject without changing output. The source copy must equal
its loaded input while both are alive and have a distinct address. All retained
identities remain readable after menu buffers are freed. Double close is checked.
No allocator fault injection or sanitizer is claimed.

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus \
  python3 projects/nfl-street/reconstruction/menus/verify_team_field_picker.py \
  --output-dir /mnt/w/Daywater/research-temp/nflstreet-menu-match-owner-20261013 \
  --live-window
```

Private report:
`W:\Daywater\research-temp\nflstreet-menu-match-owner-20261013\verification.json`.
The gate fingerprints the linked owner/roster sources, GAME schema and STAT
inputs, original fallback projections and current bridge evidence. Protected
DOL/DLL/runner and private inputs stay unchanged. Full original menu replay,
manual keyboard testing and live Dolphin integration are not claimed.

## Remaining

Pass `session.roster.selected_rows` to fresh visual/record loading, attach owned
geometry/materials to fresh skeletons and create actors, then load the retained
field choice into the match renderer. There are no actors, field resources,
match scheduler, snap or playable fresh match in this owner. The visible scene
still renders reference players.
