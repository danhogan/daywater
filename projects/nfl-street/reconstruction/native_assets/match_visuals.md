# Both selected lineups as simultaneous native visual owners

`match_visuals.c/h` joins the retained `quick_match_session` roster with
`selected_player_visual_from_match` for all 14 selected players in side-major,
menu lineup order. The result owns 14 face/hair resource groups,350 texture
slots, 630 material instances and their material allocations at the same time.
Processed body variants and source catalogs/archives remain borrowed.

## Ownership boundary

Each caller supplies one disjoint logical-token namespace per player and an
explicit player-record side input. These are native host reservations, not
original RAM addresses. All namespaces exclude all borrowed body spans. Context
base tokens must fit their namespaces; decoded face/hair/slot/scratch spans
are checked after staging. Individual owners retain their existing internal
geometry/material collision checks. Close releases all 14 owners and can repeat.

The aggregate queues flush notifications until every player has loaded and all
ranges are checked, preserving per-player and per-call order on replay. Earlier
players cannot publish flush notifications when a later player fails to load.
A real replay callback can still have side effects before a later callback
fails; the staged owners are freed and output stays unchanged. Deferred
notifications are a new host contract. This does **not** establish original
cache timing, scratch-buffer visibility, GPU uploads or draw behavior. Callbacks
must support deferred token notifications; they are not byte upload payloads.

## Evidence — 2026-10-13

**Verified native composition**, using the actual Win32 menu's retained teams 1/1
selection. The original menu source buffers are freed after building the owned
session, before loading the 14 visual owners. Body variant 0 and constructor
side inputs 0/1 are explicit fixture inputs, not inferred controller semantics.
Each retained player equals a separately built single-player owner in geometry,
texture bytes, bindings, material instances and material allocation bytes/tokens.
The positive build replays 2,162 deferred notifications.

Six guards cover occupied output, overlapping player namespaces, a context token
outside its reserved namespace, decoded face data exceeding its namespace,
a failure at the 14th player's resolver after earlier players staged, and a
failure during real notification replay. Pre-replay failures emit no real
notifications. The final failure verifies that earlier notifications can have
occurred while output remains empty. Cleanup and double close pass. No allocator
fault injection or sanitizer is claimed.

The gate requires current source-matched proofs for original GAME copying, the
menu-owned roster and selected single-player assembly. This is not a new full
original 14-player startup comparison or live Dolphin integration. The original
DOL/DLL/runner and private inputs remain unchanged.

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus \
  python3 projects/nfl-street/reconstruction/native_assets/verify_match_visuals.py
```

Two fresh Windows processes must match. Private report:
`W:\Daywater\research-temp\nflstreet-match-visuals-20261013\verification.json`.
The QMS1 input is generated privately from the native menu selection, not a save
or publicly distributed game asset. Builds, assets and diagnostic logs stay on W:.

## Remaining

The loading check consumes the emitted menu selection; the running menu host
still exits after retaining/validating its roster. Connect this asset owner to
that host's match recipient, attach fresh skeletons/create actors and draw the
selected players. Keep catalogs/archives and shared body pools alive until all
visual owners close. There are no actors, animation channels, field resources,
match scheduler or playable fresh match in this owner. The visible native scene
still renders reference players.
