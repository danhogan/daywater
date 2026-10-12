# Rendered reconstructed movement

**Update:** [native neutral/braking](native_braking.md) now replaces the neutral
pause described below. This document records the earlier movement milestone.

The live Windows player now has a second movement mode that calls the existing
reconstructed input, acceleration, turning and kinematics code. Its rendered
placement uses positions from those native updates, rather than the inspection
viewer's constant-speed rule. The existing inspection mode remains available.

Private launcher: `W:\Daywater\Start-NFLStreet-MovementPlayer.cmd`.

- W/up, A/left, D/right: request forward, left or right in reconstructed mode.
- M: switch between reconstructed movement and unrestricted inspection movement.
  Switching resets the seed and inspection placement.
- R: reset; Escape: close.

Neutral input **pauses** the bounded simulation. Diagonal and backward input
are unsupported in reconstructed mode and also pause it, with the reason in
the window title. These pauses are host inspection behavior, not original
stopping or braking. The original guard failures remain explicit. The existing
inspection mode still supports all arrows/WASD and normalized diagonal speed.

## Architecture

`live_movement.c` adapts semantic directional input to the previously checked
`nflstreet_actor_runtime_step_vector` interface. It exposes relative position,
command-result heading and per-update speed to the renderer. The renderer
contains no live RAM reads or raw game addresses. All game-state layout work
stays in the existing isolated NFL Street reconstruction package.

The simulation starts from a small private NMS1 fixture extracted from a
previous original-game recording. This supplies the actor state and ratings;
it is **not** independent actor or match initialization. The code owns each
subsequent input and update. It never supplies recorded output state to its
movement computation. Expected states in the fixture are used only by checks.

The viewer chooses a 60-step/second host clock with an accumulator, separates
rendering time from native movement steps, and renders native position deltas
relative to its visible field spawn. That host rate, placement, model-facing
conversion and run/idle clip selection are inspection choices. Their original
scheduling/render selection has not been verified. The displayed body/face
remain the captured draw selection plus base head template; this does not
establish a visually identified Thomas player model.

## Verified recorded movement integration

Reference: NFL Street GNNE69 revision 0, ISO SHA1
`813c4511ba3135059a13de424555647e8252254b`, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`, Dolphin 2603a,
saved-match SHA256
`ea3d1fa29797d1521b2ab83aee866c190ed092865b7739ceb537d15aba5e7f32`.

Original recordings already documented in
[input-vector bridge](../command_to_movement/input_vector_bridge.md) and
[consecutive updates](../command_to_movement/update_sequence.md) were reused.
No new live Dolphin capture was made for this integration.

Five actual Windows window runs drive the same semantic adapter from window
keyboard messages and compare every reconstructed actor-input byte and all
21 movement words with the original recordings. They also render each update
from the resulting native position and command-result heading:

| Path | Original movement checkpoints | Windows window updates/paints |
|---|---:|---:|
| RFRRRRRR | 8 | 60/60 |
| RFF | 3 | 60/60 |
| FRR | 3 | 60/60 |
| RLL | 3 | 60/60 |
| RFR | 3 | 60/60 |

All **20 original checkpoints match exactly**, including acceleration/speed,
turning and complete state carry. The eight-call path accelerates from
0.0896274149 to 0.111094579 units per native update, with its recorded turn
progression. Its displayed relative endpoint is x=0.788392067,
y=-19.9498978 (the inspection spawn adds -20 to native relative Y).

Each run also checks that neutral, unsupported diagonal/backward requests and
reset preserve or restore the native state correctly. These are host guard
checks, not proof of original neutral/backward behavior. The Windows loop
keeps the same completed native-update count while unsupported/neutral input
is paused. Physical human-key testing remains pending; automated keyboard
messages exercise the actual window event handler.

The existing inspection path's five checkpoint BMPs remain byte-identical to
the previous build after this integration. Cached geometry and combined-scene
pixel comparisons also pass. Original runner source and normal DLL remain
unchanged. Native viewer runtime does not load that DLL, Dolphin, saved RAM,
ISO or pre-rendered animation frames. It does load private captured selection
profiles and the small movement seed, plus extracted original assets.

**Confidence:** verified for the 20 complete recorded movement states and
semantic-input-to-rendered-placement bridge in the guarded branch. Candidate
for continuous longer play, visual correspondence of animation/facing to a
live original actor, and use in other match states.

Private evidence:
`W:\Daywater\research-temp\nflstreet-native-movement-player-20261008`.
Fixtures, checkpoint images, original probe logs, executables and hashes are
kept there. `prepare_live_movement.py` extracts NMS1 from bounded original
JSONL records, checking capture completion and original PCs. Build with the
existing `build_windows_live_player.sh` and a new W: output directory.

## Neutral investigation

The research-only `input_original_probe.c` executes original function
`0x8003DDEC` from the unchanged normal module on full existing MEM1 snapshots
`position-neutral-14.mem1`, `position-neutral-15.mem1` and
`position-neutral-16.mem1`. It supplies a scratch neutral vector, then separate
right/forward/diagonal controls, restoring the complete RAM snapshot between
calls. Original calls and all their callees run without stubs; this oracle is
never linked into the viewer. Its stack/vector locations are harness
allocations, not discovered game fields.

The already documented actor base `0x803C19C0` retains `Thomas` at `+0xB80`
in all three snapshots and after all 12 calls. Each call returns normally with
no CPU exception. On the neutral calls, both original branch predicates
`0x8006510C` and `0x80065290` return zero, and the complete 0x30-byte input block
remains unchanged. The cached original decompilation has exactly that early
return path. Control calls produce directional requested headings, showing
that the call is exercised and can modify the actor inputs.

This is **verified for these controlled original-module calls**, not verified
full-game neutral behavior. The snapshots already had zero requested strength;
that does not imply zero active velocity or identify the controlled player.
Stopping cannot be derived just by writing zero strength into the existing
positive-strength movement slice. It requires the original controller/state
selection and zero-motion paths, followed by a valid consecutive original
movement comparison. The prior actor-clearing live neutral-capture limitation
still applies to that separate saved-state test.

## Remaining milestone

Reconstruct neutral/backward/diagonal state paths, braking and zero-speed actor
creation, and original movement/animation selection. Then connect contacts,
other actors and ball behavior to this visible native runtime. A complete
standalone match, native startup, personalized appearances, camera behavior,
audio and menus remain unfinished.
