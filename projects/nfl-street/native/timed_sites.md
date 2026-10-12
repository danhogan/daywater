# Native Windows timed-site profile

**Date:** 2026-10-04  
**Scope:** GNNE69 revision 0, ISO SHA-1
`813c4511ba3135059a13de424555647e8252254b`, saved first-period
Chiefs–Falcons match on Windows x64  
**Confidence:** `verified` measurement of the stated runs; `probable` that
the two leading PCs are useful CPU optimization targets for this match
segment; no claim about a full match or all game modes.

## Offline process

The opt-in [timed-site patch](moderngekko-timed-sites.patch) measures a
sparse, randomized sample around `m_module->dispatch` and aggregates
inclusive elapsed host nanoseconds by guest entry PC. Its interval varies
from 4,096 to 6,143 native dispatches. It does not time rendering, audio,
all runtime work outside that call, or every call to each PC. The patch was
applied to the local vendored Dolphin checkout, built as
`moderngekko-run-timed-sites.exe` beside its `Sys` resources on W:, and
reverted from source. The known-working normal `moderngekko-run.exe` was
restored byte for byte.

The [offline worker](profile_windows_native.py) runs paired normal and
timed trials from the same `match.sav`, alternating order between rounds.
Each trial is stopped after at least 600 observed frames. It requires a
restored frame number of at least 16,000, zero frame-counter resets, zero
interpreter fallback steps, zero failed code chunks, and a clean process
exit. It records runner, module, and state SHA-1 hashes, stdout logs, and
machine-readable JSON on W: at
`W:\Daywater\research-temp\nflstreet-timed-sites-600x3-2026-10-04`.
The profiler runner SHA-1 is
`28ecc69f06cbc326b4d217a4371b6fd61ae0e29e`; the normal runner
SHA-1 is `d1afca7d18e3691e12837888f872f8f3ba4b31ea`.
The module SHA-1 is `dd521cc38b0db6bca98a0bb261570a2be5e6e0ed` and the
savestate SHA-1 is `bfae118f2cce879dc4387d503a082a230a671d85`.

| Round | Order | Normal frames / FPS | Timed frames / FPS |
|---|---|---:|---:|
| 1 | normal, timed | 601 / 14.87 | 601 / 15.55 |
| 2 | timed, normal | 604 / 24.45 | 601 / 18.04 |
| 3 | normal, timed | 600 / 26.89 | 604 / 27.11 |

The whole-run FPS increased for both variants across the batch. Host
conditions therefore changed enough that these six runs do not establish a
profiler overhead ratio or a reliable uninstrumented speed. The two guest
PCs below held the top sampled-time ranks in all three timed trials:

| Guest entry PC | Samples per round | Sampled inclusive time per round | DOL operations |
|---|---:|---:|---|
| `0x80188BB0` | 795, 795, 801 | 739.3, 607.6, 438.6 µs | 29-instruction paired-single calculation with `psq_l`, multiply, madd, and sum |
| `0x801FFDF0` | 288, 288, 288 | 681.0, 579.3, 409.1 µs | Paired-single-heavy calculation reading two structures and writing one |

Those values are sums over sampled calls, not whole-run time at each PC.
`0x80188BB0` had more samples; `0x801FFDF0` cost more per sampled call.
Scaling the two sampled sums by `native_dispatches / timed_samples` gives
roughly 7.3, 6.1, and 4.3 seconds across the three timed trials, or about
18–20% of their respective wall times. This is an estimate from sparse
deterministic sampling, sensitive to timer overhead and host scheduling; it
is useful for choosing an experiment, not a verified whole-run cost.
The original DOL disassembly and generated C place them in chunks
`chunk_0098_text1_80187480.c` and `chunk_0128_text1_801FF480.c` on W:.
The generated C calls general paired-single helpers repeatedly. This is a
concrete lead for a targeted optimization, but a safe change must preserve
PowerPC numerical behavior, memory effects, and code-validation behavior.
The helpers in `GXRuntime/src/core/cpu_interpreter_float.c` use
`force_25bit_c`, `ni_mul`/`ni_madd_msub`, `force_single`, and FPSCR result
classification. The `psq_l` fast path in `GXRuntime/include/core/cpu.h`
also handles GQR type and LSQE checks. Any direct arithmetic replacement
needs comparison against these semantics, including exceptional float values.
The [curated source candidate](../reconstruction/odd_polynomial_pair/README.md)
for `0x80188BB0` derives its two-lane odd polynomial and matches the native
module's output words on 100,000 synthetic finite inputs using the GQR6
configuration from the game's initialization routine.
The [second candidate](../reconstruction/paired_product/README.md) models
`0x801FFDF0` as six paired outputs from a three-group product. Its 12 output
words also matched the native module bit for bit on 100,000 synthetic finite
calls. Subsequent guarded full-state comparisons and a live Windows probe
are documented in the [combined experiment](two_fastpaths_experiment.md).
Both candidates ran in a separate experimental DLL; the default Windows
bundle still uses the original generated instructions.
Two smaller sites in the same generated chunk were reconstructed later:
[`0x801FFEBC`](../reconstruction/weighted_pair_reduction/README.md) and
[`0x801FFF10`](../reconstruction/three_pair_projection/README.md). They
accounted for 123 and 109 samples in the third timed round, respectively.
Those sample counts guided selection; they do not establish performance
benefit from replacing either function.

## Reproduce

Apply the patch to the vendored Dolphin checkout and build a separate
Windows runner with `build_windows_runner.cmd`; keep the normal executable
backed up and restore it after saving the profiling build. The patch is
inactive without `STATICRECOMP_TIMED_SITES=1`. From Windows Python, run:

```bat
py -3 W:\Daywater\research-temp\profile_windows_native.py ^
  --output W:\Daywater\research-temp\nflstreet-timed-sites-next ^
  --frames 600 --repeats 3 --max-seconds 180
```

Use a new output directory. The worker and the bundle require the user's
locally owned game files; keep all generated code, ISO data, savestates,
screenshots, and RAM captures outside Git.
