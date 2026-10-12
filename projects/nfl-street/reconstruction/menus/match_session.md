# Native session continuation

`match_session.c` reconstructs ordinary-mode0 `801E577C` and carries an owned
initialization flag, resolved side contexts, continuation byte and stage
results. Configuration resolution now calls `match_settings.c` through
`match_pipeline.c`; the outer caller is reconstructed `801EA480`.
All three native helpers run without an original module or emulator.

The sequence retains source behavior that affects retry and cancellation:

- The check callback runs only when the initialization byte equals1 and the
  callback is present. Its continuation byte is preserved on status0;
  only continuation1 runs preparation and downstream stages.
- Table clearing and preparation stop on the first error, but roster
  assignment still runs and the initialization byte becomes1. The assignment
  status is ignored by this caller.
- Later errors force continuation0. A cancellation with status0 still reaches
  the outer caller's final update; it does not mean a match was created.
- Nonzero session modes are rejected by the native API. The source branch
  consumes uninitialized local side contexts; no safe interpretation is
  invented for it.

## Verification

Confidence: **verified controlled orchestration**. `verify_match_session.py`
passes39 cases twice across each saved context14/15/16 (234 comparisons),
including five joined outer-caller cases. The original `801E577C` body runs;
configuration, table/resource and stage responses are explicitly supplied.
Complete ordered requests, roster words, result status, continuation and
initialization flag match. Input rosters are synthetic distinct words in
this branch checker, not selected real-player fixtures.

`verify_match_pipeline.py` separately passes nine cases twice per context
(54 comparisons), executing original `801EA480 → 801E577C → 801E51D4`.
It consumes the actual Win32 selection's28 formation words and resolves player
queries from the actual private team1 PLAY table with context1 supplied.
Configuration values are controlled fixtures: they are not the original
menu's live FNIG record. All25 raw configuration words,24 integer update
arguments, the floating argument, both roster arrays, ordered services,
status, continuation and owner flag match. Success includes55 boundaries and
five configuration queries. Query, preparation, stage and final-update errors,
plus cancellation, are reproduced.

The pipeline owns normalized configuration and orchestration. Original query
engine execution, session table insertion, resource I/O, player construction,
appearance, field/ball initialization and a playable match remain unfinished.
No absent backend is treated as a successful default by the public API.

Private reports are beneath `W:\Daywater\research-temp\` in
`nflstreet-match-session-20261012` and `nflstreet-match-pipeline-20261012`.
Builds, original inputs and diagnostics remain on W:.

## Initialization field evidence

Game: NFL Street GameCube GNNE69 revision0. DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`; original source snapshots from
Dolphin2603a. Field: `r13-0x62A8` with supplied r13 `0x803A9A40`, giving
`0x803A3798`; unsigned byte, size1, byte endianness inapplicable. Readers and
writer: `801E577C`. Test procedure: seed flags0/1/2/255 in copied saved RAM,
run original and native ordinary-mode sequences with controlled services,
compare the complete service trace and final byte on success, cancellation
and each error. Assignment writes1 even after preparation errors; resolution
errors preserve the incoming flag. Confidence: **verified controlled field**;
these writes do not establish broader game-state or fresh live-match semantics.
