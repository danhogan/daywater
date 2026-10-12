# NFL Street (GameCube): displayed team scores

## Finding

**Name:** In-match left and right team score fields  
**Confidence:** `verified` for the tested game image and Dolphin build  
**Date:** 2026-09-29 through 2026-10-04  
**Researcher/agent:** Codex

## Game identity

| Item | Evidence |
|---|---|
| Title | `NFL STREET` in the disc header |
| Platform | GameCube |
| Game ID / region | `GNNE69`; region code `E` (North America) |
| Disc number / revision | `0` / `0` from header bytes 6 and 7 |
| ISO SHA-1 | `813c4511ba3135059a13de424555647e8252254b` |
| DOL SHA-1 | `46e3de854998a0f66c9be8c72a1164d22cd29eb4` |
| DOL section containing pointer slot | Initialized data section 10, `0x803A1A40..0x803A3D3F` |
| Dolphin build | `2603a`, matching the six supplied savestate headers |

The normal Daywater container has Dolphin 2609, which could not restore the
2603a states. Live verification used the official
[Dolphin 2603a Linux build](https://dolphin-emu.org/download/release/2603a/)
in the existing container. Daywater's Dolphin binary was not replaced.

## Memory representation

The big-endian 32-bit pointer at `0x803A29C8` is in an initialized DOL data
section (its initial DOL bytes are zero). In every
recorded Bears–Lions match state it contained `0x803B6C20`. PowerPC code
reads and writes two unsigned 16-bit fields at that base:

```text
base        = read_be_u32(0x803A29C8)
Lions score = read_be_u16(base + 0x3C)  # right side of scoreboard; game side 0
Bears score = read_be_u16(base + 0x3E)  # left side of scoreboard; game side 1
```

| Field | Observed address | Offset from pointer target | Type | Size | Endianness |
|---|---|---:|---|---:|---|
| Right team score (Lions) | `0x803B6C5C` | `+0x3C` | unsigned 16-bit integer | 2 bytes | big endian |
| Left team score (Bears) | `0x803B6C5E` | `+0x3E` | unsigned 16-bit integer | 2 bytes | big endian |

The original byte scan found the low bytes at `+0x3D` and `+0x3F`; Ghidra
disassembly exposed their respective high bytes at `+0x3C` and `+0x3E`.
All observed high bytes were zero. The pointer target stayed at
`0x803B6C20` in the tested states; read it from the static pointer slot
rather than hard-coding that target. Its structure name, lifetime outside a
match, and behavior in other revisions are unknown.
The relationship of these fields to a separate score or statistics system
remains under investigation.

## Verification procedure

1. Confirm the ISO game ID, revision, and SHA-1. Confirm that the six supplied
   savestates identify Dolphin 2603a.
2. Restore each supplied state in a separate Dolphin 2603a process. Capture
   its scoreboard and read live MEM1. The Bears and Lions score changes identify
   the left and right fields independently.
3. Restore the 7–6 state, quit through the game menu, and start a new
   Bears–Lions Quick Game on EA Field. At a visible 0–0 play-selection screen,
   capture a 24 MiB MEM1 baseline. Capture additional 0–0 states on third and
   fourth down as negative controls.
4. In a separate run of the same Quick Game, capture the Lions' visible 0–2
   score and MEM1. Compare 0–0 and 0–2 images for values that changed from
   zero to two. Continue the recreated match to capture a visible 0–6, then
   0–17, with corresponding MEM1 images. Intersect candidate byte and integer
   sequences across these states and the negative controls.
5. Find the left score independently using the Bears' 0→6→7 sequence in the
   six supplied states. Require it to remain zero in the new-match images.
6. Follow the pointer at `0x803A29C8` in every saved MEM1 image and read
   big-endian halfwords at `+0x3C` and `+0x3E`. Independently read the
   candidate from the live 0–17 Dolphin process. The pointer and both score
   fields agreed with every recorded scoreboard.
7. Inspect Ghidra disassembly for the same DOL SHA-1. Confirm `lhz` reads at
   `0x80104280` and `0x8010428C`, and `sth` writes at `0x801042AC` and
   `0x801042A0`. Trace direct callers of the reader and writer.

All snapshots were taken from live Dolphin memory. The six supplied savestates
were also independently decompressed with LZ4 during the initial search;
the serialized MEM1 image occurs at varying file offsets, so comparisons used
GameCube addresses rather than raw savestate offsets. The later validation
used fresh live reloads and new matches. No game behavior was patched.

## Observations

The table lists scores visually read from the game. Every corresponding RAM
read through `read_be_u32(0x803A29C8)` matched. The pointer target was
`0x803B6C20` in all twelve recorded images.

| State | Visible scene | Bears on screen / RAM u16 | Lions on screen / RAM u16 |
|---|---|---:|---:|
| Supplied `19bb4a7c` | First down before play | 0 / `0000` | 0 / `0000` |
| Supplied `8287f395` | Second down, play underway | 0 / `0000` | 0 / `0000` |
| Supplied `ca793e43` | PAT play selection | 6 / `0006` | 0 / `0000` |
| Supplied `27bb155a` | First down after PAT | 7 / `0007` | 0 / `0000` |
| Supplied `9cf343e0` | Play selection | 7 / `0007` | 6 / `0006` |
| Supplied `a419c605` | Pause menu | 7 / `0007` | 6 / `0006` |
| New match | First down, play selection | 0 / `0000` | 0 / `0000` |
| New match | Third down, play selection | 0 / `0000` | 0 / `0000` |
| New match | Fourth down, play selection | 0 / `0000` | 0 / `0000` |
| Separate new-match run | After score change | 0 / `0000` | 2 / `0002` |
| New match | PAT play selection | 0 / `0000` | 6 / `0006` |
| New match | Later play selection | 0 / `0000` | 17 / `0011` |

The Bears score changed 0→6→7 while the Lions score stayed zero in the
supplied states. The Lions score changed 0→6 there while the Bears stayed
seven. In new matches, the Lions score changed 0→2 and 0→6→17 while the
Bears stayed zero. Down, menu, gameplay, and pause transitions did not make
the candidate fields change when the visible score stayed constant. A fresh
live read at the visible 0–17 screen returned `00 11 00 00` from
`base + 0x3C..+0x3F`; `0x11` is 17 in decimal.

The 0→2, 0→6, and 0→17 transitions are enough to rule out a score value that
merely happened to equal a down, gamebreaker meter, or single touchdown count.
The 0–17 observation also exposes a misleading 32-bit candidate at
`0x803BB1B4`: it read 6 at both visible 0–6 and 0–17. Its meaning is
unknown; it is not the current displayed score.

## Rejected `EMAG` locator

The first six supplied states also had adjacent bytes at `0x804A21D0/D1`
that matched the Bears and Lions scores. A pointer at `0x80421B44` led to
object base `0x804A2190`, with `EMAG` at `+0x08`. A static-data-rooted linked path
from `0x803A35D8` reached this object. It moved to `0x804B3D50` in later
matches, and its `+0x40/+0x41` bytes still read `00 00` at a visible 0–0.

When the scoreboard later showed 0–2, 0–6, and 0–17, those relocated
`EMAG +0x40/+0x41` bytes remained `00 00`. The object's identity and
relationship to the supplied match are unknown. It must not be used as a
general score locator. The heap pointer slot and object address are not
permanent constants.

## Retained evidence

Screenshots and raw 24 MiB MEM1 images are outside the repository under
`W:\Daywater\research-temp\`. Filenames start with `bears-lions-ea-field-`
for the new match and `original-` for the six supplied states. The most
informative pairs are:

| Visible score | Frame | MEM1 image |
|---|---|---|
| 0–0 | `bears-lions-ea-field-0-0.png` | `bears-lions-ea-field-0-0.mem1` |
| 0–2 | `bears-lions-ea-field-0-2.png` | `bears-lions-ea-field-0-2.mem1` |
| 0–6 | `bears-lions-ea-field-0-6.png` | `bears-lions-ea-field-0-6.mem1` |
| 0–17 | `bears-lions-ea-field-0-17.png` | `bears-lions-ea-field-0-17.mem1` |

The third- and fourth-down 0–0 images are retained as negative controls.
An additional read-only live PAT run is retained as
`score-poll-run1-success.mem1` and `score-poll-run1-52.png`. The frame shows
the one-point conversion announcement shortly before the RAM score update;
it still shows 6–0. A later completed frame with the visible 7–0 score is
`score-breakpoint-pat-forward-last.png`.
No ISO, proprietary game asset, savestate, RAM image, or authentication state
was added to the repository.

## Ghidra relationships

These are functions at addresses in the indexed DOL. The descriptions below
are limited to observed instructions and caller data flow; no symbol names
have been assigned in Ghidra.

| Function | Evidence | Confidence |
|---|---|---|
| `0x80104274` | Takes a side argument; `lhz` at `0x80104280` reads `base+0x3C` when side is not 1, and `lhz` at `0x8010428C` reads `base+0x3E` when side is 1. Callers compare scores, record event data, and copy these values into another structure. | `verified` static read relationship |
| `0x80104294` | Takes side and value arguments; `sth` at `0x801042AC` writes `base+0x3C` for side other than 1, and `sth` at `0x801042A0` writes `base+0x3E` for side 1. For nonzero values it sets `base+0x40` to 1. Live breakpoint hits received side 1 and value 7 immediately before two separate 6→7 PAT score changes. | `verified` static write relationship and tested PAT execution |
| `0x801042C8` | Stores zero to the 32-bit field at `base+0x40`. Called by `0x8002676C` before its score calculation and by `0x80004E4C` after its score copy, among other callers. | `verified` static clear relationship; field meaning unknown |
| `0x80103624` | Initializes the pointed-to object and both score halfwords to zero on one startup path; another path copies halfwords from a separate structure at `+0x9E` and `+0x9A`. | `verified` static initialization relationship |
| `0x8002676C` | Obtains a side-specific value via `0x80053128`, adds an input amount, caps the unsigned result at `0xFF`, passes it to `0x80104294`, and also calls `0x80053180`. Live breakpoint hits received side 1 and amount 1 before two separate 6→7 PAT score changes; no-change runs received amount 0. A [native-buildable C candidate](../../projects/nfl-street/reconstruction/score_delta/README.md) preserves the static control flow. | `verified` static flow and tested PAT execution; C binary match absent |
| `0x80025ECC` | Reads byte and word fields at `+0x00`, `+0x18`, `+0x1C`, and `+0x1D` of a record; conditionally transforms the side and amount, calls `0x8002676C`, and may call `0x80102F0C`. The live score-delta entry had return address `0x80025F30`, consistent with its call at `0x80025F2C`. A [C candidate](../../projects/nfl-street/reconstruction/score_record/README.md) models the static branches. | `verified` static flow and tested caller relationship; `candidate` record semantics and C source |
| `0x8000AD8C` / `0x8000AEC8` | Copy both score halfwords to/from offsets `+0x598` and `+0x59A` in another structure. | `verified` static copy relationship; structure purpose not established |
| `0x80053128` / `0x80053180` | Select/update a side-specific named field in the `FNIG` table: `CSHG` for side 0, `CSAG` for side 1. In a successful PAT, the getter returned 6 and the setter received side 1, value 7. [C candidates](../../projects/nfl-street/reconstruction/score_table/README.md) use a native query interface for these fields. | `verified` static query relationship and live side-1 byte read/write below |
| `0x80004E4C` | At game-loop entry, selects `CSAG` and `CSHG` from `FNIG`, then passes those values to `0x80104294` for sides 1 and 0 respectively. | `verified` static copy relationship |

The 32-byte reader, 52-byte writer, and 16-byte clear function at
`0x80104274..0x801042D8` now have [separate PowerPC C variants](../../projects/nfl-street/reconstruction/README.md)
whose generated `.text` matches the hash-checked DOL byte for byte. This
verifies the machine-code reconstructions, not the original C syntax or
compiler. The score update function at `0x8002676C` executed during the
tested PAT transitions, but its C candidate remains without a binary match.

The query templates in the DOL at `0x802470D0` and `0x802470F0` select and
update a named field in `FNIG`; the four-byte field identifiers passed to
them are `CSHG` (`0x43534847`) and `CSAG` (`0x43534147`). A game-loop-entry
query at `0x80244358` selects both values and copies them into the two RAM
score fields. This establishes a second score representation at the named
table level. Later MEM1 captures locate `FNIG`-linked records and score-shaped
bytes within them; the side-1 query-engine read/write path is traced below.

The direct callers of `0x80104294` also include match/reset paths that write
zero, and a path that copies values from another structure. The
`0x8002676C` caller passes the same new value to both the RAM writer and the
`FNIG` update. The live read trace below verifies that this gameplay path
uses the `FNIG` side-1 byte as its starting score. It does not establish
`FNIG` as the sole source of truth in every game path.

The `base+0x40` word was zero at a visible 0–0 and one at supplied 6–0 and
7–0 states, but zero again in the separately captured 0–17 match. The writer
sets it to one for any nonzero *input* (before the halfword truncation); the
adjacent function clears it. It is not established as a persistent score or
"ever scored" field. Its broader interpretation remains `candidate`.

### Candidate copy of the match object

For this GNNE69 revision-0 DOL (SHA-1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`) in Dolphin 2603a, a
read-only scan of five 24 MiB MEM1 captures found exactly two aligned matches
to the first `0x30` bytes of the pointer-rooted match object in each capture.
The other match shared the score halfwords at `+0x3C`/`+0x3E` (big-endian
unsigned 16-bit) and the word at `+0x40` (big-endian 32-bit):

