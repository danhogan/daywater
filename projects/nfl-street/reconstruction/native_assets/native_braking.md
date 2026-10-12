# Neutral release and native braking

The reconstructed movement viewer now advances the original grounded braking
branch when movement keys are released. The actor decelerates, reaches zero
speed, and stays in position. The preview's former neutral pause is removed.
The existing full-direction inspection mode remains unchanged.

Use `W:\Daywater\Start-NFLStreet-MovementPlayer.cmd` (updated to the braking
build). W/up, A/left and D/right move in reconstructed mode. Release those
keys to brake; R resets; M switches to inspection controls; Escape closes.
Backward/diagonal input remains unsupported in reconstructed mode. A
same-direction restart after stopping passes a native lifecycle check;
arbitrary rest-to-direction transitions remain guarded and need broader
original comparisons.

## Original path and confidence

Reference: NFL Street GameCube GNNE69 revision 0, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`, ISO SHA1
`813c4511ba3135059a13de424555647e8252254b`. Existing full MEM1 snapshots came
from the Dolphin 2603a research environment. No new live Dolphin capture was
made for this milestone.

The previous input-translator probes did not establish braking: neutral
translation can return without changing actor inputs. The actual braking path
is the original actor update dispatch:

1. `0x800F86BC` calls reset `0x80036AF8`, clearing movement acceleration and
   the subtraction field and restoring the decrement at movement `+0x3C`.
2. Its table at original DOL `0x80254500` maps status byte actor `+0x21E`
   value 0 to `0x800F7C3C`. This table entry is read directly from the hashed
   original DOL, not inferred from a guessed enum meaning.
3. That status skips `0x800F7C90` acceleration setup. Handler `0x800F7C3C`
   stores zero at requested strength `+0x200` and acceleration `+0x1DC`.
   The previous maximum speed and active strength are retained.
4. The grounded call runs `0x80036B28`. With zero acceleration, it clamps
   the decrement to current speed, subtracts it, caps the result at maximum,
   preserves the velocity heading, generates displacement and updates position.
5. The actor caller clears transient bytes `+0x20E` and `+0x21E` afterwards.

`nflstreet_actor_runtime_brake` reconstructs those bounded operations.
`nflstreet_live_movement_step` maps keyboard neutral to this explicit zero-status
branch and exposes the resulting native motion. All raw layout work remains
inside the separate NFL Street reconstruction package. Daywater generic
platform code is not changed.

**Verified:** these operations and resulting full state blocks match original
module execution as described below. **Candidate:** universal use of this
zero-status branch across every gameplay/controller context. The viewer's
keyboard-to-status choice is a bounded semantic executor; it does not
reconstruct all earlier game services or special stopping states. Host 60 Hz
scheduling, appearance selection and run/idle animation still use the previous
inspection choices.

## Original comparisons

The research-only `braking_original_check.c` loads the unchanged normal module
and a full existing MEM1 snapshot in a separate process. It executes the full
original actor function `0x800F86BC`, including all its callees, with no stubs.
Native code carries its own state from the same initial block. After every
update it compares all 48 actor-input bytes and all 84 movement-state bytes.
The oracle is never linked into the native game viewer.

| Source | Procedure | Exact checkpoints | First zero-speed update |
|---|---|---:|---:|
| `position-neutral-14.mem1` | Naturally zero requested/active strength and acceleration | 48 | 26 |
| `position-neutral-15.mem1` | Same observed branch, later existing snapshot | 48 | 23 |
| `position-neutral-16.mem1` | Same observed branch, later existing snapshot | 48 | 19 |
| Controlled moving release | Recorded moving seed in original full context; select original status 0, retain positive active strength | 48 | 23 |

All **192 complete actor and movement checkpoints match exactly**, including
held zero speed. The original actor identity `Thomas` is checked at the
previously documented base `0x803C19C0`, name bytes `+0xB80`. The three natural
snapshots are existing saved observations, not three fresh live sessions.
The fourth case explicitly supplies the captured moving block and zero-status
selection; it establishes that controlled dispatch, not an unmodified full-game
controller release.

Six additional controlled direct original movement calls match all 21 words:
decrement larger than speed, already stopped, zero maximum with positive
force, quarter-turn heading, speed above maximum, and motion below the original
small-value threshold. The original decrease constant is `0x3B6BEDFA`; no
invented host damping coefficient is used.

## Windows and regression checks

Four real Windows window runs render all 192 original braking checkpoints,
comparing full actor and movement blocks through the same semantic neutral
path used by the viewer. Each also continues to update 60 and confirms zero
velocity. Another window run uses actual key-down/key-up messages for the
recorded moving sequence, then releases into braking and reaches a stop.
Physical human-key testing remains pending.

All 20 previous recorded acceleration/turning checkpoints still match through
five Windows window runs. The inspection-mode five BMPs remain byte-identical.
Native lifecycle checks cover holding a stop, same-direction restart,
unsupported/airborne rollback, reset and truncated fixture rejection.

Build the native viewer with `build_windows_live_player.sh` and the research
oracles/checks with `build_windows_braking_checks.sh`. Private NBS1 fixtures use
the same 152-byte seed header as NMS1, followed by complete original actor and
movement states (132 bytes each). Expected outputs are used only for checks;
the viewer computes every update from its owned native state.

Assets, module-based experiments, captured-state fixtures, images, builds,
logs and verification hashes remain private at
`W:\Daywater\research-temp\nflstreet-braking-20261008`. Baseline runner source
and original DLL hashes remain unchanged. The native viewer uses neither
Dolphin nor the original module at runtime; it still needs the small captured
movement seed and static selection profiles.

## Next

1. Make rest-to-movement transitions reliable in every direction, then add
   backward/diagonal input with original comparisons.
2. Replace chosen run/idle playback with original animation selection,
   transitions and root travel tied to movement.
3. Join other players, contact responses and ball behavior, followed by fresh
   native match initialization and rules.

This completes the bounded neutral/braking viewer milestone, not a complete
football match or full original controller pipeline.
