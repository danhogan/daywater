# Team menu player preview loader evidence

Target: NFL Street GameCube, GNNE69 revision 0. The original DOL SHA1 recorded by the existing player capture is `46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Original function listings are in the local `cache/binaries/46e3de854998a0f66c9be8c72a1164d22cd29eb4/decompiled/` and `disassembly/` caches; original/private binary and unpacked payloads stay outside this note.

## Verified original call and field flow

`80128F90(preview_slot, player_id, force)` is an original menu preview update entry point. Its cached decompilation does the following in order:

1. Calls `8014E800` to select the player row by `DIGP` using `player_id`.
2. Stores the `DIGP` key in the preview context, reads `DIGT` from that row into a one-byte context field, and separately reads `PHGT` into `80129098`'s per-preview metadata slot.
3. Reads `CFRP`, `ISKP`, and `TBYP`, and obtains the player name via `8014EB58` (which composes fields from the same selected row).
4. Calls `800DF5C4` to check whether the preview system is active, then calls `800DF868(player_id, name, CFRP, ISKP, TBYP, preview_slot, force)`.

`800DF868` keys a 0x50-byte cache record by preview slot. With `force == 0`, it can reuse an existing record when its valid flag, stored `DIGP` key, selected-side flag, and name match. Otherwise it updates that record with the appearance fields and selected-player identity, then calls `800DE6C8` and `800DF218`. `800DE6C8` passes the selected player id to `800DE000`, which selects the same `PLAY/DIGP` row and reads additional fields before initializing resource state. `8012888C` later draws the selected preview object through the `800E20E4` path. These calls establish the original preview lifecycle; the decompiler does not establish semantic names for every cached field.

The `800DE000` listing provides the next concrete asset trace point. From the selected PLAY row it reads the raw tag `0x49544754`, plus `ISKP`, `PLHT`, `PHAR`, `PJEN`, `TBYP`, and `PLEX` (tag spellings use the repository's reversed four-byte convention). It also calls `8014F9D8`, which queries additional PLAY data, and `8014FB4C`, which queries the `LCGP` table using the listed keys `1CHP`, `2CHP`, `1WEP`, `2WEP`, `1RSP`, `2RSP`, `3RSP`, and `1NPP`. The raw `0x49544754` tag is deliberately not assigned the decompiler's ambiguous `GTI` spelling. The raw fields are verified reads; their semantic labels are not inferred from the tag text.

The same `80128F90` entry is reached from multiple UI/script paths. For example, script opcode `0x0e` in `8010C3BC` invokes it with `force=0`; `8010FDDC` and player-selection code in `8010FAF0` invoke it with `force=1`. This supports a shared preview service rather than a renderer unique to one screen. Do not treat every caller as the team-selection menu without tracing its script context.

## Verified asset selector from real PLAY rows

`8014EAC4` reads `YTRP`; when it is zero it returns `PXSP`. Otherwise it reads `CFRP`, calls the `ISKP` accessor (its value is discarded), reads `TBYP`, and returns:

```text
CFRP * 3 + 0x1ad + (TBYP == 1 ? 1 : TBYP == 2 ? 2 : 0)
```

The independent native parser in `team_preview.c` implements this calculation from `PLAY` table fields, filtering rows by `PLAY/DIGT == selected team id`. `team_player_assets_check.c` runs the original `8014EAC4` function for each supplied row from all 32 actual private team DB blocks and compares its output with the native parser. Its expected pass condition is exact agreement for every enumerated row; its report explicitly says that archive/I/O integration is not covered. This is strong evidence for the selector formula, not proof that the result is a filename or a complete athlete identity.

Field evidence:

| PLAY field | Verified use in this path | Confidence |
|---|---|---|
| `DIGT` | Team identity; rows are grouped by this value | verified |
| `DIGP` | Player-row key passed to `80128F90` and used to select the PLAY row | verified |
| `YTRP` | Branch in `8014EAC4` between `PXSP` and the `CFRP`/`TBYP` formula | verified |
| `PXSP` | Return value when `YTRP == 0` | verified |
| `CFRP` | Input to the formula when `YTRP != 0`; also passed to preview-cache setup | verified |
| `TBYP` | Adjustment input to the formula; also passed to preview-cache setup | verified |
| `ISKP` | Accessor runs in the nonzero-`YTRP` branch, but its return is discarded by `8014EAC4`; the preview setup receives it | verified behavior; meaning unknown |
| `PHGT` | Read by `80128F90` and stored through `80129098`; not passed to `800DF868` | verified behavior; meaning unknown |

The field spellings above follow the established reversed four-byte DB tag notation used by the parser. No undocumented field meaning is inferred from the tag name.

## Original portrait and player-named model resources

The team UI registers archive slot `0x0d` from `uis_port.dat` in `80166344`. In the selector branch, the return from `8014EAC4` is passed down as an archive-entry selector. The independent catalog was built from the 32 actual private `db_teams` blocks: 402 PLAY rows were checked, producing 101 distinct selector values, all of which resolved to an entry under the `uis_port` directory. Each selected payload is an MMAP v2 containing one 64x64 format-9 image. I decoded two distinct selected entries through the existing `texture.c` MMAP image decoder and inspected the resulting PNGs at original resolution; both are individual head portraits. This establishes the resource's portrait appearance in the sampled entries, not a pixel match against the running UI. Inspection files and SHA-256:

| Private decoded image | SHA-256 | Visual check |
|---|---|---|
| `W:\Daywater\research-temp\nflstreet-original-menus-20261009\uis_port-00027-0.png` | `5ded73679c523302ab6e4731f9cce7fcd83ace01d441ebf72ec7b3abe4a30e74` | 64x64 RGBA; cropped human head portrait |
| `W:\Daywater\research-temp\nflstreet-original-menus-20261009\uis_port-00093-0.png` | `531fcaec077ec631b38072ddd7510d5536f9f795a0210c3786182a2eeb8e84c3` | 64x64 RGBA; cropped human head portrait |

The complete row-to-entry/hash mapping is private and is kept at `W:\Daywater\research-temp\nflstreet-team-player-loader\selector-catalog.json`.

The 3D models use a separate cached global asset pool. `800E09A8` registers `plamodel.dat`, `plahair.dat`, `plarosterhair.dat`, `plaface.dat`, `plahat.dat`, and `plaglass.dat` through `801A6178`. `800E2928` similarly registers `platex.dat`. These handles are distinct from the UI archive slot 13 path above. The selected actor's default body resources are requested as `PLYRBODY` from the `plamodel` pool by `800DFBE4`; `800DFC60` and `800DFD58` request `PLYRHAND` and `PLYRPAD` from that same pool. `800E00C8` requests `PLYRMODEL` from `plahair` or `plarosterhair` according to the selected resource index. `800E0408` and `800E04F8` use the `plahat` and `plaglass` pools. These resource-type strings and pool registrations are directly present in the original decompilation.

There is a concrete name-derived custom head path. `8014EB58` reads the selected PLAY row's `PLNA` and `PFNA` fixed-width string fields, appends them in that order, uppercases lowercase letters, and removes characters outside `A`–`Z`. `800DF868` caches this resulting string in the preview record at `+0x35`. `800DEFB8` passes that cached string to `800DEBD0`; when it is nonempty, it calls `800E1650`, which constructs `HEAD_MODEL_%s_2700` and searches the `plaface` pool with `801A8088`. If the lookup fails, the function returns the hard-coded fallback index `0x28a`. This directly ties selected-row text to a real original model-name lookup. `800DEBD0` also derives hair model keys from that same cached player string, `PLHT` and `PHAR`; `800E1734` combines those numeric inputs with a derived boolean. `800E177C` uses that boolean to choose `HAIR_MODEL_%s_2700` or `HAIR_MODEL_%sHAT_2700`, and queries the corresponding 1500 variants in `plarosterhair`. The boolean is read through `800DEB04` from derived preview state; this trace does not prove that it is equivalent to `PLHT != 0`. On a successful hair lookup, the code adds `50000` to the entry number before handing it to the generic model loader; `800E00C8` subtracts that marker before loading from the roster-hair pool.

The repeatable private-only checker [`catalog_team_player_resources.py`](catalog_team_player_resources.py) catalogs the original HSH1 table and model `Info` names. [`verify_team_player_resources.py`](verify_team_player_resources.py) regenerates private queries from the 32 actual team DB blocks, builds both harnesses with the host C compiler, and runs the original-module checks. `team_player_lookup_original_check.c` supplies the original `plaface.dat` and `plarosterhair.dat` HSH1 arrays in controlled emulated RAM and calls untouched DOL entry point `801A8088`. The run passed 2,416 calls: uppercase and lowercase head, hair, and hat keys for all 402 actual PLAY rows (2,412 calls), plus four no-match probes. The 387 matched head keys returned the same unique `plaface` indices as the catalog; the 15 absent keys returned `-1`. Hair and hair-with-hat have the same 387/15 split and agree with the `plarosterhair` indices. The harness stubs only the pool mutex and diagnostics boundaries; it does not run live menu actor creation or generic resource acquisition. The 15 head misses take `800E1650`'s explicit `0x28a` fallback according to its original decompilation; the fallback branch itself has not been invoked in this harness. Hair selection remains conditional on the derived preview branch/type. Row-level names, IDs, indices, and hashes are kept only on W at `research-temp/nflstreet-team-player-loader/selected-resource-hash-catalog.json`; the earlier Info-name-only catalog remains at `selected-name-model-catalog.json`.

Oracle provenance for the recorded run: the Linux module was `/mnt/w/Daywater/research-temp/nflstreet-moderngekko-modules/GNNE69/a065ae2f4c192124266a56171aa5eed4e2dccb586d32670c5dab2e2ed3ad980c-c4dcfb815f5c2267/module-build/gGNNE69_recomp.so`, SHA-256 `ce97e16c6bb176b11598b3e816b9d6b43734943bcbc52d66195bf5020bc4b5ce`. `python projects/nfl-street/reconstruction/menus/verify_team_player_resources.py` runs `cc -O2 -I/mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/include <checker.c> -ldl -o <W output>` and invokes each checker with the module plus private W inputs. The generator enumerates every nonzero-team PLAY row in entries 00001–00032, builds the three model-name queries using the cataloged original row normalization, adds upper/lower probes and two misses per archive, and writes the TSV only to W. Oracle RAM is synthetic zeroed 24 MiB MEM1 with the original HSH1 arrays/descriptors, query key, the ASCII lowercase conversion table (`R13 - 0x6538` ready byte and `0x802612c9` table), and resource-ready flag supplied. The original pool-mutex acquire/release (`801AF944`, `801AF9CC`) and diagnostics (`801AF57C`) are the only skipped boundaries for this lookup routine. The protected module, source archives, all 32 unpacked team DBs, generated queries/catalog, sources, and harness binaries are SHA-256 checked before/after; the private JSON report records exact command arguments and hashes. The body-route harness shares that module and checks all distinct actual row TBYP values plus synthetic 7. These are controlled module-oracle tests, not an emulator/gameplay integration run.

## Original body-pool lookup and preview binding

### Controlled constructor proof

The new `team_player_pool_constructor_original_check.c` loads the complete
SHA1-checked original DOL into controlled RAM and executes unchanged
`800DFBE4` and `801EB3F4`. It verifies all three type-0 records and nine ordered
preload requests: key0 requests entries1/2/3, key1 requests4/5/6, key2
requests7/8/9. The original descriptor pointers and terminators are retained;
the record's final word points to the first descriptor, not the last model
record. Nine supplied buffers are read from the original array at `802AB684`.
The native `team_preview_body_entries` planner matches those entry requests;
the separately checked TBYP7 override uses key2. **Verified within this
controlled constructor boundary**. `801A75B0` archive preload, `801C65A0`
resource construction, `801C5EC8` attach prelude and `801C67E4` attachment
remain supplied services. This establishes the numeric resource-entry plan;
it does not prove loading the complete athlete or rendering selected appearance.
`verify_team_player_resources.py` now builds/runs this third checker, invalidates
stale reports on failure, and records DOL/module/native source hashes privately.

`800DFBE4` initializes a set of `PLYRBODY` handles from the table rooted at `PTR_DAT_802760B8`; `800DFE00` resolves those pool handles into the type-0 resource records rooted at `802ab1e8`. `800E088C(0, key)` returns record `802ab1e8 + key * 0x14`, and `800E0734(0, key, variant)` resolves the same record for a requested variant. This is the generic body-pool lookup interface.

The team preview route binds the selected row into that pool directly. `80128F90` reads the selected row's `TBYP` and passes it as parameter 5 to `800DF868`; `800DF868` stores it in the preview cache at `+4`. `800DEFB8` passes that cached value to `8004221C(slot, key)`. In `8004221C`, key `7` is changed to `2`, then the final key is stored in actor `+0x418` and `800E088C(0, key)`'s type-0 record pointer is stored in actor `+0x41c`. This chain identifies `TBYP` as the selected preview's body-pool key, subject to the observed `7 -> 2` override.

[`team_player_body_route_original_check.c`](team_player_body_route_original_check.c) calls untouched original-module `8004221C` and its `80041E18`/`800E088C` callees in controlled RAM. It tested each distinct `TBYP` found among the actual PLAY rows (0, 1, and 2) and a separate synthetic value 7. All four produced the expected actor key and type-0 record pointer; 7 routed to key 2. This proves the local original routine's routing behavior for supplied values, not that a full original game session has loaded or rendered the corresponding pool entries. The mapping from numeric pool keys to specific files such as `HeavyPlayer2700.ea3`, `MuscularPlayer2700.ea3`, and `ThinPlayer2700.ea3` remains **probable**: it correlates the original descriptor indices read by `800DFE00` (1, 4, 7) with unpacked `plamodel` entries, but the archive registration/lookup initialization has not been executed in the controlled harness to join those names to the type-0 record keys.

The general actor constructor `800E1F7C` calls `801639C8(object, init_record)`. The latter copies `init_record + 0x38` into actor `+0x418` and copies bytes `init_record + 0x42`/`+0x43` into actor `+0x1362`/`+0x1363`. `80164000` then uses the actor's `+0x418` value as the type-0 `PLYRBODY` key and chooses its variant from actor state. This constructor/init-record route is distinct from the team preview's explicit `8004221C` binding above; it should not be used to weaken or replace the direct preview evidence.

`800E1178` separately consumes the three indices returned by `800DEBD0` and updates `PTR_DAT_802760DC` model records; `800E00C8` routes those model indices to `plahair` when below `0xC351` and to `plarosterhair` otherwise, stripping the `50000` marker before loading. The body pool (`plamodel` / `PLYRBODY`) and the name-derived head/hair pools therefore have distinct selector inputs. The remaining body gap is joining the selected key's type-0 resource record to the actual archive entry/name and its assembled body/gear resources.


`800E318C` proves a `PLATEX_FACE_CAP_%02d` lookup with `PLATEX_TEMPLATE_FACE` fallback, while `800DD720` creates additional `PLATEX` names from selected DB/LCGP data. The complete selected-row texture key set and palette/material inputs are still unmatched. The existing NAP1 capture cannot fill those gaps because it stores fixed capture state rather than live `DIGP`-bound asset keys.

## Native loader boundary and concrete next implementation

The current `native_assets/player_assets.py` and `export_player.py` identify original model display-list geometry and texture planes by matching a captured frame. The resulting `NAP1` profile is a fixed captured draw/pose/material profile; it does not bind that captured actor to a selected `DIGP` athlete. The existing render report marks body/hair source grouping probable, uses a `BaseHead1500` template for 92 head draws, and omits the unresolved secondary skin index plane on 66 draws. It therefore cannot claim selected-player appearance fidelity.

The next native change should add a separate selected-row loader beside the captured `NAP1` exporter. Its concrete initial input/output contract is:

1. Accept `(team DB, selected team id, selected DIGP)`, resolve exactly one PLAY row after checking `PLAY/DIGT`, and preserve the original fixed-width `PLNA`/`PFNA` bytes plus all raw appearance fields.
2. Implement the verified portrait selector as an opaque field and resolve it through `uis_port` archive index 13. The selector-to-entry mapping and format evidence are already catalogued privately; do not feed this index to a model-pool loader.
3. Build the exact original normalized name using the `8014EB58` composition rules. Query `plaface` with `HEAD_MODEL_<name>_2700`; the lookup result is verified against original `801A8088`, and `800E1650` supplies fallback index `0x28a` on a miss. Query `plarosterhair` with both name-derived HAIR keys, selecting the hat suffix from the original derived `800DEB04` flag and retaining the `+50000` roster-pool namespace where subsequent `PLYRMODEL` selection requires it.
4. Use selected row `TBYP` as the verified `PLYRBODY` type-0 key, applying the original `7 -> 2` override. Resolve that type-0 handle to its archive entry and name, then connect its body variants, hands, and pads; do not infer a body class from `PLHT`/`PHAR` or from descriptor order alone. Continue tracing the name builders below `800DD720`/`800E318C` before implementing material, palette, or overlay selection.
5. Re-run the controlled original lookup checker when the archive parser or query construction changes. Validate portrait index through original archive acquisition and map the verified body key through full original archive initialization. Compare chosen geometry and textures against captured bytes only after every selected `DIGP` has its original resource keys. Keep payload dumps, selector-to-file maps, and render comparisons on private `W:`; commit only code and non-proprietary evidence summaries.

The profile format should be revised only after step 3 supplies the original asset dependency set. Until then, pass selected row identity and the opaque selector through the loader interface, and report unresolved personalization rather than silently reusing the fixed capture's head, palette, or textures.

## Evidence files

- Original cached listings: `80128f90.txt`, `800df868.txt`, `800de6c8.txt`, `800de000.txt`, `800debd0.txt`, `800defb8.txt`, `800e1650.txt`, `800e177c.txt`, `800e09a8.txt`, `800dfbe4.txt`, `800dfe00.txt`, `800e0734.txt`, `800e088c.txt`, `800e00c8.txt`, `8004221c.txt`, `80041e18.txt`, `800e1f7c.txt`, `801639c8.txt`, `80164000.txt`, `801a6178.txt`, `801a8088.txt`, `801a8b20.txt`, `801aa884.txt`, `8012888c.txt`, `801289a4.txt`, `8014eac4.txt`, `8014e800.txt`, `8014e7b0.txt`, `8014ee78.txt`, `8014fc3c.txt`, `8014ec40.txt`, `8014f028.txt`, `8014eb58.txt`, `80129098.txt`, `80166344.txt`, `801d54a4.txt`, `801d5dc0.txt`, and `801a69c0.txt`.
- Existing independent DB/formula parser: [`team_preview.c`](team_preview.c), [`team_player_assets_check.c`](team_player_assets_check.c), and [`team_preview.h`](team_preview.h).
- Existing captured renderer limitations: [`native_player_render.md`](../native_assets/native_player_render.md), [`export_player.py`](../native_assets/export_player.py), and [`player_assets.py`](../native_assets/player_assets.py).
