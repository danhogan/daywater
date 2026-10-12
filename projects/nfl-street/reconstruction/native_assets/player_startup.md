# Connected native player initialization

`player_startup.c` connects the owned model/channel initializer, tail reset and
appearance initializer through one API. It builds their state in caller-owned
storage; it does not copy a saved actor. The native code has no dependency on
Dolphin, ModernGekko or original RAM addresses. Original game data enters as
bounded resource views and catalog words supplied by the caller.

The connected `nflstreet_player_create` API now supplies factory clearing and
header ownership before this initialization. Its separate 1,536-case gate
enters through the actual original factory. See [factory evidence](actor_factory.md).

Direct `nflstreet_player_initialize` leaves the factory header with its caller;
`nflstreet_player_create` owns that header. Resource lifetime remains external. Complex
model, texture and node binding operations are required services. A rejected
service returns zero with earlier writes retained. This is a checked player
initialization path, not complete selected-player asset loading or a playable
fresh match.

## Joined original comparison

**Verified controlled behavior:** 1,536 comparisons, covering all 64 model
resource-presence masks, zeroed and prefilled storage, and two binding service
fixtures. Each of saved contexts 14, 15 and 16 runs twice in fresh Windows
processes. The original DLL executes `800E1F7C` through `801639C8`, including
the reconstructed model/channel, reset, vector, node, table, mapping and
registry helper bodies. The appearance entry is not replaced by a stub.

Each case compares:

- All 1A90 actor bytes, including fields untouched by the original.
- Every model allocation request and its complete storage.
- Both appearance allocations and their complete storage.
- Both registry spans and their counters.
- All 89 ordered resource/binding service requests and outputs.
- The original constructor's zero-length memset request.

Resource lookup, rendering bindings and allocation are supplied in both paths.
The mutation fixture applies explicitly controlled writes at binding services;
those writes are not reconstructed gameplay behavior. The descriptor inverts
known writes in reference actor state, with presence masks applied to its six
resource tokens. It is not a captured fresh constructor descriptor. Source
catalog values and resource views remain private on W:.

The verifier records native/checker source hashes, transitive fixture and ABI
headers, executable hashes, input hashes and repeated log hashes. It rejects
changes to protected originals or source files during the run. Shared checker
fixtures serve both isolated appearance and joined initialization checks.

```bash
python3 projects/nfl-street/reconstruction/native_assets/verify_player_startup.py
python3 projects/nfl-street/reconstruction/native_assets/verify_player_appearance.py
```

Root independently rebuilt and passed both gates. Private reports:
`W:\Daywater\research-temp\nflstreet-player-startup-20261012\verification.json`
and `W:\Daywater\research-temp\nflstreet-player-appearance-20261012\verification.json`.
The isolated appearance gate passes 48 comparisons, including actual catalog
rows 0 and 1. All builds use `-Wall -Wextra -Werror`.

## Confidence and next boundary

NFL Street GameCube GNNE69 revision 0, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`; saved reference RAM came from
Dolphin 2603a. These checks execute the untouched original module offline,
without a fresh live Dolphin startup. Protected DOL, DLL, runner and reference
RAM hashes are unchanged. No baseline DLL or source was replaced.

Confidence is **verified for the controlled initialization behavior** above.
See [constructor fields and reset evidence](player_constructor.md),
[model/channel evidence](actor_model_init.md) and
[appearance fields, catalog and service limits](player_appearance_init.md).

Remaining work is to own selected-player resource lookup and model/texture
binding, produce descriptors from fresh selected roster data, create factory
and session state, and connect field, ball and gameplay initialization. Current
menus preserve the selected teams, formations and field; the independent scene
uses reference players. This module does not yet connect those selections to
fresh playable match actors.