| Capture state | Pointer-rooted object | Other prefix match | Scores at both (+0x3C, +0x3E) | Differing bytes in first 0x200 |
|---|---:|---:|---:|---:|
| 0–0 | `0x803B6C20` | `0x804B26E4` | 0, 0 | 2 |
| 0–2 | `0x803B6C20` | `0x804B26E4` | 2, 0 | 9 |
| 0–6 | `0x803B6C20` | `0x804B26E4` | 6, 0 | 10 |
| 0–17 | `0x803B6C20` | `0x804B26E4` | 17, 0 | 36 |
| Separate 7–0 PAT run | `0x803B6C20` | `0x804C0F34` | 0, 7 | 6 |

The pointer chain for the first object is the verified slot `0x803A29C8`;
the pointer chain or owner of the second object is **unknown**. Its changed
address in the separate run is evidence against treating `0x804B26E4` as a
fixed address. The comparison is reproducible with
`projects/nfl-street/tools/find_score_copies.py` against the retained W:
captures. **Confidence: `probable`** that this is a second copy of the match
state; **candidate** that it is live game state. The timing of its updates,
its readers/writers, and whether it backs the `FNIG` table are unresolved.
The byte scan alone does not establish those relationships.
Exact aligned 32-bit references to the second match's base and score-field
addresses were absent from the five checked MEM1 captures. Interior pointers
were present in the separate PAT capture, so this scan does not establish
that the second object is unreachable or identify its owner.

