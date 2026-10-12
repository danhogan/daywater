# Native replay of real PAT score events

**Game:** NFL Street GNNE69 revision 0, ISO SHA-1
`813c4511ba3135059a13de424555647e8252254b`  
**DOL:** SHA-1 `46e3de854998a0f66c9be8c72a1164d22cd29eb4`  
**Scope:** one score-handler call, not a playable game or a binary-matching
decompilation.

The [trace tool](../tools/trace_score_path.py) now accepts
`--capture-record`. It stops at handler `0x80025ECC`, reads its 32-byte
record from the observed `r3` pointer, and logs `r4` control and score
fields. It also stops at the score delta and RAM writer. The raw record is
kept in a private JSONL trace on W:, never in the repository.

The [Windows replay program](score_event_replay.c) reads that record at run
time and passes it through the curated score-record, delta, table, reader,
and writer C candidates. An in-memory FNIG adapter supplies the named score
field boundary. Build it with
[`build_windows_score_replay.sh`](../reconstruction/build_windows_score_replay.sh).
[`verify_score_event_replay.py`](../tools/verify_score_event_replay.py)
compares its output with the Dolphin trace's final RAM score and original
writer argument without printing the record bytes.

Two fresh Dolphin 2603a PAT replays from the same 6–0 savestate produced
different outcomes:

| Captured handler input | Original game | Windows C replay | Confidence |
|---|---|---|---|
| Side 1, original amount 0, control 1 | Delta amount 0; writer value 6; final 6–0 | Final 6–0, zero update signals | `verified` for this captured call |
| Side 1, original amount 2, flags `0x20CC`, control 1 | Delta amount 1; writer value 7; final 7–0 | Final 7–0, one update signal | `verified` for this captured call |

For the successful case, the handler's `+2` branch becomes `+1` because
flag `0x4` is set and flag `0x80000` is clear. This interpretation follows
the checked DOL instructions and the observed delta argument. The two
replays agree on the RAM score; the native adapter's named score output was
not compared with a named-score read in these exact runs. Separate named
score traces are documented in `research/nfl_street/score.md`.

Private evidence on `W:\Daywater\research-temp`:

| File | SHA-256 |
|---|---|
| `score-record-live.jsonl` (failed PAT) | `99b50b1ab32f3dfd63cd790ac30e91397874e6c1a4bcee0fd3bc3431343d8e97` |
| `score-record-live-second.jsonl` (successful PAT) | `d45eb9ada6e04be1c1b844d1650e71ff1ff40ce5d3e4caf3d0b1b2c8a207d767` |
| `nflstreet-score-event-replay.exe` | `2c839ab6d09c51e5ceba0915771691050487f974cb2a7203e33524e7f7392e83` |

The executable rejects the handler's `-2`/control-zero path because its
follow-up service is not modeled. Its event-update and signal services are
test counters, and its FNIG implementation is a native adapter rather than
a reconstruction of the game's database engine. The score C functions
remain semantic candidates; these two input/output replays do not prove
equivalence for all records or a PowerPC binary match.

The running Daywater web container had an older debugger and session
module. For this replay only, the current project files were staged into
the container to enable execution breakpoints and background pipe input.
The original container files were restored afterward, and their SHA-256
hashes matched the saved copies.
