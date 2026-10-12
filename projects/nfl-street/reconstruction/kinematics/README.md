# Native Thomas kinematics replay

`kinematics_replay.c` is a human-written C reconstruction of the moving
branch through GNNE69 revision 0 function `0x80036B28`, including its
direction polynomial, acceleration, speed, angle, and three-component
position update. It consumes a `0x54`-byte game-state block as hexadecimal
big-endian words and emits the updated block. It runs without Dolphin,
ModernGekko, or a game image at replay time. The executable is a replay tool,
not an NFL Street runtime or a playable game.

The original DOL SHA-1 is
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. C constants in this
implementation were checked against that DOL. The eight sine-polynomial
coefficients are initialized into RAM by the game's `0x801888F0`; their
bits were checked in the supplied second-down MEM1 image. The
[boundary notes](../kinematics_boundary.md) document the executed caller
and the complete static direct-call closure.

## Verification

Eleven distinct fresh Dolphin 2603a calls from the supplied GNNE69 second-down
state were captured before and after `0x80036B28`. Four used left, right,
forward, and backward input after a two-second delay; two more used right
and forward input after a five-second delay. Three more followed the live
[stick-to-movement path](../input_to_movement.md). Two further calls followed
newly changed right and forward headings. The Linux C build and actual
Windows x64 executable both matched **all 21 output words bit for bit**
against every live result, including the position, displacement, speed,
angle, and copied prior-state words. The two later traces have W: SHA-256
values `724cdbad5000f2809b592237a67c809da2078c33240f05f42f3b8d62c220f173`
and `635408c4f3d09a6f32c018546edfa3dfd4033c5f3206de8bf7506e28c5efa23e`.
The first four trace hashes are in the boundary notes; the stick-to-movement
path records the five input and heading traces. All raw traces remain private
under `W:\Daywater\research-temp\`.
An additional eight-call direction-change run exposed a one-bit speed
difference on one call. The DOL length helper `0x801DDFCC` uses a fused
`fmadds` after its `y*y` multiply. Matching that operation made the rebuilt
Windows replay match all 21 words on all eight calls. See the
[input-vector bridge](../command_to_movement/input_vector_bridge.md).

Build the Windows replay to W: with
`bash projects/nfl-street/reconstruction/build_windows_kinematics_replay.sh`.
The verified executable at
`W:\Daywater\research-temp\nflstreet-kinematics-replay.exe` has SHA-256
`4643d2265e32952b58597851b9f376eec2e3d9245da7b92ffcaf5eee6f1715b4`.
For an offline Linux build:

```bash
cc -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off \
  projects/nfl-street/reconstruction/kinematics/kinematics_replay.c \
  -lm -o /tmp/nflstreet-kinematics-replay
```

Compare a private trace with `verify_capture.py` and that executable. Pass
`--windows` and a `W:\...exe` path to run the actual Windows build via
`cmd.exe` from WSL.

**Confidence:** `verified` for the eleven earlier live input/output transitions,
the eight-call direction sequence, and three calls in each of the three new
[strength-transition sequences](../command_to_movement/partial_strength.md).
The code deliberately rejects untested zero-acceleration, airborne, and
near-zero-motion branches. It covers the observed speed-cap branch and is
used by the guarded native actor update; other actor states, game flow,
assets, rendering, audio, and Windows controller input are still absent.
The match is evidence for this isolated movement calculation, not evidence
that the game runs independently.
