# Native restarts, eight directions and original run-stream timing

## What works

The independent Windows viewer now accepts all eight WASD/arrow directions in
reconstructed movement mode. Releasing keys brakes to a held stop, and a new
direction can restart movement. The launcher remains
`W:\Daywater\Start-NFLStreet-MovementPlayer.cmd`.

The rest-heading fix follows original `0x800F6FA4`: when speed or maximum speed
is zero, it turns toward the requested heading without querying animation
state. Previously the native guard incorrectly required animation-dependent
alternatives to agree even at rest. Finite normalized diagonal vectors now use
the original vector-to-angle and magnitude projection; backward is accepted.
The earlier cardinal-only API retains its cardinal-only contract.

The selected common-bank cycle 575 now uses reconstructed stream setup,
countdown, strict-negative tag advancement, catch-up and looping, then the
existing wrapped-short fixed-point pose interpolation. Original clip selection
is **not** reconstructed by this change. Cycle 575 remains a candidate run-like
inspection choice. One stream unit per 60 Hz host update is a host mapping,
not verified original wall-clock timing. Rest cycle 18 previously retained inspection playback because C00C was
unsupported. The subsequent [idle stream milestone](native_idle_stream.md)
reconstructs the accepted C00C effect and now drives idle with original timing.
The chosen run/idle switch is not an original transition or root-motion system.

## Evidence and confidence

Game: NFL Street, GameCube GNNE69 revision 0. DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Reference saved RAM comes from the
previous Dolphin 2603a captures. This work executes the unchanged original
ModernGekko module as an offline comparison oracle; it does not claim a new
live Dolphin integration run.

- **Verified within controlled normal context:** eight full 48-byte input
  projections and eight rest-heading results per snapshot. Three saved
  snapshots give 24 of each comparison.
- **Verified within controlled stages:** eight directions, eight updates each,
  from a stop reached by 48 full original caller updates. All 12 actor words
  and 21 movement words match per update: 192 complete movement checkpoints
  across three snapshots. The oracle supplies the observed requested-to-active
  handoff and strength cap, then executes original reset, acceleration setup,
  preparation/command and kinematics. These are controlled stage comparisons,
  not evidence for every branch of the full original actor/controller caller.
- **Verified stream mechanics:** actual bank 00103 program 575 setup and 200
  complete 64-byte stream states match original `0x8017724C` / `0x80177374` on
  each snapshot. Includes exact countdown boundaries, fractional deltas,
  catch-up, and repeated loops. No callbacks are skipped on this program.
- **Windows integration exercised:** eight 60-update direction segments after
  an initial stop, using WM_KEYDOWN/WM_KEYUP and alternating WASD/arrow inputs.
  Every segment moves, releases, brakes and holds zero velocity. Native
  lifecycle checks also cover reset, invalid direction and airborne rollback.
- All earlier 20 recorded moving checkpoints and 192 braking checkpoints pass
  through nine Windows viewer runs. All five inspection-mode BMPs remain
  byte-identical to the previous braking build.

The runtime still starts from a private captured movement seed. It has no
Dolphin or original-module runtime dependency. Native actor creation, arbitrary
animation-dependent movement branches, personalized players, contacts, ball
rendering and full match startup remain unfinished.

## Reproduction and next work

Build the viewer with `build_windows_live_player.sh`. Build comparison tools
with `build_windows_direction_checks.sh`. `live-player --direction-check`
requires the existing `--movement` NMS1 fixture and the usual asset/profile
arguments. Expected checkpoint data is used for checks, never native updates.

All assets, snapshots, generated code, builds, images and logs stay private in
`W:\Daywater\research-temp\nflstreet-directions-20261009`; verification JSON
records identities and hashes. Neither original DLL nor baseline runner source
was modified.

Continue the four priorities with original animation selection and original transition blending, then joins between the already reconstructed paired actor,
contact completion and animation asset systems. The new native ball observation,
anchor and flight-plane sampling code is documented in `../ball/README.md`.

The later [paired playback milestone](native_paired_playback.md) connects
source-selected clip pairs, weights, rates and decoded poses to an optional
Windows viewer path. All eight native movement directions pass with it enabled.
The later [phase transfer](native_phase_rebind.md) follows the original rebind
caller. The subsequent [bounded retiring fade](native_retiring_fade.md) carries
four channels through checked fades and original frame-weight ordering.
Upstream variants and full gameplay transition choices remain unfinished.

The subsequent [native curve-choice join](native_curve_choice.md) connects original primary/alternate/other identity decisions and durations to paired playback. Upstream variant generation remains unfinished.

The subsequent [directional playback join](native_directional_playback.md) connects the original distinct heading thresholds for clip selection and blending to this native controller. Full gameplay callback eligibility and changing upstream variants remain unfinished.

The later [supported set playback](native_set_playback.md) connects two checked
source sets to visible V-key inspection changes, retaining the direction and
stop checks. Full automatic gameplay eligibility remains unfinished.

The later [two-player native scene](native_two_players.md) gives two reference
instances independent movement/animation state and shared scene depth. Original
roster identities, contacts and ball gameplay are not yet connected.

The later [source-backed contact inspection](native_contact_playback.md) connects
native selection/application and finite original streams to visible player poses.
Natural contact gameplay and the ball/startup milestones remain unfinished.
