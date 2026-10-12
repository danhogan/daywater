# Score delta at `0x8002676C`

**Status:** live PAT execution verified; semantic C candidate and unmatched
linked GCC probe. Target: NFL STREET GameCube `GNNE69`, North America
revision 0, DOL SHA-1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. The function occupies
124 bytes at DOL file offset `0x2376C`; `verify_score_targets.py` checks its
SHA-256 before comparison.

The original instructions call `0x801042C8` to clear the `+0x40` word, read a
side-specific named value through `0x80053128`, add the caller's amount as a
32-bit PowerPC operation, and cap the **unsigned** result at 255. They then
call the verified RAM score writer `0x80104294` with the low 16 bits and call
`0x80053180` with the full capped value. A nonzero amount also leads to calls
at `0x80109030` and `0x8010A384`. The roles and implementations of those
last two functions remain unknown.

`score_delta.c` preserves the observed control flow. Its `0x80053128` and
`0x80053180` callees now have [source candidates](../score_table/README.md).
The two remaining event calls have external service declarations with
placeholder C names and signatures. The offline test supplies fakes for
those event calls and a native in-memory `FNIG` store, checking 6+1, cap at
255, zero amount, zero result, call order, and
the unsigned wrap case `0 + (-1) -> 255` implied by `add` then `cmplwi`.
This does not establish the intent of a negative scoring amount in gameplay.

Fresh Dolphin 2603a breakpoint runs on the tested 6–0 PAT state stopped at
`0x8002676C` with `r3=1`, `r4=1` before two separate 6→7 score changes.
The following writer breakpoint received side 1 and value 7. Two failed
conversion controls reached the same path with amount 0 and writer value 6,
then stayed 6–0. The delta entry LR was `0x80025F30`, consistent with the
static record-handler call at `0x80025F2C`. The [score evidence](../../../../research/nfl_street/score.md)
records the procedure and W: logs. The replay is available as
[`trace_score_path.py`](../../tools/trace_score_path.py). This verifies the
tested PAT execution path, not the source candidate's PowerPC bytes or the
event-service meanings.

## PowerPC code-generation probe

`score_delta_gcc_probe.c` is an isolated PowerPC version of the semantic
candidate. `build_probe.sh` compiles it with GCC 15.2.1, assembles it with
Zig, and links its six calls to their researched GNNE69 addresses using
`score_delta_probe.ld`. The script first validates the source DOL SHA-1 and
124-byte target hash. The linked ELF is big-endian PowerPC, starts at
`0x8002676C`, and has no remaining relocations. Generated files stay on W:.

The result is **140 bytes versus the DOL's 124 bytes**, so it is not a byte
match. The high-level call order, unsigned comparison against 255, and
conditional event calls align. GCC emits individual `stw`/`lwz` instructions
for the three saved registers, whereas the DOL uses `stmw`/`lmw`. It also
orders some `mr` instructions differently. An empty assembly constraint in
the probe preserves the DOL's explicit halfword conversion before the score
writer; it adds no instruction itself. The compiler probe does not identify
the original compiler or source syntax.

```bash
bash projects/nfl-street/reconstruction/score_delta/build_probe.sh
```

The slice builds as a native Windows x64 console executable with Zig 0.16.0:

```bash
ZIG_GLOBAL_CACHE_DIR=/tmp/daywater-zig-cache \
  /mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig cc \
  -target x86_64-windows-gnu -std=c89 -O2 \
  projects/nfl-street/reconstruction/score_reader/score_reader.c \
  projects/nfl-street/reconstruction/score_writer/score_writer.c \
  projects/nfl-street/reconstruction/score_writer/clear_unknown_40.c \
  projects/nfl-street/reconstruction/score_delta/score_delta.c \
  projects/nfl-street/reconstruction/score_record/score_record.c \
  projects/nfl-street/reconstruction/score_table/score_table.c \
  projects/nfl-street/native/fnig_store.c \
  projects/nfl-street/reconstruction/tests/test_score_delta.c \
  -o /mnt/w/Daywater/research-temp/nflstreet-score-slice-check.exe
```

This is a test executable with fake storage and event services, **not a
playable game**. The original DOL is not linked into it; no game assets or
captures are included. The native game still requires reconstruction of the
surrounding game systems and their platform interfaces.
