# Guarded native input-vector to movement bridge

The [actor runtime](actor_runtime.h) now accepts a normalized 0x20-byte
controller vector through `nflstreet_actor_runtime_step_vector`. It applies
the [bounded original input translator](../input_translation/input_translation_replay.c),
models the observed heading handoff, and advances the actor through the
existing reset and movement step. This is a native C API. The replay CLI
accepts `V:` followed by comma-separated 64-hex-character vectors to test it.

**Reference:** NFL Street GameCube GNNE69 revision 0, ISO SHA-1
`813c4511ba3135059a13de424555647e8252254b`, DOL SHA-1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`, Dolphin 2603a,
saved-match state SHA-256
`ea3d1fa29797d1521b2ab83aee866c190ed092865b7739ceb537d15aba5e7f32`.

In a fresh right-to-forward three-update capture, the live call to
`0x8003DDEC` received the normalized forward vector (`y = 1.0`) before
updates 1 and 2. The existing Windows translator executable matched the
complete 0x30-byte actor input block at both original returns. At the first
return, the requested heading at actor `+0x204/+0x208` had changed to
`0x003FFFFF`; the active heading at `+0x214/+0x218` still held the prior
right direction. A later live watchpoint stopped at `0x80014AF4` while
writing `+0x214`. Its registers placed the source at actor `+0x200` and
destination at actor `+0x210`. The hash-checked DOL instructions there copy
the two heading words at `0x80014AF4/0x80014AFC` and the strength at
`0x80014B04`. Separate write-watchpoint stops at `0x800F8254` and
`0x800F8618` identify the following strength writes; the latter left both
input strengths at the settled value observed before movement preparation.
The watchpoint's pre-step bulk RAM read returned zeroes, so the claim about
individual stores relies on the stopped PC, registers, DOL instructions, and
post-step values, not that anomalous pre-step read.

The vector bridge models this *guarded saved-match branch*: it copies the
translated heading into the active heading fields, sets the observed active
status byte, and caps the requested strength at the DOL's 0.72f value for
both strength fields. It rejects zero and diagonal vectors or unequal
starting strengths. Fresh partial-strength captures support this rule and
the [movement speed-cap branch](partial_strength.md). Other branches of
`0x800F8254..0x800F8618` remain untested.

The actual Windows x64 build matched every byte of the final 0x30-byte actor
input block and every word of the 0x54-byte movement block on all three calls
in each of four live direction sequences: `RFF`, `FRR`, `RFR`, and `RLL`.
The original translator entry/return was captured and separately matched
for the two later calls of `RFF`; the other three captures check complete
preparation blocks and movement exits. Earlier calculated-reset and
captured-reset traces also still pass.

An independent eight-update run used `RFRRRRRR`. The first build differed
by one bit in movement speed on update 2; that value propagated to the copied
prior-speed word on update 3. The original DOL length helper at
`0x801DDFCC` uses `fmuls` for `y*y` followed by `fmadds` for `x*x + y*y`.
The C replay had rounded the second multiply separately. Reproducing the
fused operation removed the difference. The rebuilt Windows actor runtime
matches every actor-input byte and all 21 movement words on **all eight calls**;
the standalone kinematics replay matches the eight movement calls as well.
The four earlier three-update
vector cases also still match. This verifies a longer isolated run, while
the rest of the match still depends on game scheduling and services.

| Private W: evidence | SHA-256 |
|---|---|
| `thomas-switch-input-function-rff.jsonl` | `4a35ab06fd15e0c26de093826a5bd50cdd90a33746e63c0c28871ff121a01a3e` |
| `thomas-switch-input-heading-rff.jsonl` | `cd759fc99e2541822003a2aa73ed92169d0b602eac8abfa355a0f3c1beab0d8e` |
| `thomas-switch-input-magnitude-rff.jsonl` | `898a563741996b82dbe0e2539633b7e33febb78a541aba499bbc77e26a0d5b48` |
| `thomas-vector-eight-rfr.jsonl` | `29bca10846ff3e524fee2059d3ea840649e3288c33cd2d54e6e77c72964926aa` |

The rebuilt executable is
`W:\Daywater\research-temp\nflstreet-actor-update-loop-replay.exe`, SHA-256
`3b7cc1274c9d83b7b86be5d3cd95bd9c8d4b5d66fff3ef8fe5a77e3b91b29834`.
Raw traces, the incomplete forward-to-right input-function diagnostic, and
executables remain private on W:. The Daywater container source and the
normal gameplay DLL were restored to their prior hashes.

**Confidence:** `verified` for final actor and movement blocks on the four
three-update sequences and the eight-update saved-match sequence, and for
the original translator returns in the
captured `RFF` run. `candidate` for using the observed 0.72f cap in other
game states. The bridge still starts from a saved actor and movement state;
it does not schedule the rest of the match, render, or replace the emulator.
