# Owned player texture and material surface

## Purpose

`player_surface.c/h` joins the verified gear/face request, slot loading,
palette recoloring, texture-set binding, default settings and actor material
hookup into one native lifetime. This is a host ownership type, not a recovered
original game structure. No original module or snapshot is read by its API.

Inputs are locally decoded archives and private catalogs, a completed gear
plan or player record, the normalized selected roster name, and caller-reserved
token ranges. `nflstreet_player_surface_from_player` consumes a complete record
from `player_record_build` and a name from the selected geometry manifest. It
builds the gear plan, uses the existing named face lookup in `platex`, and
composes colors from record +4C..+63. Slot0 takes the plan's fallback; slot1 takes
the separate face request. The plan's record +65 result stays caller-owned.

The result owns all 25 MMAP destinations and each allocation behind their
texture sets, plus the material bindings and 45 actor material instances.
Private catalog strings remain borrowed. Material allocation tokens advance
by 32-byte strides; allocator fill is explicit because original functions
retain unwritten bytes. Tokens are identity namespaces, not discovered RAM
addresses. The caller reserves these spans against actor/geometry resources;
the owner rejects collisions among its destination, scratch and material spans.

Open publishes only after all stages succeed. Partial failure releases new
material blocks and texture storage, preserving the output. An occupied output
rejects; close releases and clears it and can be repeated. Flush callbacks can
have external side effects, which cannot be undone on later failure.

## Evidence

**Verified host composition and lifetime, GNNE69 revision 0.** Run:

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus \
  python3 projects/nfl-street/reconstruction/native_assets/verify_player_surface.py
```

Each process checks:

- 28 builds: the 14 captured players' requests and color states with allocator
  fills 00/FF. Complete destination bytes, texture-set blocks and identities,
  material instances and ordered flushes match the separately exercised stages.
- 307 rejection checks: 28 occupied-output rejections, every callback-failure
  position, a partially exhausted material namespace, a scratch collision and
  all 125 material allocation failure positions. Output remains unchanged and
  instrumented material allocations balance after cleanup.
- 15 semantic record/name builds: 14 missing-name fallback combinations and a
  present face name derived from a disc roster row. These are controlled inputs,
  not proof that a fresh match chose that actor.

Three Dolphin 2603a saved contexts repeat twice in fresh Windows processes.
The gate requires a passed, source-matched `verify_slot_textures.py` report,
which compares constituent stages against the untouched original module and
live captured state. It verifies private input hashes and protected original
DOL/DLL/runner hashes before and after. No new live Dolphin gameplay was run.

Private report:
`W:\Daywater\research-temp\nflstreet-player-surface-20261013\verification.json`.
The Windows checker aliases malloc/free only in the owner translation unit to
check material allocation balance and inject failures. This is not a sanitizer
result; the host AddressSanitizer library was unavailable.

## Remaining connection

Use this owner with fresh GAME/player records and the geometry/skeleton owner
inside actor creation. Currently the surface and selected geometry are separate
owners; full descriptor/resource registries and draw submission remain explicit
services. The `8017B570` pool, OnePass/TwoPass TEV setup and `801C7AD4` draw path
are not implemented here. No selected-player rendering or playable fresh match
is claimed.
