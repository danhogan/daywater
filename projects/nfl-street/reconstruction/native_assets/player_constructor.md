# Owned player constructor and reset state

`player_constructor.c` connects the source model/channel initializer, the
descriptor word transfer, and the tail reset routines in the order used by
`800E1F7C`. `actor_tail_init.c` owns `800D0448` and `800D1330` writes. It
preserves untouched storage and accepts the original default words as data.

The constructor's initial `8017BC54(actor, 1A90, 0)` is a **zero-length
memset**. The final argument is the length. Clearing the whole actor here
would change original behavior. Factory allocation and header initialization
belong to the caller. The native constructor accepts both zeroed and prefilled
storage; it does not copy a saved actor into that storage.

The public input supplies at least 45 descriptor bytes and bounded resource
views. The six model tokens must agree with the descriptor before any writes.
Appearance is a required callback; the connected implementation is exposed by
`player_startup.c`. Rejected callbacks or allocations return zero without
rollback. Original failure behavior is not claimed by this bounded API.

## Original comparison

**Verified controlled behavior:** 1,536 comparisons: 64 resource-presence
combinations, zeroed/prefilled storage and two appearance callback fixtures,
repeated in two fresh Windows processes for each of contexts 14, 15 and 16.
The original DLL executes `800E1F7C`, `800D4E0C`, the channel helpers and both
tail reset routines. The checker supplies allocation, memset and the appearance
callback. Each case compares all 1A90 actor bytes before appearance and after
the callback, the complete 45-byte descriptor, allocation requests and all
allocated bytes. It also checks the initial zero-length memset request.

The capture-only and supplied mutation callbacks validate constructor ordering;
they do not validate the appearance implementation. See
[appearance evidence](player_appearance_init.md) for that separate gate.

Reproduce the constructor gate:

```bash
python3 projects/nfl-street/reconstruction/native_assets/verify_player_constructor.py
```

Private root report:
`W:\Daywater\research-temp\nflstreet-player-constructor-20261012\verification.json`.
Protected DOL, original DLL, runner source and saved RAM inputs were unchanged.
Strict Windows compilation uses `-Wall -Wextra -Werror`.

## Provenance and limits

NFL Street GameCube GNNE69 revision 0, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Reference RAM snapshots came from
Dolphin 2603a. This gate runs the untouched original module offline; it does not
exercise a new live match.

| Source field | Type, size, endian | Procedure and observation | Confidence |
|---|---|---|---|
| Descriptor +04..18 | Six token words, 24 bytes, BE | Supply actual bounded resource views from saved actor headers, then vary their presence; complete model storage agrees | Verified controlled transfer |
| Descriptor +1C → actor +135C | Word, 4 bytes, BE | Controlled descriptor word copied exactly in fresh and prefilled storage; meaning unspecified | Verified controlled transfer |
| Actor +1368..1A8F | 728-byte reset span, mixed BE words and bytes | Original reset routines execute; compare the complete actor including untouched bytes | Verified controlled reset |
| 80252654, 802526B0 | Default words, 4 bytes each, BE | Supply original 3F800000 words and compare each reset write | Verified copied defaults |
| 80270C84..90 | Four default words, 16 bytes, BE | Original inputs are 3F800000, 3F800000, 3F800000, 3E800000; compare full reset output | Verified copied defaults |

Related readers/writers are `800E1F7C`, `800D4E0C`, `800D0448` and `800D1330`.
All six resource views are reference-state inputs. This does not establish
selected-player asset lookup, fresh factory ownership, rendering, complete
appearance binding or a playable match. The descriptor's remaining fields are
controlled fixtures, not a captured fresh game constructor descriptor.