### Earlier breakpoint attempt without debugger mode

On 2026-10-02, Dolphin 2603a restored the supplied Bears 6–Lions 0 PAT
state. Dolphin's GDB stub accepted an execution breakpoint at `0x8002676C`.
Automated controller-0 A presses moved from play selection to the field, but
the game remained before the snap and both scores stayed 6–0 for the 45-second
observation window. A repeat with later A presses snapped the play but did
not score. A third run held the stick forward after the snap: the RAM field
changed from Bears 6 to 7 and a completed frame showed Bears 7–Lions 0 on
the next play-selection screen. `0x8002676C` did not report a breakpoint hit
in that run. A repeat with a breakpoint at the lower-level writer
`0x80104294` ended with a failed conversion and stayed 6–0, so it did not
test a score write.

The GDB stub returned `OK` when setting execution breakpoints, but a control
breakpoint set on the currently interrupted PC also did not report a hit after
continuation. The 2026-10-04 control below found that these sessions lacked
Dolphin's `DebugModeEnabled` setting; the old negative breakpoint result does
not exclude this path. Completed frames are retained on W: as
`score-breakpoint-pat-last.png`, `score-breakpoint-pat-repeat-last.png`,
`score-breakpoint-pat-forward-last.png`, and
`score-breakpoint-writer-last.png`. The 6→7 run did not capture `FNIG` storage.

