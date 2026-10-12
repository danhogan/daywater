# Three consecutive native Thomas updates

The [sequence tracer](../../tools/capture_thomas_update_sequence.py) captured
three consecutive calls for Thomas under held right input and three under
held forward input. Both fresh Dolphin 2603a runs started from supplied
GNNE69 revision 0 state `8287f395`. Each call was stopped at movement
preparation `0x800F705C`, movement entry `0x80036B28`, and movement return
`0x80036E58`. The actor input block, header, and rating stayed identical
within each three-call sequence.

The complete 0x54-byte movement block at one call's return matched the next
call's preparation entry at **20 of 21 words**. The only difference was
acceleration at block `+0x34`: it returned to the same pre-preparation value
before each subsequent call. The right and forward sequences each reproduced
this handoff twice. The [Windows native loop replay](actor_update_loop_replay.c)
therefore carries the full output block forward, resets `+0x34` to that
captured initial value, and calls the same native actor update again.

The actual Windows x64 loop matched all 21 output words for each of the six
live movement returns. The single-call Windows replay also matched each of
the six movement-entry blocks before kinematics. The
[verifier](verify_update_sequence.py) checks the original PCs, constant actor
inputs, complete intercall block differences, all six command outputs, and
all six movement outputs.
The [calculated reset extension](reset_acceleration.md) now reconstructs the
observed intercall setup instead of supplying the reset value from the
capture. It matched separate right and forward runs and a longer right run.

In a fresh three-update Dolphin 2603a run, the held stick changed from right
to forward immediately after the first movement exit. The original game
updated its 0x30-byte actor input block before the next reset call. The
[native loop](actor_update_loop_replay.c) now accepts one actor input block per
update and uses the new block for both reset and actor update. Its Windows x64
build matched every word of all three live movement exits, including the two
updates after the direction change. The first exit matched the held-right
control run; later exits diverged in position and heading, confirming that the
new input affected movement. The [verifier](verify_update_sequence.py) also
matched all command-entry blocks and the first reset function's full output.
Trace SHA-256: `557a2544fc74f8195c07c3153fe887aa4489d8bbb712d7917e6cc175a9f2e8c2`.
The private trace is `W:\Daywater\research-temp\thomas-switch-right-forward-3.jsonl`.

For these full-strength right/forward states, only four actor heading words
changed at `+0x204`, `+0x208`, `+0x214`, and `+0x218`. A guarded loop mode
accepts `RFF` or `FRR` and computes those words from the stick direction,
starting with the saved actor input block. In a second fresh Dolphin run, the
stick changed from forward to right. The actual Windows loop matched all 21
movement words on all three calls with `FRR`; `RFF` also matched its complete
three-call run. The verifier independently checks that its four-word
projection equals each live actor input block. The second private trace is
`W:\Daywater\research-temp\thomas-switch-forward-right-3.jsonl`, SHA-256
`3ea999ec0184146ddbb4b3f143f0309493b0475a5755a29b7c592553a9d97081`.
The stick mode covers only this observed cardinal branch and still starts from
a game-populated actor block; it does not model the full controller pipeline.
A fresh right-to-left three-update run also changed only the same four heading
words, to `0x007FFFFF`. The Windows `RLL` path matched all 21 movement words
for all three calls. Its private trace is
`W:\Daywater\research-temp\thomas-switch-right-left-3.jsonl`, SHA-256
`62d000b76359a14ea9cea4b12a3b261c45cb6912317fbe7836047367ef1b876d`.
A third fresh run switched twice, right to forward to right. The Windows
`RFR` path again matched all 21 movement words at each of the three returns,
and the verifier matched every projected actor block. Its private trace is
`W:\Daywater\research-temp\thomas-switch-right-forward-right-3.jsonl`,
SHA-256 `dddc8f7e4609574f3b6130788e526a3b4eff09ea97de80fefd24c1f175818239`.
The [partial-strength branch](partial_strength.md) adds four fresh live
three-update sequences with analog strength changes, including a speed-cap
case and a simultaneous strength and direction change.

The [actor runtime interface](actor_runtime.h) now owns persistent movement
state and exposes one step per call. It accepts a complete actor input block
or a guarded R/F/L direction. The replay CLI uses this interface for
calculated-reset mode. After that extraction, the Windows build still matched
the four direction-change traces, the earlier held-right calculated-reset
trace, and the original captured-reset loop trace. A failed guarded step leaves
the runtime's previous state intact. This interface is a native game-code
boundary for future control and scheduling work; it is not a full game loop.
The [normalized input-vector bridge](input_vector_bridge.md) now connects the
guarded original translator to that native step. The Windows replay matched
complete actor input and movement blocks in four three-update live direction
sequences and a separate eight-update sequence.

| Held input | Private W: trace SHA-256 |
|---|---|
| Right | `d1cb364a0d357de3ab9886f722f89d14e5264e798303d076c686625da92653bd` |
| Forward | `443f2956dda006b32ef7d379ea2e00de4b04d2057b0896bce85a6f23a1fb166b` |

A write watchpoint on `+0x34` between the first and second forward updates
stopped at `0x80036B10`, `0x800F7FA0`, `0x800F8008`, and `0x800F8028`. The
original DOL disassembly has stores to `+0x34` at those PCs, and the caller
`0x800F86BC` invokes `0x800F7C90`, which contains the latter three stores.
A later [stepped watchpoint capture](reset_acceleration.md) verified that the
stores at `0x800F8008` and `0x800F8028` wrote the reset value in a fresh
right-input run. The intercall block change is verified by the complete
sequences.

Build with
`bash projects/nfl-street/reconstruction/build_windows_actor_update_loop_replay.sh`.
The checked executable at
`W:\Daywater\research-temp\nflstreet-actor-update-loop-replay.exe` has SHA-256
`3b7cc1274c9d83b7b86be5d3cd95bd9c8d4b5d66fff3ef8fe5a77e3b91b29834`.
The rebuilt [single-call executable](actor_update.md) has SHA-256
`bf1b2caa6fbb9e46f0aed65d76cefcadc55af30ae32c3c5e301df71107ba5fa1`.
Raw traces and Windows executables remain private on W:. The Daywater
container's debugger and session source returned to their baseline hashes,
and the normal Windows gameplay DLL stayed unchanged.

**Confidence:** `verified` for the observed right-to-forward,
forward-to-right, right-to-left, and two-switch sequences and
the earlier held-input sequences, within the guarded branch and saved match.
The original replay mode uses a captured reset value; the calculated mode
computes it for the guarded observed branch. The new loop consumes distinct
game-populated actor input blocks or computes the four observed heading words
from an R/F/L path. It does not yet translate general controller input, schedule
the game's other systems, or render a game.
