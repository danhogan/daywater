# Local decompilation toolchain status

Checked 2026-10-03 for the verified GNNE69 revision-0 DOL SHA-1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`.

The official [decomp-toolkit](https://github.com/encounter/decomp-toolkit)
v1.8.4 Linux x86-64 release is retained outside the repository at
`W:\Daywater\tools\dtk-v1.8.4-linux-x86_64`. Its SHA-256 is
`7c86b1994401bde0cad1d308e6c04cccb65e85453ccc5062ac12baab3b03d320`,
matching the release metadata. It reports version `1.8.4` at commit
`a0c455e46cab58e1d2e0885623f85089ff0499db`.

`dtk dol config` successfully generated a minimal configuration from the
local DOL and recorded the correct SHA-1. Whole-DOL analysis is not ready:

- `dtk dol info` stopped with an overlapping-function error beginning at
  `0x8016B73C` and reaching `0x801EF644`.
- `dtk dol split` was tried with a narrow skip around that region, then a
  broad `0x80160000..0x801EF644` skip with one worker. Both processes ended
  with exit code 137 before producing symbols, splits, or objects. The cause
  of termination has not been established. These attempts were temporary;
  no generated files or game code were committed.

The existing Ghidra cache has bounded disassembly for the three small score
functions. The checked DOL offsets and hashes in
`verify_score_targets.py` give a reproducible target while full-DOL
splitting remains unresolved.

The official [Zig](https://ziglang.org/download/) 0.16.0 Linux x86-64
release is retained at `W:\Daywater\tools\zig-x86_64-linux-0.16.0`. Its
download SHA-256 is
`70e49664a74374b48b51e6f3fdfbf437f6395d42509050588bd49abe52ba3d00`,
matching the official release index. `zig cc` uses Clang 21.1.0 and emitted
32-bit big-endian PowerPC assembly for all three C candidates using:

```bash
ZIG_GLOBAL_CACHE_DIR=/tmp/daywater-zig-cache \
  /mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig cc \
  -target powerpc-freestanding -mcpu=750 -std=c89 -O2 -g0 \
  -fomit-frame-pointer -S source.c -o output.s