### Live score-path breakpoint verification

On 2026-10-04, fresh Dolphin 2603a sessions restored the same GNNE69
revision-0 Bears 6–Lions 0 PAT state. A control execution breakpoint at the
paused PC stopped correctly after the isolated Dolphin config set
`[Interface] DebugModeEnabled = True`. The GDB client was also corrected to
skip duplicate stop notifications and empty packets before reading a
register reply. The previous no-hit result was therefore a debugger setup
limitation, not evidence against the score path.

Execution breakpoints at `0x8002676C` and `0x80104294` then produced:

| Fresh run | Delta entry (`r3`, `r4`, LR) | Writer entry (`r3`, `r4`, LR) | Side-1 RAM result |
|---|---|---|---|
| Successful PAT 1 | `(1, 1, 0x80025F30)` at 61.89 s | `(1, 7, 0x800267AC)` at 62.66 s | 6→7 at 63.21 s |
| Successful PAT 2 | `(1, 1, 0x80025F30)` at 53.75 s | `(1, 7, 0x800267AC)` at 54.50 s | 6→7 at 55.04 s |
| Failed PAT 1 | `(1, 0, 0x80025F30)` at 38.68 s | `(1, 6, 0x800267AC)` at 39.40 s | Remained 6 through 90 s |
| Failed PAT 2 | `(1, 0, 0x80025F30)` at 45.90 s | `(1, 6, 0x800267AC)` at 46.43 s | Remained 6 through 90 s |

