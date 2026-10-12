# Live function differential probe

**Date:** 2026-10-05  
**Game:** NFL Street, GNNE69 revision 0, ISO SHA-1
`813c4511ba3135059a13de424555647e8252254b`  
**Confidence:** `verified` for the reported live call comparisons; other
states and guard-declined calls remain untested by this comparison.

The [staging script](stage_live_diff_probe.py) injects a diagnostic at two
entry PCs in private generated C on W:. For each of the first 32 calls, the
[probe helper](live_diff_probe.inc) copies MEM1 and CPU state, runs the
guarded candidate on that private copy, and then allows the original
generated instructions to run on live state. It compares the complete
`CPUState` byte for byte, plus the stack and output RAM ranges written by the
function. The candidate's private writes have the memory journal disabled;
the original runs with the journal normally. Guard-declined calls execute
the original only. This checks both implementations on identical live inputs
without requiring whole-game replays to be deterministic.

The diagnostic Windows DLL is
`W:\Daywater\research-temp\gGNNE69-recomp-live-diff.dll`, SHA-1
`4b68c6310ad4527d1808b070af6b6d133bd26647`. A 120-frame headless
replay from `match.sav` exited cleanly, with zero interpreter fallback steps
and failed chunks. Its log is
`W:\Daywater\research-temp\nflstreet-live-diff-120.log`.

| Entry PC | Attempts | Candidate accepted and matched | Guard declined | Mismatches |
|---|---:|---:|---:|---:|
| `0x80188BB0` | 32 | 32 | 0 | 0 |
| `0x801FFDF0` | 32 | 16 | 16 | 0 |

The product guard declined half its sampled calls; the reason was not
instrumented in this run. No conclusion is drawn about candidate behavior
on those calls. The 120-frame segment and the first 32 calls do not cover a
full match, exceptional floating-point cases beyond the guard, or every
game state.

The same probe was then rebuilt with `NFLSTREET_LIVE_DIFF_START=1000000`
and exercised during a 1,801-frame replay. This samples calls after each
function's millionth invocation rather than its first calls. Its separate
DLL is `W:\Daywater\research-temp\gGNNE69-recomp-live-diff-late.dll`,
SHA-1 `d804d2e08767f6ac96d2932d5ca8485ece77ee5e`; the log is
`W:\Daywater\research-temp\nflstreet-live-diff-late-1800.log`.

| Entry PC | Later attempts | Accepted and matched | Guard declined | Mismatches |
|---|---:|---:|---:|---:|
| `0x80188BB0` | 32 | 32 | 0 | 0 |
| `0x801FFDF0` | 32 | 3 | 29 | 0 |

The longer run exited cleanly with zero interpreter fallback steps and
failed chunks. Across both windows, the accepted-call total is 64/64 for
the polynomial and 19/19 for the product, with no CPU-state or touched-RAM
mismatch. The product guard's later acceptance rate is low; its declined
calls still require the original generated implementation. The guard reason
was not recorded, so no explanation for those declines is claimed.

The staging script was reversed after each run. The default bundled DLL and
restored build-tree DLL both have baseline SHA-1
`dd521cc38b0db6bca98a0bb261570a2be5e6e0ed`. The rebuilt baseline
differed only in six PE timestamp bytes before the known baseline DLL was
copied back to the build tree. No proprietary generated C, RAM dump,
savestate, or game image is in Git.
