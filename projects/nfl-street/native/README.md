# Native Windows support layer

Host implementations for interfaces used by reconstructed NFL Street code
belong here, separate from the GameCube function candidates under
`../reconstruction` and separate from generic Daywater.

`fnig_store.c` currently provides an in-memory `CSHG`/`CSAG` record for the
Windows score-path executable. It replaces the original query dispatcher at
`0x801B4510` only for the two researched score fields. There is no disk
persistence, asset loading, game loop, rendering, sound, or controller input.
The [score-event replay](score_event_replay.md) feeds this native C path two
records captured at the live GameCube score handler. The failed and
successful PAT outcomes both matched Dolphin's RAM score and writer value.
The separate [kinematics replay](../reconstruction/kinematics/README.md)
matches six complete live movement updates in Windows C without using the
ModernGekko runtime for replay.

`recomp.md` records the separate whole-DOL static recompilation and
ModernGekko runtime experiment. Its generated game code and extracted disc
data live on W: outside this repository.

`launch_windows_match.cmd` opens the local Windows runtime from a saved
NFL Street play-selection state. It expects the locally owned ISO, compiled
module, and savestate on W: as described in `recomp.md`. Its default user
profile includes the tested game-specific idle-PC setting; a second argument
can select the original profile.

`stage_local_windows_bundle.py` copies those locally owned inputs into a
single private W: directory with a relative-path `launch.cmd`. The directory
includes the runner's adjacent `Sys` resources, which are required to restore
the saved match after relocating the executable. Run the staging script from
WSL, then open `W:\Daywater\research-temp\nflstreet-native-windows-local\launch.cmd`
on this Windows machine. The bundle contains the ISO and extracted game files;
keep it private and outside Git. This is a packaged static-recompilation
experiment, still dependent on ModernGekko and not a reviewed decompilation.
The user confirmed that X and the arrow keys work in the staged Windows
window, and reported that it runs very slowly.

`launch_windows_jit_reference.cmd` opens the same saved state with the
ModernGekko/Dolphin JIT64 CPU path. It is a faster gameplay reference and
does not execute the statically recompiled DLL. Both launchers are staged in
`W:\Daywater\research-temp` and accept an optional savestate path as the
first argument and user directory as the second.

`launch_windows_inline_cache_experiment.cmd` opens an experimental static
recompilation build from the same state. It uses its own runner and module on
W:, leaving the normal launcher available. It greatly reduces cache-related
fallback calls, but matched on-field runs did not show a clear FPS gain.
Bounded match and control tests are recorded in `recomp.md`; full-match and
manual-input verification remain open. It accepts the same two optional
arguments.

`gameplay_input.md` records matched native Windows main-stick tests from a
saved on-field state. `probe_windows_controls.py` produced the screenshots and
RAM captures retained on W:.

`moderngekko-timing-probe.patch` and `build_windows_runner.cmd` record an
optional runtime profiling experiment. The probe is not part of the normal
launcher build; see `recomp.md` for its bounded measurements and teardown
error.
`moderngekko-fast-chunk-cache.patch` records a reverted runtime speed
experiment that did not improve the matched run.
`run_windows_headless.py --frames N --seconds LIMIT` provides a fixed-frame
match benchmark for future native runtime changes. `--runner` and
`--module-file` select the experimental pair for comparison.

The [timed-site profile](timed_sites.md) describes an opt-in Windows runner
patch and an offline worker that repeated fixed-frame match trials, measured
sampled native dispatch time by guest PC, and retained its logs on W:. It
identified two paired-single-heavy guest functions as the next focused
performance investigation.

The [live paired-function probe](live_pair_probe.md) captured 32 calls to
each hot function from the saved Windows match. Their output words matched
the two curated portable C candidates bit for bit. The baseline module still
uses the original generated instructions.

The [guarded fast-path experiment](odd_fastpath_experiment.md) gave the first
candidate full CPU-state coverage within explicit conditions, built it as a
separate Windows module, and measured 28.82 versus 28.01 median headless FPS
across three paired 600-frame trials. The verified baseline remains the
default module.

The [combined experiment](two_fastpaths_experiment.md) also added a guarded
full-state candidate for the second hot function. Both were exercised in a
saved Windows match; the combined build measured 29.02 versus 28.15 median
headless FPS across three paired 600-frame trials and rendered a windowed
on-field frame. The baseline launcher remains unchanged.

The [live differential probe](live_differential_probe.md) compared the
guarded candidates with the original generated functions on identical live
CPU and RAM inputs, both early and after the millionth call in a longer
saved-match replay. Every accepted call matched in those windows. The
[frame checkpoint harness](checkpoint_windows_state.py) also showed that
whole-RAM hashes vary between repeated baseline replays, so RAM snapshots
alone cannot establish full-game equivalence in the current setup.

The [locked-cache experiment](locked_cache_fastpath.md) extended the guarded
product candidate to the observed `0xE0000000` inputs and outputs. Offline
and live differential checks matched the original generated function on the
tested calls. Its separate Windows DLL did not show a reliable speed gain in
paired 600-frame trials. The normal bundled launcher remains the baseline.

The [projection experiment](projection_fastpath.md) reconstructed another
21-instruction paired-single routine at `0x801FFF10`. Its guarded candidate
matched 100,000 offline calls and 64 early/later live differential calls. A
separate Windows DLL also completed a 600-frame saved-match smoke test. The
normal bundled launcher remains unchanged.

The [weighted-pair experiment](weighted_fastpath.md) took adjacent routine
`0x801FFEBC` through guarded full-state comparison, early and later live
probes, and a separate 600-frame Windows build test. The baseline bundle
remains unchanged.