The successful runs stopped at the score-update entry with amount 1 before
the adjacent writer received value 7. The pointer slot continued to contain
`0x803B6C20`, and the big-endian side-1 halfword at `base+0x3E` changed
from 6 to 7 after continuation. The no-change runs provide controls for
amount 0 and writer value 6. A completed frame from the second successful
run shows the one-point conversion announcement. This verifies that the
researched score path executes during the tested PAT outcome, with the
record-handler and writer call sites indicated by LR. It does not establish
the exact instruction responsible for every score change or the `FNIG`
table's physical storage.

The successful logs are retained on W: as
`score-breakpoint-debugmode-replay.log` (SHA-256
`ed3b3251ffed39eed7ce407a0169fd47ed6963320e706b45daac52e9f687e517`)
and `score-breakpoint-debugmode-third.log` (SHA-256
`36c3134555e24b330e9fea03029a9ba913a6670e0f62fdbcc8ef42405da11cc7`).
The failed-run controls are `score-breakpoint-debugmode-repeat.log` and
`score-trace-tool-validation.jsonl`. A complete PNG from the second success
is `score-breakpoint-debugmode-success.png`; it stays outside Git.
The reproducible [trace tool](../../projects/nfl-street/tools/trace_score_path.py)
verifies the ISO SHA-1, replays the controller sequence, and records JSON
breakpoint events. It can yield either game outcome, as these runs show.

### Live named-score wrapper values

A separate pair of fresh PAT sessions and a run of the reusable trace tool
armed the `FNIG` score wrappers after the delta entry. Both successful
conversions reached
`0x80053170` immediately after the getter loaded its local result into `r3`;
that register contained **6** while RAM still held side-1 score 6. The RAM
writer then received **7**. At the setter entry `0x80053180`, registers were
`r3=1` (side) and `r4=7` (submitted value), and RAM already held 7. The
failed-conversion control returned 6 from the getter, sent 6 to both writer
and setter, and stayed at RAM score 6. These observations verify the live
wrapper value flow and its order relative to the RAM write. The paired MEM1
captures below establish a `FNIG` descriptor-to-record pointer and a
score-shaped update during this setter call. Direct dispatcher access and
the source-of-truth relationship remain unresolved.

