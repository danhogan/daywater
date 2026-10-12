# Combined native function experiment

**Date:** 2026-10-04  
**Game:** NFL Street, GNNE69 revision 0, ISO SHA-1
`813c4511ba3135059a13de424555647e8252254b`  
**Confidence:** `verified` for synthetic full-state comparisons and the
reported Windows trials; full-match equivalence remains open.

Two reviewed functions now have guarded CPU-state implementations:

| Entry PC | Algebraic role | Synthetic full-state matches / trials | Guarded skips |
|---|---|---:|---:|
| `0x80188BB0` | Two evaluations of an odd polynomial | 99,942 / 100,000 | 58 |
| `0x801FFDF0` | Six paired product outputs | 100,000 / 100,000 | 0 |

Each comparison used the existing Linux whole-DOL module as reference and
checked the entire CPUState and touched guest RAM. The original generated
instructions remain the fallback when a candidate's guard declines a call.
The tests include the FPSCR mode observed in the saved match, but do not
cover all game states, exceptional floats, or every memory alias.

The opt-in [polynomial staging script](stage_odd_fastpath.py) and
[product staging script](stage_product_fastpath.py) inserted both candidates
into separate generated chunks on W:. The resulting DLL was saved as
`W:\Daywater\research-temp\gGNNE69-recomp-two-fastpaths.dll`, SHA-1
`1c130b779784d8c355f20e92e93b6e976c0eadb9`. A 120-frame saved-match
smoke run logged a hit for each function, zero interpreter fallback steps,
and zero failed code chunks.

The [paired benchmark worker](benchmark_windows_modules.py) then ran three
baseline and three combined-candidate trials from the same savestate,
alternating order. All candidate logs show hits for **both** functions.
Each run reached at least 600 observed frames, exited cleanly, and reported
zero fallback steps and failed chunks. Logs and `summary.json` are at
`W:\Daywater\research-temp\nflstreet-two-fastpaths-600x3`.

| Round | Order | Baseline FPS | Combined FPS |
|---|---|---:|---:|
| 1 | baseline, combined | 28.29 | 29.02 |
| 2 | combined, baseline | 28.15 | 28.95 |
| 3 | baseline, combined | 28.09 | 29.11 |

Median FPS was **28.15 baseline** and **29.02 combined**, a 3.1% increase
in this fixed-frame, headless workload. The [single-function trial]
(odd_fastpath_experiment.md) measured 2.9% for the stricter polynomial
candidate in a separate batch. The extra gain from combining them is small
relative to host variation, so these data do not isolate the product
candidate's independent speed effect.

A separate 124-frame windowed Vulkan run with the combined DLL restored and
rendered the saved on-field match, logged hits for both functions, and exited
cleanly with zero fallback steps and failed chunks. Its private screenshot is
`W:\Daywater\research-temp\nflstreet-moderngekko-win-vulkan-automation\artifacts\two-fastpaths.png`,
SHA-256 `9106108eb7e4b176b7a156ce8cc1fc618ff2319b1de45854900e638420a2215a`.

The generated source injections were removed after the trial. The normal
module in the build tree and bundled launcher was restored byte for byte,
SHA-1 `dd521cc38b0db6bca98a0bb261570a2be5e6e0ed`. The combined DLL
remains on private W: for further verification and is not the default.

## Longer replay and RAM checkpoint (2026-10-05)

A further paired replay from the same state ran 1,802 baseline frames and
1,805 combined-candidate frames. Both exited cleanly with zero interpreter
fallback and failed chunks. The candidate logged hits for both functions.
The observed rates were 30.94 and 31.79 FPS respectively. These are single
runs; their differing final frame counts and host variation make them a
stability check, not a precise speed or state-equivalence measurement. Logs
and hashes are at `W:\Daywater\research-temp\nflstreet-two-fastpaths-1800x1`.

The [checkpoint harness](checkpoint_windows_state.py) then loaded the same
saved state, paused, replayed a fixed count of neutral-pad frames, paused
again, and captured all 24 MiB of MEM1 on W:. Four 120-frame captures (two
per DLL) reported the same checkpoint frame, 16682. Two baseline captures
already differed in 3,722 bytes across five 4 KiB pages. The two candidate
captures also differed. At a 10-frame checkpoint, two baseline captures
differed in 17,994 bytes. Thus whole-RAM hashes are not reproducible in this
setup and cannot establish candidate equivalence or a regression. All seven
captures had the same score pointer `0x803B6C20` and score halfwords `(0, 0)`;
that is a limited live-state check, not proof of full equivalence.

Private capture directories are named `nflstreet-checkpoint-{baseline,candidate}-120`
and `nflstreet-checkpoint-{baseline,candidate}-10` under
`W:\Daywater\research-temp`, with `-repeat` for repeated captures. Raw RAM
and game data remain outside Git. A useful next comparison needs a
deterministic time/RNG setup or live per-call differential checking so that
unrelated run-to-run memory changes are excluded.

The subsequent [live differential probe](live_differential_probe.md) made
that per-call comparison on the same live inputs: 32/32 accepted polynomial
calls and 16/16 accepted product calls matched full CPU state and touched
RAM, with zero mismatches. The product guard declined 16 of its first 32
calls, which continued through the original generated implementation.
Those declines were later traced to the runtime's
[locked-cache range](locked_cache_fastpath.md). A separately tested candidate
now covers that range under an explicit pointer and callback guard.