```

Generated `.s` files are retained at
`W:\Daywater\research-temp\daywater-score-{reader,writer,clear}-zig-opt.s`.
An assembled reader object was also checked with `readelf`: it is ELF32,
big-endian PowerPC. Its global-pointer references use
`R_PPC_ADDR16_HA`/`R_PPC_ADDR16_LO` relocations, consistent with the
absolute-address load visible in Clang's assembly.
The initial, unconstrained C comparison is useful but **not a binary match**:

| Function | Original DOL | Zig/Clang PowerPC | Main difference |
|---|---:|---:|---|
| Reader `0x80104274` | 8 instructions | 8 instructions | Original branches to two separate `lhz` loads and uses `r13` small-data access; Clang selects offset 60/62 and uses one `lhzx` with an absolute-address global load. |
| Writer `0x80104294` | 13 instructions | 12 instructions | Both have side-specific `sth` paths and conditional early return; Clang hoists the pointer load before the branch and uses an absolute-address global load. |
| Clear `0x801042C8` | 4 instructions | 5 instructions | Original loads the pointer from `r13-0x7078`; Clang uses `lis`/`lwz` for the placeholder global. |

The original uses `cmpwi`; Clang uses `cmplwi` for the equality checks.
Those comparisons have the same equality result, but they encode different
instructions. The generated code confirms the candidate's broad behavior
and identifies exact code-generation differences. It does not identify the
original compiler version.

## Byte-matching score functions

The four-instruction clear function at `0x801042C8` can be reproduced with
the local Zig/Clang 21.1.0 PowerPC compiler. The separate
`score_writer/clear_unknown_40_clang_match.c` derives the pointer slot from
the verified `r13-0x7078` relationship and constrains the compiler to use
`r9` for the pointer and `r0` for zero. An empty inline assembly constraint
supplies **no instruction**; it only preserves those registers. The compiled
sequence is `lwz r9,-0x7078(r13)`, `li r0,0`, `stw r0,0x40(r9)`, `blr`.

`build_clear_match.sh` compiled an ELF32 big-endian PowerPC object with a
16-byte `.text` section and no `.text` relocations, extracted that section to
`W:\Daywater\research-temp\nflstreet-clear-match`, and ran the hash-checked
DOL comparator. The raw `.text` SHA-256 was
`1285789de99233791d6b210a671239e912c303d856eb089e13eae8e64f6c3415`,
identical to the 16 target bytes. This is a **verified byte match** for that
function under the recorded Clang build. It does not establish the original
compiler or original C syntax. The
portable `clear_unknown_40.c` remains the host-test implementation.

The [Fedora PowerPC GCC package](https://packages.fedoraproject.org/pkgs/cross-gcc/gcc-powerpc64-linux-gnu/)
15.2.1 was downloaded to W: without system installation. `rpm -K` reported
valid digests and signatures; the RPM SHA-256 is
`511cc42f13d961fdafd2592a5893ffac49cea262e1f5abd5897bd891d0976a6b`.
Its `cc1` accepted `-m32` and produced PowerPC assembly for the separate
`score_reader/score_reader_gcc_match.c`. Empty assembly constraints preserve
the observed two-path control flow and registers without adding instructions.
The source returns a 32-bit zero-extended value, consistent with `lhz`; the
original C return type is unknown. GCC's `-mno-always-hint` was necessary:
without it, the branch had a different prediction-hint byte.

`build_score_matches.sh` runs the clear check and then compiles the reader
with `-m32 -mcpu=750 -mno-always-hint -ffreestanding -nostdinc -O2
-fomit-frame-pointer`. Zig assembles GCC's output; the ELF32 big-endian
PowerPC `.text` has 32 bytes and no `.text` relocations. The hash-checked DOL
comparator reported a **byte-for-byte match**, with raw `.text` SHA-256
`31ee22d7af9b9955dab8b962c614050e9d0333d84ca1808531a7aaa52806afe8`.
Artifacts stay at `/mnt/w/Daywater/research-temp/nflstreet-score-matches`.
This verifies a reconstruction of the reader's machine code, not the game's
original source or compiler. The portable `score_reader.c` remains the
host-test implementation.

The same script compiles `score_writer/score_writer_gcc_match.c` with GCC
15.2.1 and additional `-fno-reorder-blocks`,
`-fno-reorder-blocks-and-partition`, `-fno-if-conversion`,
`-fno-schedule-insns`, and `-fno-schedule-insns2` options. Its relocation-free
52-byte `.text` matches the writer at `0x80104294` byte for byte. The raw
SHA-256 is `fdea0f82363ab97caa73b3abd6c03cb672a583f12cea6b4c5ada8534cb4aaeaf`.
These options preserve the observed branch layout; they do not establish the
original compiler or flags. The portable `score_writer.c` remains the host
test implementation.

## Compiler provenance lead from another EA project

The [Need for Speed: Underground GameCube decompilation](https://github.com/dbalatoni13/nfsug)
is a useful contemporary EA example. Its
[build configuration](https://github.com/dbalatoni13/nfsug/blob/main/configure.py)
uses **ProDG/3.9.3** for game code and SDK objects, with game flags including
`-O2`, `-mps-nodf`, and `-mfast-cast`. This establishes a compiler and build
workflow worth investigating; it does **not** establish NFL Street's compiler.
The NFS project currently targets a September 2003 prototype of a different
game and development team.

The hashed NFL Street DOL itself contains four exact `Please See the ProDG
manual` strings. All are in DOL data section 8, immediately after library
error messages for external calls to `malloc`, `free`, `realloc`, and
`calloc`, respectively:

| DOL file offset | Loaded address |
|---:|---:|
| `0x265F19` | `0x80268F19` |
| `0x265F97` | `0x80268F97` |
| `0x266016` | `0x80269016` |
| `0x266095` | `0x80269095` |

**Verified:** these bytes and locations are present in this DOL. **Candidate:**
the score functions were compiled by ProDG. The surrounding messages point
to a linked library, and a game can contain objects built with more than one
toolchain. No compiler attribution has yet been made for `0x80104274`,
`0x80104294`, or `0x801042C8`.

The public [decomp.me compiler release](https://github.com/decompme/compilers/releases)
lists ProDG v3.9.3 as GCC 2.95.3 SN build v1.76 for GameCube. Its 7.5 MB
archive is retained at `W:\Daywater\tools\ProDG-v3.9.3.tar.gz` (SHA-256
`bfe968dec5fd75a1340d21e5578ab8df124569be774db0cfd8e164e84af72338`),
extracted alongside it. [Wibo](https://github.com/decompals/wibo) v1.2.0
is retained at `W:\Daywater\tools\wibo-1.2.0-x86_64` (SHA-256
`13f86a2d618f0dbe67179d349625345eabf9b46450295cb4c904e49f6aff85af`).
Running `ngccc.exe --version` through Wibo stops with:

> NGCCC Error : SN Systems NGCCC requires a ProDG tools user license.

Therefore no ProDG candidate was compiled. The license check must be met by
an authorized installation; the downloaded package alone is insufficient.
The tools and archive stay outside Git, as do all game binaries and generated
objects.

No GameCube CodeWarrior compiler or objdiff binary was found in the checked
local tool locations. Decomp-toolkit analyzes and splits binaries; it does
not supply the original compiler. The larger score functions still need
matching compiler output or an explicitly constrained equivalent, with
correct SDA placement and linked instruction comparison.
Investigate callers and neighboring functions for compiler fingerprints, or
test an authorized ProDG installation if one becomes available.
