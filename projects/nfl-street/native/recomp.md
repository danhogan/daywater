# Whole-game static recompilation experiment

Checked 2026-10-03 against the local GNNE69 NFL Street image. This is a
possible route to a native Windows build, separate from the reviewed score
function decompilation in `../reconstruction`. Generated code and extracted
game data stay on W: and must not be committed or redistributed.

## Input identity

- Disc image SHA-1: `813c4511ba3135059a13de424555647e8252254b`
- Extracted `main.dol` SHA-1: `46e3de854998a0f66c9be8c72a1164d22cd29eb4`
- Game ID reported by DolRecomp: `GNNE69`
- Local ISO: `cache/isos/813c4511ba3135059a13de424555647e8252254b.iso`
- W: extraction: `W:\Daywater\research-temp\nflstreet-moderngekko-game`

The extracted `sys/main.dol` hashes identically to the already researched
`cache/extracted/.../boot.dol`. The W: extraction occupies about 906 MiB.

## Tools checked

- [DolRecomp](https://github.com/ExpansionPak/DolRecomp), commit
  `c876e2b9e022e084aa9a690fe15e2ad79a0706cd`, built at
  `W:\Daywater\tools\DolRecomp`. Its C backend passed 20/20 tests; its
  LLVM backend passed 36/36 tests.
- [ModernGekko-Template](https://github.com/ExpansionPak/ModernGekko-Template),
  commit `c2553a99584b2045dbe46a70e1570994dae8beef`, cloned at
  `W:\Daywater\tools\ModernGekko-Template`. Its pinned ModernGekko commit is
  `edd460adad0e82b56909af1d793642c83b6d31c8`.

On this Fedora WSL host, ModernGekko CMake configuration required
`libXi-devel` and `libXrandr-devel`; the first configure failed without them,
and the second completed. ALSA, PulseAudio, and BlueZ were optional and
disabled by that configuration. The runtime build directory is
`W:\Daywater\tools\ModernGekko-Template\lib\ModernGekko\build`.

`moderngekko-port inspect` recognizes the extracted tree as `NFL STREET`,
`GNNE69`, GameCube/Gekko, entry `0x80003100`, and DOL SHA-256
`a065ae2f4c192124266a56171aa5eed4e2dccb586d32670c5dab2e2ed3ad980c`.
The template's C backend built a native Linux `gGNNE69_recomp.so` from all
146 generated chunks with GCC and linked it successfully. The output is a
56 MiB x86-64 ELF shared object. Its cache is under
`W:\Daywater\research-temp\nflstreet-moderngekko-modules`.

The first `moderngekko-run --headless` test did **not** reach a confirmed game
boot. Dolphin's Wiimote scanner stopped at `IOhidapi.cpp:97` because
`hid_init()` returned an error. The initial CMake configuration lacked
libudev and chose the bundled libusb HIDAPI backend. `systemd-devel` was
installed to provide libudev, and CMake then selected the bundled HIDAPI
`linux/hid.c` source. A rebuilt runner passed this initialization point.

Booting `sys/main.dol` with null video loaded the module and eventually
executed native instructions, but its status remained at zero frames. A
non-static Dolphin JIT baseline from the same extracted tree also remained
at zero frames. `MODERNGEKKO_BOOT_IMAGE` in the small local
`moderngekko-boot-image.patch` lets the runner instead pass an ISO path to
`BootParameters::GenerateFromFile`; without that environment variable, the
runner retains its original DOL-only path. The patch was applied to the W:
ModernGekko checkout only. The original game image was copied to
`W:\Daywater\research-temp\nflstreet-GNNE69.iso` and its SHA-1 was checked
against the input identity above.

With that patch and the ISO, this bounded run reached 140 frames and 442
present events before the 30-second timeout:

```bash
MODERNGEKKO_BOOT_IMAGE=/mnt/w/Daywater/research-temp/nflstreet-GNNE69.iso \
  timeout --signal=INT --kill-after=5s 30s \
  /mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/build/moderngekko-run \
  --game /mnt/w/Daywater/research-temp/nflstreet-moderngekko-game \
  --module /mnt/w/Daywater/research-temp/nflstreet-moderngekko-modules/GNNE69/a065ae2f4c192124266a56171aa5eed4e2dccb586d32670c5dab2e2ed3ad980c-c4dcfb815f5c2267/gGNNE69_recomp.so \
  --user-dir /mnt/w/Daywater/research-temp/nflstreet-moderngekko-user \
  --automation-dir /mnt/w/Daywater/research-temp/nflstreet-moderngekko-iso-automation \
  --headless --graphics Null
```

At shutdown the static recompiler reported 88,086,619 native dispatches,
zero interpreter fallback steps, 83,369 native exceptions, 2,271,260 hook
fallback instructions, 79 verifications, zero failed self-modifying-code
chunks, and 2,660,384,268 charged guest cycles. The automation status said
`booted=1`, `state=running`, `frame_count=140`, and `present_count=442`.
These observations verify a headless CPU/render-loop boot in this
environment. The null backend produces no viewable image; gameplay,
controller input, and audio were not verified by this Linux test.

## Native Windows result

The existing Visual Studio 2022 Build Tools installation contains MSVC
`19.43.34808.0`. Portable Kitware CMake `4.4.3` was downloaded to W:; its
Windows x86-64 ZIP SHA-256
`4d52ebab7193a698651639ed80d8d04fd903358843572cf44c7fd234cb7c26ab`
matches Kitware's release checksum manifest. Ninja was already installed on
Windows. The Windows build is separate at
`W:\Daywater\tools\ModernGekko-Template\lib\ModernGekko\build-win64`.
`configure_windows_runtime.cmd` and `build_windows_runtime.cmd` record the
local MSVC/CMake commands.

MSVC rejected two lambda declarations in the pinned Dolphin source that omit
`()` before `mutable`. `moderngekko-msvc-lambda.patch` inserts those empty
parameter lists in the W: checkout. This does not change their behavior. The
Windows runner, port tool, and recompiler then built successfully.

The first Windows module cache path exceeded Windows' object path limit.
`build_windows_module.cmd` uses the short `W:\nfsmod` output path. The native
Windows port tool recognized the same DOL SHA-256 and compiled all 146 chunks
into `gGNNE69_recomp.dll` (160,538,624 bytes).

`run_windows_headless.py` boots the verified ISO with that DLL and can select
null or Vulkan graphics. On Windows, a 30-second headless/null run reached
173 frames and 558 presents. A later windowed Vulkan run stopped cleanly via
ModernGekko's automation `stop` command with exit code zero. Its final status
reported `booted=1`, 170 frames, 532 presents, and 9.2 FPS at the instant of
the status sample. The static recompiler reported 99,036,063 native
dispatches, zero interpreter fallback steps, 138,345 native exceptions,
2,716,620 hook fallback instructions, 79 verifications, zero failed
self-modifying-code chunks, and 3,140,520,718 charged guest cycles. A
separate 60-second windowed run reached 415 frames and 1,705 presents.

This is a **verified native Windows boot and running render loop** for the
researched image. A screenshot taken after startup was black; a later
640×491 PNG taken after the intro advanced shows the game's street-football
intro video. That frame is preserved at
`W:\Daywater\research-temp\nflstreet-moderngekko-win-vulkan-automation\artifacts\boot-intro-70s.png`.
The automation channel accepted a three-frame GameCube Start input, but a
later screenshot still showed the intro; game response to that input is not
established. A longer bounded run reached the NFL Street title screen with
"Press Start" visible in a 640×491 screenshot at
`W:\Daywater\research-temp\nflstreet-moderngekko-win-vulkan-automation\artifacts\boot.png`.
Sending ten frames of Start from that visible title state advanced to the
main menu with "Play It" selected, captured at
`W:\Daywater\research-temp\nflstreet-moderngekko-win-vulkan-automation\artifacts\after-start.png`.
This verifies a working GameCube Start input and front-end navigation in the
native Windows runner. The 240-second run stopped cleanly with exit code zero
after 5,765 frames, 13,756 presents, 1,102,036,406 native dispatches, zero
interpreter fallback steps, and no automation error. A second bounded run
pressed A with "Play It" selected and captured the resulting menu with
"Quick Game", "Pickup Game", and "NFL Challenge" at
`W:\Daywater\research-temp\nflstreet-moderngekko-win-vulkan-automation\artifacts\after-play-it.png`.
Its automation channel also saved a 19 MiB menu state at
`W:\Daywater\research-temp\nflstreet-moderngekko-win-vulkan-automation\artifacts\menu.sav`.
Loading that state in a fresh Windows runner process restored the "Play It"
menu, confirmed by the screenshot at
`W:\Daywater\research-temp\nflstreet-moderngekko-win-vulkan-automation\artifacts\loaded-menu.png`.
From that state, two separate A presses selected "Play It" and "Quick Game".
The next captured screen was "Select Controller" with "None" selected and
"User ID" shown above it, at
`W:\Daywater\research-temp\nflstreet-moderngekko-win-vulkan-automation\artifacts\after-quick-game.png`.
The intermediate Quick Game menu and controller-selection state were also
saved on W: for repeatable front-end experiments.
At "Select Controller", one D-pad Up input changed the highlighted User ID
choice from "None" to "Create". From the saved "None" state, A advanced to
"Select Teams", displaying Chiefs and Falcons in the capture at
`W:\Daywater\research-temp\nflstreet-moderngekko-win-vulkan-automation\artifacts\after-controller-continue.png`.
Two further A presses selected the highlighted Chiefs and Falcons. The
resulting screen offered "Auto Pick" for Player 0 of 7; A filled the lineup
and displayed a highlighted "Ready" choice. The captures are at
`W:\Daywater\research-temp\nflstreet-moderngekko-win-vulkan-automation\artifacts\after-both-teams.png`
and
`W:\Daywater\research-temp\nflstreet-moderngekko-win-vulkan-automation\artifacts\after-auto-pick.png`.
Pressing A on "Ready" advanced to "Select Field", with EA Field highlighted;
the capture is at
`W:\Daywater\research-temp\nflstreet-moderngekko-win-vulkan-automation\artifacts\after-ready.png`.
Pressing A on EA Field loaded a Chiefs versus Falcons match at 0–0 and
reached the offensive/defensive play-selection screen. The visible capture is
at
`W:\Daywater\research-temp\nflstreet-moderngekko-win-vulkan-automation\artifacts\after-field-select.png`.
A 23 MiB savestate of that state is at the matching `after-field-select.sav`
path. From that state, selecting the highlighted play and capturing earlier
in the ensuing interval produced an on-field pre-snap view with the Chiefs and
Falcons lined up, a selected player marker, and turbo meters:
`W:\Daywater\research-temp\nflstreet-moderngekko-win-vulkan-automation\artifacts\early-play.png`.
This verifies progression into a rendered match. An A input from the saved
on-field state produced a screenshot with players moving through a play at
`W:\Daywater\research-temp\nflstreet-moderngekko-win-vulkan-automation\artifacts\after-onscreen-a.png`.
A no-input run from the same state also showed a play in motion at
`W:\Daywater\research-temp\nflstreet-moderngekko-win-vulkan-automation\artifacts\no-input-control.png`.
Therefore those runs do not establish that A caused the snap. The separate
[on-field input check](gameplay_input.md) held left, neutral, or right for 180
frames from the same saved state in six fresh Windows processes. The selected
actor's X position and screenshots separated consistently by direction,
verifying automated main-stick control in this tested play.

The Windows ModernGekko user directory lacked `Config/GCPadNew.ini`. The
machine's existing Dolphin keyboard profile was copied there locally; it
maps A to X, B to Z, Start to Enter, and the main stick to arrow keys.
`launch_windows_match.cmd` starts the runner from the saved match
play-selection state, using the verified local ISO and native module. It was
staged as `W:\Daywater\research-temp\launch_windows_match.cmd` for manual
play. The user manually confirmed that X and the arrow keys work in the
window, while reporting that play is very slow. The other mapped keys have
not been manually checked. The runner selected Cubeb
audio in the windowed test, but sound was not observed. The Windows DLL and
runner depend on the locally owned ISO at runtime and are not distributable
game artifacts.

## Match stability and speed

A 210-second windowed Vulkan run from `after-field-select.sav` stopped cleanly
with exit code zero, zero interpreter fallback steps, and no automation error.
It reached frame 19,136 with 5,149 presents and 311,089,463 native
dispatches. Captures at approximately 60 and 190 seconds show the match
continuing in the second period at 0–0. The late capture is at
`W:\Daywater\research-temp\nflstreet-moderngekko-win-vulkan-automation\artifacts\late-match.png`.
The bounded run did **not** reach the end of a match. Its last sampled speed
was 0.18× real time, around 11 FPS, so full-match stability remains
unverified. A 60-second headless/null run from the same saved state also
sampled about 0.17× speed; switching off Vulkan did not show an immediate
speed gain in this non-profiled comparison.

The runtime source selects the static recompilation CPU core by default,
even when `allow_interpreter` permits running without a module.
`MODERNGEKKO_STATICRECOMP=0` selects its Dolphin JIT64 CPU core on this
Windows x64 host. Three 60-second runs from the same savestate used Vulkan
and the same 640×528 render resolution:

| CPU path | Module | Final frame | Last sampled speed | Exit |
|---|---|---:|---:|---:|
| C-backend static recompilation | GNNE69 DLL | 17,064 | 0.19× | 0 |
| Static core without a module | none | 17,921 | 0.57× | 0 |
| Dolphin JIT64 | none | 19,287 | 1.00× | 0 |

These are bounded observations, not a formal performance profile; startup
and game events can affect short runs. The DLL was compiled with MSVC Release
`/O2 /Ob2`. The JIT baseline is a diagnostic control, not the desired
decompiled build.

For manual speed comparison, `launch_windows_jit_reference.cmd` is staged at
`W:\Daywater\research-temp\launch_windows_jit_reference.cmd`. It uses the
same ISO, saved match state, and keyboard profile, with
`MODERNGEKKO_STATICRECOMP=0` and `--allow-interpreter` to select JIT64. This
path does not run the statically recompiled DLL; use
`launch_windows_match.cmd` for that build. A separate 20-second JIT64 run with
the current restored runner reached 1.00× speed and exited zero. Manual
keyboard input in this JIT64 launcher has not yet been checked.

The runtime's opt-in `StaticRecompIdlePC` setting identified a concrete
performance opportunity. Built-in dispatch sampling put `0x801F471C` first
(1,888 of approximately 13,254 samples) during a 40-second match run. The
verified DOL disassembly shows `lwz`, `cmplwi`, and `beq` at that address,
repeating while a scheduler-like mask at `r13-0x5590` is zero. The role of
the mask is still a candidate; it was not renamed in Ghidra. The static CPU
core calls `CoreTiming::Idle()` when a configured idle PC is reached. A
separate W: user directory sets
`[Core] StaticRecompIdlePC = 2149533468` (hex `0x801F471C`) without
changing the original test profile or the game code.

Two 60-second runs from the same match state using that isolated profile
reached frames 17,781 and 17,815, compared with 17,064 without the setting.
Both exited cleanly, rendered match action, and reported zero interpreter
fallback steps. The last instantaneous speed samples varied with game phase
(0.60× and 0.35×), so final frame count is the better bounded comparison.
The [on-field control check](gameplay_input.md) was repeated with the setting:
Green's X after neutral, left, and right input was -0.368, -12.609, and
+8.916 respectively. A no-input run from the saved first-period pre-snap
state also auto-snapped and reached active play with the idle setting.

A further 180-second windowed run using the isolated profile exited cleanly
at frame 19,566 with 6,009 presents, zero interpreter fallback steps, and no
automation error. The late capture still showed a second-period field state;
the match did not finish, and the last sampled speed had fallen to 0.20×.
The benefit is clear in the early wait-heavy stretch but is not a complete
performance fix. The staged `launch_windows_match.cmd` uses this isolated
profile by default for local testing. Its second argument can select the
original user directory if a later behavioral difference appears. Neither
profile has been manually played through a full match. The user confirmed
working X and arrow-key input in the default launcher, with poor speed.

Dispatch sampling from the saved second-period state with the idle setting
put `0x801F0BC0` (384 samples) and `0x801F0B94` (157) above the old wait
loop at `0x801F471C` (72). DOL disassembly places the first two addresses
inside loops containing `dcbf` and `dcbi` cache instructions, respectively.
The runtime already has a fast path for these instructions, but each still
performs cache invalidation work. These are **dispatch-frequency samples, not
a time profile**; they do not yet identify a safe optimization or prove the
cache loops dominate elapsed time. They must not be treated as idle loops.

An experimental opt-in timer patch (`moderngekko-timing-probe.patch`) sampled
elapsed host time around generated-code dispatch and the cache-operation
hook. It was built with `build_windows_runner.cmd` and enabled through
`run_windows_headless.py --timing-samples`; the staged manual launcher did
not enable it. Randomized sample spacing avoided repeatedly measuring the
same position in a guest loop. Two bounded windowed Vulkan runs from
`after-field-select.sav` produced:

| Duration | Native dispatches | Dispatch sample mean | Cache-hook calls | Cache-hook sample mean | Exit |
|---|---:|---:|---:|---:|---|
| 60 seconds | 121,190,714 | 389 ns (29,605 samples) | 9,934,189 | 97 ns (9,703 samples) | `0xC000041D` |
| 30 seconds | 50,203,925 | 373 ns (12,255 samples) | 4,351,006 | 95 ns (4,243 samples) | `0xC000041D` |

These samples suggest that repeated generated-code dispatch is a larger host
CPU cost than cache-hook invalidation. They are an estimate, not a full CPU
profile, and include any hooks called inside a sampled dispatch. Both
instrumented runs printed shutdown statistics but then returned a Windows
teardown error. The experimental source changes were reverted and the normal
runner rebuilt; the launcher uses that normal build. The teardown error's
cause has not been established. A 20-second run with the restored runner
exited zero, reported no fallback steps, and reached a last sampled speed of
0.45× from the same saved match state.

A smaller generic runtime experiment cached the last verified code-chunk
range before consulting the per-instruction lookup table. The change is
recorded in `moderngekko-fast-chunk-cache.patch`, but is **not** in the staged
runner. Matched 30-second runs from `after-field-select.sav` reached frame
16,697 without the cache and 16,676 with it; both exited zero and reported
zero fallback steps. This did not show a speed benefit, so the change was
reverted and the normal runner rebuilt.

The pinned DolRecomp C emitter has an opt-in cross-chunk direct-call helper,
but the generated GNNE69 module contains no calls to it. Its own source notes
that these calls bypass runtime code-validation checks. This game has
documented self-modifying-code candidates and the native runs report chunk
reverification events, so enabling that shortcut without a validation gate
would risk executing stale code. The LLVM backend is another performance
direction, but its previous whole-DOL attempts remained incomplete (see
below). Neither route has a verified full-speed result yet.

`run_windows_headless.py --frames N --seconds LIMIT` now stops after a fixed
observed frame delta from the first live status update. The automation status
is refreshed every 200 ms, so the stop can overshoot `N`; compare the actual
observed frame count and elapsed wall time. This also excludes variable boot
time from the timed interval. A validation run from the same saved match
state with `--frames 120 --seconds 90` gave:

| CPU path | First frame | Last observed frame | Observed frames | Wall time | Approx. FPS | Exit |
|---|---:|---:|---:|---:|---:|---:|
| Native GNNE69 DLL | 16,562 | 16,682 | 120 | 8.563 s | 14.0 | 0 |
| JIT64 reference | 16,562 | 16,694 | 132 | 2.250 s | 58.7 | 0 |

This is a short fixed-work comparison, not a full-match benchmark. A future
performance change should run a longer identical frame interval and confirm
the resulting state and rendered scene, in addition to checking exit status.

### Inline cache-operation experiment

The hot DOL loops at `0x801F0BBC` (`dcbf`) and `0x801F0B90` (`dcbi`) were
returning to the runtime after every cache-line operation. The separate
`dolrecomp-inline-cache-experiment.patch` emits a call to the existing
cache-control callback and continues in generated C when that callback leaves
the current code valid. `moderngekko-inline-cache-experiment.patch` makes the
callback return to the runtime after code invalidation, an exception, or a
dcache-on operation. The generated C charges cache-op cycles in a separate
block so an early return does not charge later instructions. The vendored
sources have been restored after building; the two patches record the change.
The experimental module is selected by
`W:\nfscache\GNNE69\active-module.txt`, while the normal module remains
under `W:\nfsmod`. The experimental runner is saved separately as
`moderngekko-run-inline-cache.exe` in the Windows build directory.

Two fixed-frame runs from `after-field-select.sav` with the same user profile
reached 120 frames in 4.875 seconds (24.62 FPS) and 123 frames in 5.094
seconds (24.15 FPS). The normal DLL reached 120 frames in 8.563 and 6.734
seconds in separate runs (14.0 and 17.82 FPS). These are bounded wall-time
observations subject to host load. All four exited zero. The experimental
runs reported zero interpreter fallback steps and zero failed code chunks;
their cache hook fallback counts were roughly 25–26 thousand versus more
than 1.17 million for the normal DLL. Cache instructions still execute via
the callback, so the counter drop does not mean they were skipped.

From the same saved on-field state, fresh experimental processes applied A
for three frames and then neutral, left, or right main-stick input for 180
frames. The selected actor Green's ground X was approximately +1.517,
−12.572, and +6.760 respectively; screenshots showed active field play.
All three runs exited zero with no interpreter fallback or failed code chunks.
The controlled inputs show that direction still affects the game correctly,
but these snapshots are not a byte-for-byte comparison with the normal build.

A 120-second windowed Vulkan run from the play-selection state exited zero
with no reported error, zero interpreter fallback, and zero failed code
chunks. It reached frame 19,444; its late screenshot showed the second-period
play-selection screen at 0–0. Focused DolRecomp CFG and generated-code tests
passed 2/2. Continued match progression is established, while a complete
match, audio, manual keyboard input with this build, REL cache operations,
and state equivalence remain unverified.

After separating the binaries, a 60-frame experiment reached its target but
did not exit after the automation stop command; the harness eventually sent
Ctrl-Break and returned `0xC000013A`. The cause is unknown. Subsequent
30-frame and 120-frame runs of the same saved experimental pair both exited
zero. A matched 120-frame comparison from the play-selection state measured
28.24 FPS experimental versus 27.94 FPS normal. A longer comparison from the
saved on-field state measured 502 frames in 20.047 seconds (25.04 FPS)
experimental versus 501 in 20.110 seconds (24.91 FPS) normal. Both longer
runs exited zero with no interpreter fallback or failed code chunks. The
experimental run reduced cache-related fallback calls from 4.95 million to
about 102 thousand, but these matched wall times do **not** establish a
practical speed gain. The earlier 14–24 FPS comparison likely included host
load variation. The normal launcher and DLL remain available. The opt-in
`launch_windows_inline_cache_experiment.cmd` uses the saved experimental
runner and module; `build_windows_inline_cache_experiment.cmd` records the
module rebuild command after reapplying the patches.

Dispatch sampling over another 502-frame run from `early-play.sav` with the
experimental pair placed `0x80188BB0` first (789 samples), followed by
`0x801FFDF0` (305), `0x802033CC` (265), and `0x801CCDFC` (252). The old cache
loop PCs `0x801F0BC0` and `0x801F0B94` were absent from the reported top 16.
The generated C at `0x80188BB0` corresponds to a bounded PowerPC function
ending at `0x80188C20`; it loads, combines, and writes values with multiple
`psq_l`, `ps_mul`, `ps_madd`, and `ps_sum0` instructions. `0x801FFDF0` also
begins a paired-single-heavy function. Their precise game roles are unknown.
These are **dispatch-frequency samples**, not an elapsed-time profile; they
identify the next code to profile, not a proven cost center. The paired-single
path should be measured and checked for numerical equivalence before any
optimization.

An opt-in [timed-site profile](timed_sites.md) measured sparse native dispatch
time across three paired 600-frame runs from the same saved state. The top two
guest PCs were `0x80188BB0` and `0x801FFDF0` in every timed run. Host FPS
drifted across the batch, so these rankings guide the next investigation;
they do not establish an optimization's speedup.

The [two guarded function candidates](two_fastpaths_experiment.md) were
subsequently compared with the original module on synthetic CPU/RAM states,
then exercised together in a separate native Windows DLL. Three paired
600-frame trials from the saved match measured 29.02 median headless FPS for
the candidate and 28.15 for the unchanged baseline. The normal launcher
continues to use the original DLL; full-match equivalence remains open.

## Verified generation result

The DolRecomp C backend processed the whole DOL and produced about 145 MiB of
generated C/header/chunk files under
`W:\Daywater\research-temp\nflstreet-dolrecomp-c876e2b9\generated`.
The generation command used the verified DOL directly:

```bash
/mnt/w/Daywater/tools/DolRecomp/build/dolrecomp \
  --backend=c --cpu gekko --gamecube \
  cache/extracted/813c4511ba3135059a13de424555647e8252254b/boot.dol \
  /mnt/w/Daywater/research-temp/nflstreet-dolrecomp-c876e2b9
```

It recognized 590,873 PowerPC instructions, marked 23 embedded data words,
and reported zero unknown instructions. The generated entry chunk and three
score-related chunks pass GCC C11 syntax checking. Its report lists 141
potential self-modifying-code ranges requiring further investigation.

Two LLVM backend attempts were interrupted before completion because their
default/fixed partitioning led to 14,098/12,567 objects respectively. Their
partial output remains on W: for diagnosis. LLVM generation is not yet a
usable whole-game result.

Generated C is a mechanical static translation. It is **not** a reviewed C
decompilation, a byte-matched reconstruction, or proof that NFL Street boots.
The existing small C candidates remain separately labeled by evidence level.

## Runtime path

The ModernGekko template now builds and boots a native module from this exact
DOL on Linux and Windows. The original game files are still required at
runtime and remain outside Git. The next verification targets are full-match
stability, audio, and performance; manual X and arrow-key input works in the
staged Windows launcher.

## Private local Windows bundle

On 2026-10-04, `stage_local_windows_bundle.py` staged a 2.5 GiB private
directory at `W:\Daywater\research-temp\nflstreet-native-windows-local`.
It contains the hash-checked GNNE69 ISO, extracted game tree, Windows
static-recompilation DLL, runner, saved match state, controller profile, and
relative-path `launch.cmd`. The runner's 6.4 MiB adjacent `Sys` directory is
essential: a control with the identical runner executable relocated without
`Sys` booted the intro instead of restoring the saved match. Restoring `Sys`
made the same relocated runner load the saved match at frame 16,562.

A fixed-frame headless check using only bundle paths advanced from frame
16,563 to 16,688 (125 observed frames in 4.484 seconds), reported zero
interpreter fallback steps and zero failed code chunks, and exited zero. A
separate windowed Vulkan run using only bundle paths reached frame 16,956,
exited zero, and produced a screenshot of the Chiefs–Falcons first-down field
with Green selected. The screenshot is retained on W: as
`nflstreet-moderngekko-win-vulkan-automation\artifacts\bundle-match-verified.png`
(SHA-256 `18fffe8a9a2f5537d2c3086ac2035d41ccd4a3959d2b1bc30a0ee085e0927b1e`).
The source ISO, DLL, runner, and savestate hashes are recorded in the
bundle's `manifest.json`.

The bundle removes development-tree path dependencies for this local
launch. It still uses ModernGekko's runtime and the owned game files. The
saved match remains slow; these bounded checks do not establish a completed
match, sound output, or full gameplay correctness. The first visual bundle
capture, taken before `Sys` was copied, showed the intro and is not evidence
of a match restore. `run_windows_headless.py` now accepts direct bundle
paths and handles a frame-counter reset when a boot starts without restoring
the saved state; the reset is reported in its benchmark output. For this
match state, `--minimum-start-frame 16000` rejects an intro boot that
advances frames but fails to restore the match. A 30-frame bundle run with
that guard passed, starting at frame 16,562 and exiting zero.

The bundle also includes `early-play.sav` (SHA-256
`fc82b85bfc29c9d91d45e89d26e18273c9a62807f6912631a1a2f37bf6565023`).
`probe_windows_controls.py` was run against only bundle paths in three fresh
Windows processes. It sent three frames of A, then 180 frames of neutral,
left, or right main-stick input. The previously verified roster locator
identified Green at actor `0x803C04EC` in all three 2 MiB memory captures:

| Input | Green X | Green Y | Exit | Interpreter fallback |
|---|---:|---:|---:|---:|
| Neutral | +0.0301 | -34.5493 | 0 | 0 |
| Left | -8.6527 | -38.2088 | 0 | 0 |
| Right | +8.4676 | -36.7098 | 0 | 0 |

The same selected-player marker is visible in the retained frame captures
under `nflstreet-moderngekko-win-control-{neutral,left,right}-bundle` on W:.
These runs verify automated controller input through the bundled native
module in this saved play. They do not verify every keyboard mapping or a
complete match.
