# Guarded native fast-path experiment

**Date:** 2026-10-04  
**Game:** NFL Street, GNNE69 revision 0, ISO SHA-1
`813c4511ba3135059a13de424555647e8252254b`  
**Confidence:** `verified` for the stated synthetic full-state comparisons,
saved-match execution, and paired timing trials. Wider game-state and
long-match equivalence remain open.

The [reviewed candidate](../reconstruction/odd_polynomial_pair/README.md)
for `0x80188BB0` replaces the numerical core and reproduces the function's
register, FPSCR, stack, memory-write, PC, and cycle effects within guarded
conditions. Its Linux differential checker found 99,942 complete CPU-state
and touched-RAM matches, zero mismatches, and 58 guarded skips out of
100,000 synthetic trials. Half the trials used FPSCR `0xA2308104`, which a
short [live probe](live_pair_probe.md) observed in the saved match.

[`stage_odd_fastpath.py`](stage_odd_fastpath.py) inserted this candidate
only at the generated label for `0x80188BB0`. Its false result falls back
to the original generated instruction sequence. The experimental Windows
DLL with the final, stricter normal-intermediate guard was saved on W: as
`W:\Daywater\research-temp\gGNNE69-recomp-odd-fastpath-strict.dll`, SHA-1
`d3d8c0ce31203f1358e85a12b1365698444132d6`. A 120-frame saved-match
smoke run of the preceding version logged a fast-path hit, zero interpreter
fallback steps, and zero failed code chunks. Every strict candidate benchmark
run also logged a hit and passed the replay checks.

The [paired worker](benchmark_windows_modules.py) ran three baseline and
three candidate trials from the same `match.sav`, alternating order. Each
observed at least 600 emulated frames with the same known-working runner,
null graphics, zero fallback steps, zero failed chunks, and a clean exit.
All candidate logs included a fast-path hit. Full logs and `summary.json`
for the strict DLL are at
`W:\Daywater\research-temp\nflstreet-odd-fastpath-strict-600x3`.

| Round | Order | Baseline FPS | Candidate FPS |
|---|---|---:|---:|
| 1 | baseline, candidate | 27.10 | 28.86 |
| 2 | candidate, baseline | 28.01 | 28.82 |
| 3 | baseline, candidate | 28.15 | 28.77 |

The median was **28.01 FPS baseline** and **28.82 FPS candidate**, a 2.9%
increase in this fixed-frame, headless saved-match workload. The preceding
guard version, retained as `gGNNE69-recomp-odd-fastpath.dll` on W:, measured
28.01 versus 29.60 median FPS in a separate three-pair batch. The stricter
guard is the source retained in Git. These runs show a modest local speed
improvement, with host and gameplay variance still possible.
It does not imply 60 FPS windowed play or prove whole-match correctness.

A separate 122-frame windowed Vulkan run of the strict DLL restored the
saved match, logged a fast-path hit, and exited cleanly with zero interpreter
fallback steps and zero failed chunks. Its captured frame visibly shows the
Chiefs–Falcons lineup and score overlay. The private screenshot is
`W:\Daywater\research-temp\nflstreet-moderngekko-win-vulkan-automation\artifacts\odd-fastpath-strict.png`,
SHA-256 `f9c5fd6a675a96886e11bd0f2a41db777c99137a262c85df4566c7c6e0740bb2`.
This confirms rendering in the tested state; manual controls and a full match
were not exercised with this DLL.

After the trial, the generated source injection was removed and the original
module restored byte for byte in the normal W: build directory and the
bundled launcher (SHA-1 `dd521cc38b0db6bca98a0bb261570a2be5e6e0ed`).
The normal runner remained at SHA-1
`d1afca7d18e3691e12837888f872f8f3ba4b31ea`.
The experimental DLL and the candidate source remain available for further
validation; neither is the default launcher module.