The W: logs are `fnig-score-wrapper-trace-repeat.log` (successful, SHA-256
`ab0497e138dd3641e7cf3b479eb70d2a2889a6a506b63f2b1efc4cf0d938413f`)
and `fnig-score-wrapper-trace.log` (failed, SHA-256
`40c558819717d2f770946720667502ee4b4f8c6232b019ee5beba96b3b91ad01`).
`trace_score_path.py --trace-table` arms these wrapper breakpoints
conditionally after the delta entry; its successful JSONL validation is
`score-trace-table-success.jsonl` (SHA-256
`2329582c71495b69279e98576814dd4447a76fd7e2937f8634ca9a1405c52a84`).

### Score-shaped byte pairs in separate objects

In another successful 6→7 PAT replay, three paused 24 MiB MEM1 captures were
taken at the delta entry, at the named-table setter entry, and after the
setter returned. At the setter entry the RAM score had already become 7,
while the candidate named-score byte was still 6. After the setter returned,
that byte was 7. The adjacent byte stayed 0. The query buffer at
`0x80425630` also changed 6→7, but held a different query value before the
delta call; it is not evidence of persistent score storage.

The PAT pair is at `0x804A21D0..D1` (side 1, then side 0), within the
previously described `EMAG` object. Its bytes follow 0–0, 6–0, 7–0, and
7–6 across the supplied states. A similar byte pattern follows 0–0, 0–2,
0–6, and 0–17 in fresh-match captures at `0x804B3100..01`. The surrounding
record headers differ: the fresh-match record is not the relocated `EMAG`
object discussed above, whose score-shaped bytes stayed zero. Both pairs
have a direct pointer from a descriptor bearing the `FNIG` tag: in the
supplied PAT history, `0x804AC914` points to record `0x804A21C0` and `FNIG`
sits at `0x804AC91C`; in fresh matches, `0x804B5B34` points to
`0x804B30F0` and `FNIG` sits at `0x804B5B3C`. The pair begins at record
`+0x10` in both cases. The changed address makes a fixed absolute address
unsuitable for a runtime reader. The
[offline candidate scanner](../../projects/nfl-street/tools/find_named_score_candidate.py)
follows `FNIG` descriptor pointers and checks the surrounding pattern. It
found exactly one match in each of the eight earlier captures and three
paired PAT captures, with the expected scores. The pointer relationship,
score correlation, and the PAT pair's 6→7 update during the tested setter
call are **verified** observations. The side-1 `CSAG` read/write path is
verified below. The schema entries in both allocation histories assign an
8-bit `CSAG` field to bit offset 128 (record `+0x10`) and an 8-bit `CSHG`
field to bit offset 136 (record `+0x11`). Each entry is laid out as a
big-endian bit offset, four-byte field name, bit width, and type code; the
type code is `3` for both fields, with its meaning unresolved. The schema
pattern occurs once in each of the eleven tested captures and sits at
`FNIG` tag `+0x70`. This **verifies the named byte layout** in the retained
states. Direct side-0 query execution has not been captured.

The DOL BSS pointer slot `0x8039E4E8` supplies a bounded locator for the
tested states:

```text
node       = read_be_u32(0x8039E4E8)
descriptor = read_be_u32(node + 0x2C)
record     = read_be_u32(descriptor + 0x14)
assert read_bytes(descriptor + 0x1C, 4) == b"FNIG"
CSAG       = read_u8(record + 0x10)
CSHG       = read_u8(record + 0x11)
```

The slot lies within the DOL BSS range `0x802A05C0..0x803A4B6C`. In all
eleven captures, `node` was `0x80421B98`, while the descriptor and record
moved between the supplied and fresh-match histories. The relationship at
`node+0x2C` is an observed layout, not an established structure name. The
read-only NFL Street [observer](../../projects/nfl-street/nflstreet_core/observer.py)
checks the `FNIG` tag and score schema before using this path; it does not
hard-code the relocated descriptor or record address.

