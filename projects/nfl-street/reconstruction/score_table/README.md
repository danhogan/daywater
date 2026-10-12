# Named score table wrappers

**Status:** semantic C candidates; binary match untested. Target: NFL STREET
GameCube `GNNE69`, North America revision 0, DOL SHA-1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`.

| Function | DOL file offset | Size | Observed behavior |
|---|---:|---:|---|
| `0x80053128` | `0x50128` | 88 bytes | Initializes a local 32-bit result to zero, selects `CSHG` when side is zero and `CSAG` otherwise, invokes the query dispatcher using a `FNIG` select template, then returns the result. |
| `0x80053180` | `0x50180` | 76 bytes | Selects the same field and invokes the dispatcher using a `FNIG` update template with the caller's value. |

The DOL contains the exact field identifiers `CSHG` (`0x43534847`) and
`CSAG` (`0x43534147`) and the two `FNIG` query templates at `0x802470D0`
and `0x802470F0`. These relationships are **verified static evidence**. The
meaning of all other `FNIG` fields and most of the query engine implementation
at `0x801B4510` remain unknown. A live side-1 score byte location in
`FNIG`-linked records is described below.

Live breakpoints on the tested 6–0 PAT path add a bounded observation: after
the side-1 getter loaded its local result into `r3` at `0x8005316C`, the
register held 6. In a successful conversion the setter entry received
`r3=1`, `r4=7` after the RAM score writer ran; in a failed conversion it
received `r3=1`, `r4=6`. This is **verified** wrapper value flow for these
states. The paired captures described below locate the `FNIG`-linked record
and show an update during the setter call. The
[score evidence](../../../../research/nfl_street/score.md) records the
breakpoint logs and limits.

Paused MEM1 captures show a score-shaped pair changing 6→7 during the tested
setter call. A `FNIG`-tagged descriptor points to its containing record.
In fresh matches, a relocated `FNIG` descriptor points to a similarly
shaped record whose bytes also track scores; the relocated `EMAG` object
alone did not. The pointer and score relationships are **verified** in
retained captures. The first byte's identity as side-1 `CSAG` is **verified**
for the tested path: the query engine loaded from its address at `0x801B94F8`
before the getter returned 6, then performed a direct store at `0x801B95E0`:
`stw r11, 0(r22)` wrote `07 00 FF FF` to the pair address after the setter
received 7, replacing `06 00 FF FF`. In both tested allocation histories,
the `FNIG` schema assigns 8-bit `CSAG` to bit offset 128 (`+0x10`) and
8-bit `CSHG` to bit offset 136 (`+0x11`). This verifies the second byte's
named layout and its score values in retained captures; its direct query
execution remains unobserved. The
[offline scanner](../../tools/find_named_score_candidate.py) follows these
descriptor pointers; their addresses are not used by the runtime observer.

The field choice here is `side == 0` versus all other values. The RAM score
reader/writer instead distinguish `side == 1` versus all other values. Thus a
side value outside `{0, 1}` would make the named table and RAM helpers choose
different sides. The original code has this asymmetry; no caller contract
proving that only 0/1 reaches these helpers has been established.

`score_table.c` is a **candidate** source model. It calls `fnig_query_select`
and `fnig_query_update`, a portability boundary replacing the original
variadic query dispatcher. Those functions are provided by
`../../native/fnig_store.c` for the Windows score-path build. That in-memory
store is a host implementation of only these two fields; it is not a
decompilation of the original database engine or proof that this is the
game's source of truth.
