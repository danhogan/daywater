# Score reader at `0x80104274`

**Status:** portable C candidate unmatched; constrained PowerPC variant byte-matched.  
**Target:** NFL STREET GameCube `GNNE69`, North America revision 0, DOL SHA-1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`.  
**Evidence confidence:** `verified` for the static reads, observed RAM fields,
and compiled variant's bytes; `candidate` for original source syntax and
hypothetical symbol names.

The function is 32 bytes in text section 1, DOL file offset `0x101274`.
Ghidra's disassembly and direct extraction from the hashed DOL agree on the
following eight instructions:

| Address | Instruction | Observed effect |
|---|---|---|
| `0x80104274` | `cmpwi r3, 1` | Compare incoming side argument with 1. |
| `0x80104278` | `beq 0x80104288` | Choose the side-1 path. |
| `0x8010427C` | `lwz r9, -0x7078(r13)` | Load the match object pointer. |
| `0x80104280` | `lhz r3, 0x3C(r9)` | Return the side-0/right score as zero-extended u16. |
| `0x80104284` | `blr` | Return. |
| `0x80104288` | `lwz r9, -0x7078(r13)` | Load the same pointer on the side-1 path. |
| `0x8010428C` | `lhz r3, 0x3E(r9)` | Return the side-1/left score as zero-extended u16. |
| `0x80104290` | `blr` | Return. |

The DOL initializes `r13` to `0x803A9A40`; subtracting `0x7078` reaches
the verified pointer slot `0x803A29C8`. The object's score offsets and
left/right interpretation are independently verified by live memory and
visible scores in `research/nfl_street/score.md`. The function has no null
check, and neither does the C candidate. Equality with 1 is observed;
the original source's parameter name, signedness, and global symbol name
are unknown.

`score_reader.c` expresses this behavior using a placeholder global pointer
and the checked layout in `score_fields.h`. The source is C89-compatible.
`verify_score_targets.py` validates a supplied DOL by SHA-1 and extracts this
function without storing game code in the repository:

```bash
python projects/nfl-street/reconstruction/verify_score_targets.py \
  cache/extracted/<iso-sha1>/boot.dol reader
gcc -std=c89 -Wall -Wextra -Werror -fsyntax-only \
  projects/nfl-street/reconstruction/score_reader/score_reader.c
```

## Assembly comparison

The target has two separate `lwz` instructions, one in each branch, and
returns immediately after each `lhz`. A matching compiler output must retain
that branch shape, use `r3` for the argument and return value, access the
global pointer at `r13-0x7078`, load at offsets `0x3C` and `0x3E`, and
produce the same 32 bytes after linking. The comparator accepts a raw,
relocated candidate function through `--candidate-raw` once one exists.

The original portable `score_reader.c` compiled with Zig/Clang 21.1.0 to a
PowerPC candidate with eight instructions. It
uses one indexed `lhzx` after choosing offset 60 or 62, while the original
branches to two separate `lhz` instructions. It also loads the placeholder
global by absolute address instead of `r13-0x7078`; its `cmplwi` differs from
the original `cmpwi`. See [toolchain.md](../toolchain.md) for the command and
retained assembly.

## Byte-matching PowerPC variant

`score_reader_gcc_match.c` uses the verified `r13-0x7078` pointer slot and
empty assembly constraints to keep two distinct loads in `r9` and return a
zero-extended halfword in `r3`. Its `ScoreU32` return type reflects that
machine result; the original source type is unknown. The empty constraints
add no instructions. GCC 15.2.1 with `-m32 -mcpu=750 -mno-always-hint -O2`
emits the eight instructions above. The no-hint flag matters: without it GCC
emits `beq-`, which differs from the target by one byte.

`build_score_matches.sh` compiles this PowerPC-only source, assembles it with
Zig's PowerPC assembler, extracts the relocation-free 32-byte `.text`, and
compares it with the hash-checked DOL. The comparison passed; the raw bytes
have SHA-256
`31ee22d7af9b9955dab8b962c614050e9d0333d84ca1808531a7aaa52806afe8`.
This verifies a **byte match** under the recorded build, not the identity of
the game's original compiler or source syntax. The portable `score_reader.c`
remains the host-test version.