| Field in tested `FNIG`-linked record | Address in supplied PAT / fresh match | Type and size | Confidence |
|---|---|---|---|
| Descriptor's record pointer | `0x804AC914` / `0x804B5B34` | big-endian pointer, 4 bytes | `verified` in retained captures |
| BSS pointer slot to containing node | `0x8039E4E8` in both histories | big-endian pointer, 4 bytes | `verified` in retained captures |
| Side-1 `CSAG` score | record `+0x10`: `0x804A21D0` / `0x804B3100` | unsigned byte, 1 byte; endianness not applicable | `verified` for the tested side-1 query/update path |
| Side-0 `CSHG` score | record `+0x11`: `0x804A21D1` / `0x804B3101` | unsigned byte, 1 byte; endianness not applicable | `verified` schema and values across tested states; direct runtime access unobserved |

These heap addresses are observations, not fixed runtime addresses. The
game identity and Dolphin build are recorded at the top of this document;
the capture comparison, live read/write procedure, and related functions
are detailed immediately above and below.

### Direct `FNIG` score-byte read and store

On a no-score control and a separate successful PAT replay, an execution
breakpoint at `0x801B94F8` inside the query engine caught its 64-bit field
load. The DOL instructions are `lwz r4, 4(r3)` at that PC and
`lwz r3, 0(r3)` at `0x801B94FC`. At the breakpoint, `r3=0x804A21D0`, the
`FNIG` descriptor's record `+0x10`, and the eight bytes there began
`06 00 FF FF FF 00 7F 00`. The side-1 getter then returned 6. This
**verifies** the live read address for the tested `CSAG` query path.

Two more successful PAT replays stopped at `0x801B95E0` inside the query
engine after the named-score setter received side 1 and value 7. The DOL
instruction there is `stw r11, 0(r22)` (`91 76 00 00`). At the execution
breakpoint, `r22=0x804A21D0` exactly matched the `FNIG`-linked record's
score-shaped pair, and `r11=0x0700FFFF`. Before continuation, the four
bytes at that address were `06 00 FF FF`; afterward they were
`07 00 FF FF`. The RAM score had already changed 6→7. This **verifies** the
direct writer instruction, target address, and update order for this PAT
outcome. Together with the wrapper's `CSAG` query/update arguments and the
direct load above, the first byte's identification as the named side-1 score
is **verified** for this image and tested path. The side-0 `CSHG` byte's
schema location and observed scores are verified above; its direct runtime
access path remains unobserved. The address is specific to this allocation
history.

The reusable [trace tool](../../projects/nfl-street/tools/trace_score_path.py)
now supports `--trace-store`. Its successful JSONL is retained on W: as
`fnig-score-store-cli-success.jsonl` (SHA-256
`fefe490b154a5fb76c75ce76b2f88523236e318bb1522b1b9c2a8d1f0de54b11`).
The subsequent paired read/write CLI trace is
`fnig-score-readstore-cli-success.jsonl` (SHA-256
`f9ecec5d788e695eedd0140baa8269235a8007fa35a1ee3f32b391a7f27dc648`).
An independent no-score read control is
`fnig-score-read-breakpoint.log` (SHA-256
`03c68e93317d7efddea7280ba6535648dfd9a9a50f072a624c1142afdca30473`).
An independent execution-breakpoint prototype log is
`fnig-score-store-exec-breakpoint.log` (SHA-256
`8b71060cad73a9b67da02160923ff94f0ef7d6e7dd56c73a607afe9051a1df46`).
A write-watchpoint control stopped at the same PC but perturbed the target
bytes to zero; the execution-breakpoint result above is the nonperturbing
confirmation.

A passive 30-second replay from the supplied 7–6 play-selection state did
not call either score wrapper or the match-init copy function. Its first
generic query load was at record `+0x20`, unrelated to the score bytes.
The control is on W: as `fnig-side0-passive-control.log` (SHA-256
`08f69b73cb7eeb25bd3cad521e76b161de522a073357337f83c16040e63dded2`).

