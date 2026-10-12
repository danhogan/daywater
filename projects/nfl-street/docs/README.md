# NFL Street read-only observer

`nflstreet_core` decodes verified fields from the tested GameCube executable
(`GNNE69`, revision 0, DOL SHA-1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`). Its only input is a
`read_bytes(address, size)` method. It does not write game memory or choose
controller actions. Daywater remains generic; all game-specific addresses are
inside this project.

Each observation contains the left and right displayed scores, the separate
`FNIG` named scores, the actor
assigned to controller slot 0, that actor's roster name, and its ground-plane
position. `score_status` and `selection_status` explain absent values: for
example, `pointer_unavailable`, `no_selection`, or `pointer_changed`.
The named score follows a BSS pointer through a tagged `FNIG` descriptor
and checks its field schema. `named_score_status` reports an unavailable
pointer, tag/schema mismatch, or a pointer change. The numeric `team_index`
is the game's observed roster index, not a team name.
The actor's Z field is a ground-plane coordinate in tested states; it has not
been verified as jump height. Both score representations and player selection can change between
the separate memory reads, so the observer rereads pointer roots and returns
`pointer_changed` if it detects a transition. It is not an atomic frame
snapshot.

Run on an existing 24 MiB MEM1 capture:

```bash
python projects/nfl-street/tools/observe.py --image /path/to/capture.mem1
```

Run against a live Daywater Dolphin session in the container:

```bash
uv run python projects/nfl-street/tools/observe.py \
  --iso /path/to/game.iso --savestate /path/to/state.sav \
  --dolphin-binary /path/to/compatible/dolphin-emu \
  --duration 20 --interval 1
```

The CLI starts a temporary Dolphin session, prints one JSON observation per
interval, and closes the session. The 2603a savestates used in the research
require a compatible Dolphin build. The CLI checks the in-memory game ID but
does not verify the ISO or DOL hash itself; use the documented hash when
applying these field meanings to a new file.

## Validation

Six offline decoder tests cover a full observation, unavailable score
pointer, no selected actor, bad roster pointer, named-score schema mismatch,
a mid-update score difference, and wrong game ID. Retained
MEM1 captures decode 6–0 with no selected actor at PAT play selection and
7–0 with Kreutz or Thomas selected in later states. The named-score locator
matched all eleven original score captures. A broader
[audit](../tools/audit_score_captures.py) checked 186 MEM1 images: both
representations were readable throughout, and 185 agreed. The only
difference is the capture paused between the RAM write and `FNIG` update.
A live Dolphin 2603a restore of
the supplied 7–6 state returned 7–6 from both score representations in four
consecutive one-second samples.

An integration run on 2026-10-03 used Dolphin 2603a, restored the supplied
0–0 first-down state, pressed `A`, then held the main stick left. Across 23
samples, both scores stayed 0; the selected actor changed from Stewart
(team 1, index 0) to Thomas (team 1, index 1) at about six seconds. Thomas's
X moved from about `-0.995` at that transition to `-13.390` by 22.6 seconds,
while Y settled near `-35.285`. The JSONL trace is on W: at
`W:\Daywater\research-temp\observer-play-2026-10-03.jsonl`.

The source experiments and confidence limits are in
`research/nfl_street/score.md`, `selected_player.md`, and
`player_position.md`. No raw game image, asset, savestate, or RAM capture is
stored in this project.

## Thomas X writer replay

`tools/trace_thomas_position_writer.py` traces direct PowerPC stores to
Thomas's X or Y field during the supplied first-down play. It uses an execution
breakpoint and steps the instruction after confirming the store target.
It was validated with Dolphin 2603a and the researched GNNE69 image:

```bash
python projects/nfl-street/tools/trace_thomas_position_writer.py \
  --iso /path/to/verified-GNNE69.iso \
  --savestate /path/to/first-down-0-0.sav \
  --dolphin-binary /path/to/dolphin-emu-2603a \
  --snap --stick-x 0 \
  --output /mnt/w/Daywater/research-temp/thomas-writer-trace.jsonl
```

Use `--axis y --stick-x 0.5 --stick-y 0` for the first-down Y store.
Two fresh Y replays reached Thomas's Y address and changed that field on the
single step. The `--axis x` default retains the original X trace behavior.

The tool reports `matched=false` if the tested instruction never targets
Thomas during the trace. In a second-down control, Thomas moved but that
instruction targeted another actor; this writer finding is bounded to the
tested first-down path.

`tools/trace_thomas_position_add.py` checks a second-down path. It filters
calls at `0x80036E08` by Thomas's X address, then single-steps the actual
X store at `0x801DE590`. Add `--trace-vector` to step the adjacent Y and Z
stores too:

```bash
python projects/nfl-street/tools/trace_thomas_position_add.py \
  --iso /path/to/verified-GNNE69.iso \
  --savestate /path/to/second-down-midplay.sav \
  --dolphin-binary /path/to/dolphin-emu-2603a \
  --stick-x 1 \
  --output /mnt/w/Daywater/research-temp/thomas-second-down-trace.jsonl
```

Fresh right and left input replays both reached Thomas's X store; the
single-step changes had opposite signs. Fresh forward and backward replays
with `--stick-x 0.5 --stick-y 0` and `--stick-x 0.5 --stick-y 1`, respectively,
confirmed opposite signed Y changes at `0x801DE5A0`. The adjacent Z store
executed but left Z at zero in these samples. The addresses, tests, limits,
and W: trace hashes are in `research/nfl_street/player_position.md`.

For a larger reconstruction target, `tools/capture_thomas_kinematics.py`
captures the full `0x54`-byte input and output block of Thomas's second-down
position/velocity update at `0x80036B28`. Six controlled runs and the bounded
static call graph are documented in the
[kinematics boundary](../reconstruction/kinematics_boundary.md). The raw
blocks remain on W:. A guarded
[native Windows replay](../reconstruction/kinematics/README.md) reproduces all six full blocks.

## Score path breakpoint replay

`tools/trace_score_path.py` replays the researched 6–0 PAT savestate under a
compatible Dolphin build. It first verifies the GNNE69 ISO SHA-1, then arms
execution breakpoints on the score delta function and RAM score writer. It
prints JSON Lines containing function arguments, return addresses, and the
score transition. Save generated output on W: or another location outside
Git:

```bash
python projects/nfl-street/tools/trace_score_path.py \
  --iso /path/to/verified-GNNE69.iso \
  --savestate /path/to/6-0-PAT.sav \
  --dolphin-binary /path/to/dolphin-emu-2603a \
  --trace-table \
  --trace-store \
  --output /mnt/w/Daywater/research-temp/score-trace.jsonl \
  --frame-output /mnt/w/Daywater/research-temp/score-trace.png
```

Add `--capture-record` to capture the 32-byte handler input at `0x80025ECC`
for the independent [Windows score-event replay](../native/score_event_replay.md).
Keep that JSONL file private on W: because it contains game memory bytes.

Two fresh runs observed side 1, amount 1 at the delta entry, then side 1,
value 7 at the writer, followed by RAM score 6→7. Two failed conversions
entered with amount 0 and writer value 6; the score stayed 6. The tool can
produce either gameplay outcome. With `--trace-table`, a successful run also
observed the named score getter return 6 and the setter receive 7. Paired
MEM1 captures found a score-shaped byte pair that changed 6→7 while the
setter executed. A `FNIG`-tagged descriptor points to its containing record
in both supplied and fresh-match captures. `tools/find_named_score_candidate.py`
follows that pointer and checks the named-field schema. The schema places
`CSAG` and `CSHG` in the two adjacent bytes; which query-engine instructions
access the side-0 byte remains unresolved. A successful live
`--trace-store` run verified the `0x801B94F8` load from the side-1 byte and
the `0x801B95E0` store changing it 6→7 after the RAM score changed. The
evidence and hashes are recorded in
`research/nfl_street/score.md`. Breakpoints require Dolphin debugger mode;
Daywater now enables it for sessions opened with `gdb_port`.

## Decompilation path

The observer turns verified structure fields into runtime checks for Ghidra
analysis. A proposed C structure or function can be compared against the
observed score, roster selection, and position transitions. This helps assign
meaning and test hypotheses; matching source code to the original PowerPC
binary is a separate reconstruction task under `reconstruction/`.
