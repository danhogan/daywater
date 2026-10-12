# Fresh actor factory and connected player creation

`actor_factory.c` owns the allocation request, clearing, header writes and
optional constructor invocation from `801956A4`. The caller supplies a bounded
registry of type records and an allocation service. The original allocator
receives the record's owner token; the native allocator also receives the
minimum span needed for safe clearing. Actual pool allocation is not recreated.

The factory clears exactly the selected record's object size, then writes the
low byte of the type at +00, the supplied header byte at +01 and the BE16
literal 2 at +10. It calls the optional constructor after these writes.
Allocation rejection leaves the supplied storage untouched. Undersized record
or returned spans are rejected by the bounded API; original malformed-input
behavior is not claimed.

`nflstreet_player_create` connects type-zero factory creation to the owned
model/reset/appearance pipeline. It verifies that the allocated object's token
agrees with the appearance self-reference token. If subsequent initialization
fails, the result still identifies the allocated object so the caller can
release it and earlier allocations. This does not implement rollback or a
resource lifetime manager.

## Original checks

**Verified controlled behavior:** 576 isolated factory comparisons (96 cases,
three saved contexts, two fresh Windows processes each). Cases cover type
indices 0, 1, 255 and 256, header bytes 0 and 255, absent/capture-only/mutating
constructors, allocation success/rejection and zeroed/prefilled storage.
Alternate records and callback behavior are controlled fixtures. The type-zero
size and allocation owner come from the actual saved registry.

The checker executes the original factory body and supplies only its allocator,
memset and optional constructor. Complete storage, the allocation owner,
constructor entry state and arguments agree. Extra bytes beyond the selected
size retain their original contents. The type-256 case confirms the header's
byte truncation without interpreting that controlled type as a game enum.

**Verified joined behavior:** 1,536 comparisons execute actual `801956A4`
through `800E1F7C` and `801639C8`, then compare the native factory-plus-player
creation path. All 1A90 actor bytes, model and appearance allocations, registry
state and 89 ordered explicit service requests match. Both the factory's full
clear and the constructor's separate zero-length memset are checked. Factory
allocation, resource lookup and complex bindings remain supplied. The old
constructor-only comparison remains a separate gate.

```bash
python3 projects/nfl-street/reconstruction/native_assets/verify_actor_factory.py
python3 projects/nfl-street/reconstruction/native_assets/verify_player_startup.py --factory
```

Private reports:
`W:\Daywater\research-temp\nflstreet-actor-factory-20261012\verification.json`
and `W:\Daywater\research-temp\nflstreet-player-create-20261012\verification.json`.
Protected DOL, DLL, runner and saved RAM hashes are unchanged. Compiler gates
use `-Wall -Wextra -Werror`.

## Field evidence and limits

NFL Street GameCube GNNE69 revision 0, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Reference RAM was captured using
Dolphin 2603a; these are offline original-module comparisons, not new live
startup tests.

| Field | Type, size, endian | Observation/procedure | Confidence |
|---|---|---|---|
| `(r13-4F14)`, r13=803A9A40 | Pointer word, 4 bytes, BE | Source registry pointer resolves to 806A16C0 in the saved context | Verified reference input |
| Type record +00 | Object size word, 4 bytes, BE | Type-zero size 1A90; compare complete factory clearing in both storage fixtures | Verified controlled factory behavior |
| Type record +08 | Constructor token, 4 bytes, BE | Type-zero entry 800E1F7C; joined original body executes it through appearance | Verified joined invocation |
| Type record +10 | Allocation-owner token, 4 bytes, BE | Type-zero token 80B7F820; verify exact original allocator argument | Verified controlled token transfer |
| Object +00/+01 | Two bytes | Type and supplied header byte written before callback | Verified controlled transfer |
| Object +10 | BE16, 2 bytes | Literal 2 written before callback | Verified controlled transfer; meaning unspecified |

Record stride is 14 hex bytes. Related reader/writer: `801956A4`; connected
constructor `800E1F7C`. The runtime API contains no original RAM addresses.
Source pool storage, selected-player resource lookup, descriptors built from
fresh roster data, actor list insertion and complete match startup remain open.