The pointer-rooted observer was also exercised against a fresh Dolphin
2603a restore of that 7–6 state. Four one-second read-only samples returned
7–6 from both the RAM score and named `FNIG` score with status `available`.
This validates the decoder against the live environment in addition to the
eleven retained MEM1 captures. Reads of the two representations are not
atomic, so a transient difference during an update is possible.

The [capture audit](../../projects/nfl-street/tools/audit_score_captures.py)
checked all 186 retained MEM1 images under the W: research directory. Both
score readers were available in every image and agreed in 185. The sole
difference was the intentionally paused `fnig-pat-before-setter.mem1` image:
RAM side 1 was already 7 while `FNIG` side 1 was still 6. The JSONL manifest
is `nflstreet-score-capture-audit.jsonl` on W: (SHA-256
`116ad6b9dc94045a8326659c432713f58520d3b3538934f41dbc2b567cf978f3`).
Those 186 images cover two observed descriptor/record allocation groups,
with 180 and 6 captures respectively; they do not prove the pointer path
for every game mode or revision.

The paired captures on W: are `fnig-pat-before-delta.mem1` (SHA-256
`54a787dd86692465172f0b39561e4113d4fd5c432fe5bd576d6ef5fb57bb1643`),
`fnig-pat-before-setter.mem1` (SHA-256
`73a8cd768fe5860d9d0d2d6d89bf8360e2bebcd38bf322f8b55a08029968ab1d`),
and `fnig-pat-after-setter.mem1` (SHA-256
`a4843940fc48f4412b17562c827f09278d1b18e6549035ffffac1d379e6c37a6`).
The breakpoint trace is `fnig-score-snapshot-trace-repeat.log` (SHA-256
`2bb39233da9f4285445a68e1116c9ce6ea1756a5e84df0a2a5bd9846dbc8c54d`).

### Read-only live transition and watchpoint control

On 2026-10-03, a fresh Dolphin 2603a reload of the supplied 6–0 PAT state
replayed the successful input sequence without a GDB connection. Reads every
approximately 0.2 seconds observed `read_be_u32(0x803A29C8) == 0x803B6C20`
through the conversion and Bears `read_be_u16(base+0x3E)` changing 6→7 at
52.249 seconds after the input sequence began. The resulting 24 MiB MEM1
capture has pointer `0x803B6C20` and score bytes `00 00 00 07` at
`base+0x3C..+0x3F`. The preceding saved frame shows the one-point conversion
announcement while the scoreboard still reads 6–0. This independently
reproduces the live score-field change; that read-only run alone did not
establish the writing PC.

A Dolphin GDB write watchpoint on `base+0x3C` for four bytes did not provide
a usable scoring trace. In one PAT run it reported a stop at `0x801D4858`
after the static pointer read as zero, while a later completed frame showed
7–0. A no-input control with GDB attached, interrupted, and resumed without
the watchpoint kept the pointer at `0x803B6C20` and score at 6 for 15 seconds.
The same control with only that watchpoint armed changed the pointer read to
zero within one second and kept it zero for the remaining 15-second sample.
Thus the null pointer is associated with arming this watchpoint in the tested
setup; neither that stop nor the null value identifies a normal gameplay
score writer or disproves the score pointer in uninstrumented play. An
independent watchpoint on a player-position field did stop on a position
write, so the limitation is specific to this score watchpoint or its setup.

## Confidence and limits

The static-pointer-rooted score-field interpretation is `verified` for this ISO and
Dolphin build: two teams changed independently, multiple score magnitudes
were reproduced, separate fresh savestate reloads and new matches agreed,
and non-score state changes served as controls. The observed pointer target
was stable across these runs but has not been shown to stay fixed in every
allocation history or game revision. The source of truth versus display
mirror question remains open.

## Next experiment

Trace the side-0 `FNIG` read/write path and determine whether the BSS-rooted
locator survives other game modes and allocation histories. Resolve why the
score-address write watchpoint invalidates the restored match pointer before
using it for an instruction-level writer trace. Trace the owner and update
path of the probable second match object.
A nonzero score from a clean ISO boot would further test the pointer root
under a different allocation history. The next NFL Street state field,
controlled ball-carrier position, is documented in
[player_position.md](player_position.md).
