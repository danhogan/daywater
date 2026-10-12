# Bounded native airborne update — 2026-10-09

## What works

The independent Windows scene now has optional `--airborne-inspection`.
B cycles ownership between the two players, releases the football into the
checked detached update, and reassigns it. The actual GameBall mesh moves
independently after release instead of remaining fixed at the last wrist pose.
Current attachment velocity is retained. Movement, animation/set selection and
paired contact inspection continue while it is detached.

This is a **supplied airborne inspection**, not a reconstructed pass, snap,
catch, bounce or full original ball simulation. The host supplies force
`(0,0,-9)`, target plane 0, positive marker+9C, zero wind and no-spin context.
The force is an inspection input, **not an identified original gravity
constant**. The existing host delta is1/60 for checks. It pauses after crossing
plane 0, retaining that last pose; this is a host stop, not original ground
collision. Original state/event selection and force/marker producers remain open.

The existing fixed-release ownership mode and launchers are preserved. A new
private `play-airborne-inspection.cmd` lives under W:
`Daywater/research-temp/nflstreet-ball-flight-20261009`. It needs locally
supplied assets/profiles; no Dolphin, original DLL or ISO is used while running.

## Reconstruction scope

`ball_flight.c` reconstructs **complete original `800C0684` in a bounded
airborne branch**. It accepts an owned 244-byte record, delta and supplied
float32 target plane. It rejects association, nonpositive/nonfinite+9C,
nonzero wind+E0, nonzero mode+D0, nonzero angular input+88 or nonidentity
delta quaternion+60. Thus ground-rest, wind, rotating/spinning and collision
branches are not silently skipped for unsupported inputs. Delta is restricted
to `[0,28)`; nonfinite/truncated/unsupported inputs preserve the whole record.

The source update:

1. Multiplies force+7C by source float word`3B5F4ED0`, writing acceleration+70.
2. Adds acceleration to velocity+54, then integrates position+C with the
   original single-precision fused multiply-add using caller delta.
3. Applies `800C04C0`: XY limits±42/±75, Z upper limit30; out-of-bounds XY
   zero the corresponding velocity before clamping position. There is no
   invented ground clamp.
4. Executes the zero-angular airborne branch of `800C0088`. It still performs
   the original delta-quaternion multiplication and quaternion normalization;
   no-spin does not mean skipping orientation writes.
5. Runs `800C1300` and the existing checked plane sampler for positive+9C,
   storing time+44, XY+48 and target+50.

`nflstreet_ball_no_spin` reuses original polynomial/quaternion math. A first
comparison exposed one-bit errors: `800C0018` normalizes its zero-axis
quaternion before multiplying the carried delta. Including that normalization
restored exact complete-record agreement.

This is the detached service, not the complete `800C0FE4` caller, which also
copies history, resolves association, resets force/angular inputs and updates
an external visual service. Original force accumulation and caller scheduling
are not reconstructed here. The native host poses the existing mesh directly
from the updated world position/quaternion.

## Evidence

Game NFL Street, GameCube GNNE69 revision0. DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Source MEM1 snapshots
position-neutral14/15/16 were captured with Dolphin2603a. Confidence:
**verified complete detached-update effects within the stated branch**.
Controlled original-module comparisons are separate from native Windows
integration; neither establishes fresh full-match gameplay fidelity.

| Field | Type / size / endian | Original readers/writers and observed tests |
|---|---|---|
| Force+7C | float32[3] /12 /BE | `800BFA1C` accumulates; `800C0684` scales into+70. Controlled `(0.25,-0.5,-9)`; producer policy unverified |
| Acceleration+70 | float32[3] /12 /BE | `801DE18C` writes source-scaled force; `801DE584` adds to+54; plane sampler reads first two components |
| Velocity+54 | float32[3] /12 /BE | Add/integrate, boundary zeroing; controlled positive/negative XY/Z including repeated updates |
| Position+C | float32[3] /12 /BE | `801DE278` integrates, `800C04C0` clamps; controlled near±42/±75 and30, retained unchanged history fields |
| Marker+9C | float32 /4 /BE | `800BFC38` tests≤0, `800C1558` tests>0; supplied 1. Meaning/upstream producer remain candidate |
| Wind control+E0 | float32 /4 /BE | `800BFCB0` conditionally adds forces; supplied0 bypasses that branch; wind policy unverified |
| Rotation mode+D0 | uint32 /4 /BE | `800C0088` selects rotation branch; supplied0. Other numeric modes remain opaque |
| Angular input+88 / delta+60 | float32[3]/12 and float32[4]/16 /BE | Zero angular vector and identity quaternion select the reconstructed no-spin branch; output+60 and orientation+18 are compared |

`native_assets/ball_flight_original_check.c` executes the untouched complete
original `800C0684`. It checks the source constants, eight controlled scenarios
of 64 consecutive updates per snapshot, positive/negative boundary crossings,
nonunit input orientation, supplied forces, two target planes and deltas0,
1/60,0.5,1. **All 1,536 complete 244-byte records match exactly.** Nine atomic
rejection checks cover unsupported context, association, angular input,
nonfinite force/delta, truncated records and a target not representable as
float32. Build through `build_windows_contact_player.sh`.

## Actual Windows integration

| Run | Result |
|---|---|
| Eight directions/set changes |528 ticks, all direction checks and four set changes pass;121 detached updates all move;39 host plane pauses;529 ball layers,16,158 shaded samples |
| Paired contact |200 ticks, two contacts complete;64 detached updates all move;201 ball layers,7,007 shaded samples |
| Existing ownership mode |528 ticks passes; all 12 ownership BMPs remain byte identical to its preceding build |

Each airborne run also completes six assignments and two releases. Original
launch/catch eligibility, spin/wind, ground bounce/collision, natural tackles,
fresh team/roster/match startup and root/history scheduling remain unfinished.
Private builds, logs, commands, profiles, captures and final hashes stay on W:;
the original DLL and runner source remain untouched and hash checked.

Three saved direction checkpoints differ from frozen native ownership by
53,24 and66 pixels, respectively; other snapshots are unchanged or occluded.
This is a native visible-motion comparison, not original pixel equivalence.
