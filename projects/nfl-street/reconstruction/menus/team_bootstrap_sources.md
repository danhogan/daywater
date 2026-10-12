# Team activation source dependencies

This note records the source paths established from the cached original
decompilation and the local original DOL. It does not assign gameplay meanings
to raw database tags or runtime indices.

Game: NFL Street GameCube GNNE69-r0, original DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Confidence: **probable** for the
static source dependency interpretation below; no fresh full menu startup has
exercised these sources together. Controlled stage comparisons are documented
separately in [team screen progress](team_screen_progress.md).

## Activation, assignment, and pose are separate inputs

`8011E460(side, output)` conditionally rebuilds the roster when its side flags
request it. It calls `8011E3E8` to obtain a list, then
`8003F6B8(side, list, mode)`; it always calls `8003F4EC(side)` afterward.
`8003F4EC` copies 14 words from `0x802A4430 + side * 0x38` to
`0x802A43C0 + side * 0x38` and stores a count of 7 at
`r13 - 0x7BF0 + side * 4`.

`8003F6B8` is a wrapper for `8004278C(mode, list, &0x802A4430 + side * 14)`.
`8004278C` builds weighted rows through `80042568`, then consumes seven bytes
at `r13 - 0x7BAC` and seven at `r13 - 0x7BA4`. These are initialized DOL data
at `0x803A1E94` and `0x803A1E9C`; their first seven bytes are
`00 01 02 04 03 06 05` and `05 00 01 04 06 02 03`, respectively. Each byte `k` is passed to
`800422D4`; the returned word is written to output index `k` for the first
array and `k + 7` for the second. Their values must not be treated as verified
role names or as weighted-row ordinals.

After the optional rebuild/copy, `8011E460` independently reads pose constants
from `0x8028271C + side * 0x23` as a `float *`. Since the global is float
typed, `0x23` means35 words: seven records of five32-bit words per side.
For each record it calls `80128CF4` with the first three floats and index
`side * 7 + record`, `80128E44` with the fourth float, then `80129068` with
the index and fifth raw word. Original-module comparisons confirm that the
visibility argument is passed in GPR4 as raw bits, rather than a float argument.

The complete14-record raw table remains private on W: at
`research-temp/nflstreet-team-screen-20261010/original-team-poses.json`.
That artifact records the hash-checked DOL source,280-byte read,140-byte side
stride,20-byte record stride, primitive types and confidence. It can supply
fresh original pose inputs without reading a saved match. The final raw word
is recorded as an opaque integer passed to the visibility service.

`8011E3E8` reads roster triples at
`0x802F50D4 + side * 0xF4`: it copies entries whose second word is signed
nonnegative, then writes `0x7FFF` as the terminator. The pose table above is a
separate fixed DOL input; it is not derived from these roster IDs.

## Raw query dependencies in `80042568`

`80042568(mode, player_ids, output)` opens a query with raw tags
`0x52545354`, `0x59545453`, `0`, `0`, `0x54415453` through `80150ED4`.
`80150F34` queries tag `0x59545453` with `mode` and argument/index zero.
The player-data context is selected by `80004668()`: zero selects raw tag
`0x454D4147`, nonzero selects `0x54415453`; `8014E67C` opens that context.
For each supplied player ID, `8014E800` queries raw tag `0x44494750`.

`8014F5C8` supplies the ten adjusted values consumed by the weighting loop;
its lower rating preparation reads ten base fields and twelve gear-query IDs.
For each index0 through13, `80150F6C` reads a four-byte tag
from the original DOL constant array at `0x80285D2C + index * 4` and calls
`80149614`. The 14 exact byte tags are:

```text
0 ZBQP   1 ZBRP   2 FRWP   3 BRWP   4 LLOP   5 CLOP   6 RLOP
7 LLDP   8 RLDP   9 FBLP  10 BBLP  11 LBDP  12 CBDP  13 RBDP
```

These are literal four-byte values from the DOL; no category or position names
are inferred here. `8014FCCC` reorders ten base values by indices
`[0, 4, 1, 3, 2, 5, 6, 7, 8, 9]` and passes them with the loop index to
`801E0D00`. The returned value is combined as
`(uint32_t)(801E0D00_result * (tag_value + 100)) / 200`, preserving the original
32-bit integer wrap before unsigned division, then
stored in the weighted row. This ordering and formula follow the original
`80042568`/`8014FCCC` decompilation.

The raw TSTR modifier lookup and fresh ROBP curve are now connected in
`team_bootstrap_weights.c`. Formation orders are initialized DOL data; fresh
startup reads them directly. Controlled original weighted-row comparisons
supply resolved DB getters/allocation and execute original scoring/weighting;
full original registration/startup remains open.
See [fresh source startup](team_screen_startup.md) for the updated scope.
