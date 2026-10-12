# Connected factory, player state and model binding

`nflstreet_player_bound_create` connects owned actor factory allocation/header
setup, model/channel initialization, tail reset, appearance state and model
binding. The appearance adapter routes its former whole `BIND_MODEL` boundary
into `nflstreet_player_model_bind`; other services still delegate explicitly to
their owners. This path has no Dolphin, ModernGekko or original RAM dependency
in its native implementation.

Resource views, catalogs, scratch/shared storage and allocators are caller
inputs. Cache/query, name comparison, texture/node and renderer operations
remain services. Allocation lifetime and cleanup remain with the caller; failed
initialization retains earlier writes and returns the allocated object handle.
This is not a fresh selected-roster resource loader or playable match.

## Joined original evidence

**Verified controlled behavior:** 3,072 original/native comparisons: 64 model
resource-presence masks, zeroed/prefilled allocation storage, controlled service
mutations and empty/present shared-model state, repeated in two fresh Windows
processes for each of saved contexts 14, 15 and 16.

The untouched original DLL executes actual `801956A4 → 800E1F7C → 801639C8 →
801630B0` and the reconstructed helper bodies. The model-binding entry is not
intercepted or replaced by a whole-function stub. Each case compares full 1A90
actor storage, all model/appearance/binding allocation requests and complete
storage, temporary release records, both registries, scratch/shared state and
resource-view mutations. One combined sequence checks all 2,172 ordered
requests: 88 appearance requests and 2,084 binding requests. Of the binding
requests, 2,070 are name comparisons; these are initialization checks, not
gameplay frames.

Whole copied RAM is compared after excluding isolated CPU stack scratch
816F0000..8170FFFF. Native registry counters are separately checked, then
serialized into the comparison namespace's original global slots. Factory
descriptor substitution and service responses are identical explicit fixtures
in both namespaces. The actual original type-zero allocation-owner token is
read before that substitution; native allocation and appearance self-token
agreement are checked.

The descriptor inverts known saved-actor fields. Base resources and binding
model views also come from saved actors; model/header and name-table values are
original reference inputs. Supplied mutation fixtures exercise downstream
writes without claiming their original resource/render behavior. This gate
does not show a fresh roster descriptor or full original asset acquisition.

```bash
python3 projects/nfl-street/reconstruction/native_assets/verify_player_bound_startup.py
```

Root independently rebuilt and repeated all 3,072 comparisons. Private report:
`W:\Daywater\research-temp\nflstreet-player-bound-startup-20261012\verification.json`.
The separate factory gate passes 576 comparisons, the separate binding gate
96, and the old constructor-only gate still passes 1,536. Existing binding
checks also pass after shared fixture extraction. Strict compilation uses
`-Wall -Wextra -Werror`; native/checker sources and transitive ABI/fixture
headers are fingerprinted. Protected DOL, original DLL, runner and reference
RAM hashes are unchanged; no baseline file was replaced.

## Provenance and remaining work

NFL Street GameCube GNNE69 revision 0; DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Reference snapshots were captured
with Dolphin 2603a. These are offline original-module comparisons, not fresh
live match tests. Confidence is **verified for the controlled joined state and
ordering above**. Field/type/endianness and individual owner evidence are in
[factory](actor_factory.md), [constructor](player_constructor.md),
[appearance](player_appearance_init.md) and [model binding](player_model_binding.md).

Next, reuse the existing selected-player resolver and verified body-archive
binder to own complete [fresh resource views](player_resource_loading.md),
constructor descriptors and resource lifetime. Then connect those views to
selected match actors, field/ball state and gameplay. Current menus preserve
the choices, and the independent scene renders controllable reference players;
this path has not yet replaced those reference players with freshly loaded
selected-roster players.
