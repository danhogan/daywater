# Selected-player hair resource plans and ownership

`hair_resources.c` reconstructs original `800DEBD0` / `800E177C` / `800E1734`
resource-index planning, then independently decodes, relocates and attaches the
three requested TMdl resources. The owner releases all three files on close.
Caller-supplied token identities replace original RAM pointers. The loader
requires only local `plahair.dat`, roster `plarosterhair.dat` and the plan;
no snapshot, emulator, original DLL or Python catalog is needed at runtime.

## Selection boundary

The planner receives the normalized name, raw PLHT/PHAR inputs and an explicit
16-bit derived headwear state. It owns these original branches:

- States11/12 bypass named lookup and select the original special triplet.
- Other states use `PHAR*34 + PLHT*4 + (state ? 3 : 1)` for slot0,
  slot0+1 for slot1, and `PHAR*34 + (state ? 34 : 33)` for slot2.
- Name lookup independently replaces slots0/1 with the2700/1500 entries,
  using the HAT suffix when state is nonzero. A missing entry retains its
  fallback. Names are trimmed at the first space, matching original query
  construction; the native API does not mutate the caller's name.
- Roster entries retain the original +50000 namespace marker. Routing strips
  it only when the index is strictly greater than50000;50000 retains local
  routing and cannot be misread as roster entry0. Zero is absent and FFFF is a terminator,
  not a model index. All three indices are handled independently.

The all-roster check supplies states0/1 explicitly and applies the observed
PHAR63→12 accessor rule. It does **not** infer headwear state from PLHT, PHAR,
or whether a named asset exists. Original state selection through `800DDEBC`
and its DB/LCGP lookups remains unfinished. No automatic variant-selection or
selected-athlete rendering claim follows from this resource loader.

## Verification

```bash
python3 projects/nfl-street/reconstruction/native_assets/verify_hair_resources.py
```

The verifier checks source, five archives,32 private DB blocks, three saved
contexts and protected original DOL/DLL/runner hashes before and after. All
outputs stay under `W:\Daywater\research-temp\nflstreet-hair-resources-20261012`.

**Verified native data coverage:**402 regular-roster rows × two explicit states
produce804 plans and2,412 independently processed models per fresh Windows
process, repeated twice. The joined checker also retains the earlier selected
face/body assembly and fresh face-binding checks.

**Verified controlled original planning:** for every selected row, states
0/1/11/12 match the original three output indices:1,608 plans per process.
Original `800DEBD0`, its accessor helpers, fallback arithmetic, name formatting
and special-state branches execute. `801A8088` lookup results are supplied
from the independent HSH1 reader using the original generated query. This
checks planning with supplied archive lookup, not original archive registration.
The existing generic HSH1 lookup evidence remains separate.

**Verified controlled model processing:** all1,634 distinct archive/index
combinations used by states0/1 match the complete original relocated bytes and
52-byte attached views. Original `801C5EC8` and tested chunk handlers execute
without relocation stubs; `801C67E4` attachment also executes. Three contexts
(14/15/16), twice each in fresh Windows processes, yield9,804 complete file/view
comparisons and9,648 planning comparisons. Special-state11/12 plans are compared
but their files are not included in these loading counts.

Game: NFL Street GameCube GNNE69 revision0. DOL SHA1:
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Comparison snapshots originated
from Dolphin2603a; no new live emulator integration is claimed.

## Field provenance and limits

| Field | Primitive/size, byte order | Reader/writer | Confidence and interpretation |
|---|---|---|---|
| `802AAAA2 + slot*68` | u16/2, BE |800DE000,800DEA3C | Verified controlled input to planning; populated from PLHT accessor8014EC90 |
| `802AAAA4 + slot*68` | u16/2, BE |800DE000,800DE934 | Verified controlled input to planning; PHAR accessor8014ECB8 aliases63 to12, probable static field flow |
| `802AAAA6 + slot*68` | u16/2, BE |800DEBD0,800DEB04 | Verified controlled branch input; its actual derivation remains probable/static |
| Plan slots0/1/2 | u32[3]/12, BE in original |800E1734,800E177C,800DEBD0 | Verified indices in the controlled comparisons above |
| Descriptor+C,+5C,+AC | u32/4 each, BE |800E1178,800E00C8 | Probable original consumer routing, not newly exercised full acquisition |

Global addresses above describe the GNNE69 original module, not native runtime
addresses. Observed outputs and complete hashes are in the private report.
Malformed-input rejection and the native owner's cleanup are defensive behavior;
no original corrupted-asset handling equivalence is claimed.

The loader has not executed full `800E00C8` acquisition/pool/descriptor updates,
postattachment hair binding to its original target, material or texture binding,
or drawing. Those remain explicit connections before fresh actor creation.
Menus and the reference scene still work; fresh selected match startup remains
unfinished. The generic Daywater platform is unchanged.
