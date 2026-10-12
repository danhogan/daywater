# Native outgoing-channel fades and original frame-weight ordering

## Result — 2026-10-09

The experimental paired viewer now keeps the outgoing animation pair while
the new pair fades in. All four streams advance with checked source phase and
rates. The outgoing pair is retired when the bounded countdown completes;
new speed-selected clip requests wait during the fade.

Rendering now follows the original frame-weight list order: each active slot
contributes its next frame and then its previous frame, weighted by its stream
fraction. The list is normalized cumulatively before wrapped-short pose mixing.
This replaces the earlier preview's hierarchy of a time blend followed by a
two-channel blend, which was not a complete original list-assembly claim.

Private launcher: `W:\Daywater\Start-NFLStreet-PairedPlayer.cmd`.
The default movement launcher remains unchanged. All original files, builds,
captures and diagnostics stay on W:. The native viewer loads no original module,
Dolphin, ISO or saved RAM.

## What is reconstructed and what is still bounded

**Verified within controlled joined stages:** cloning/allocation through
original `0x8006475C`, its phase-preserving rebind and blend setup; countdown
arithmetic `0x80064AF4`; retiring weights/removal `0x80064B04`; active/retiring
rates `0x800648EC`; all four carried stream states; original `0x80176650`
numeric frame weights, frame IDs and slot order.

The native controller uses a four-slot host pool and the source callback's
default eight-update transition duration. Its upstream variant and primary
curve are fixed. It does not reconstruct every transition-duration choice,
full original actor callback eligibility, full-pool eviction, alternate curves,
body/velocity direction decisions or all animation groups. Initial channel
eligibility/metadata is supplied by the host; native actor startup remains open.

The original blend list is checked with controlled group weight1, direct source
bank metadata and no additional readiness exclusions. It does not establish all
original group weights, scatter/base-decoder choices or model remapping paths.
The existing exact compressed-frame decoder and checked wrapped-angle primitive
render that list; host cluster trigonometry and appearance/camera choices retain
their earlier limits. No full original gameplay pixel match is claimed.

## Evidence procedure and confidence

Game: NFL Street, GameCube GNNE69 revision0. DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`; source captures Dolphin2603a.
The oracle executes the untouched original module offline. It relocates the
same private disc bank in scratch RAM for a controlled registry bank2 input.
Disc bank00111 identification as the saved actor's bank remains **probable**;
complete resident-to-disc equivalence is not asserted.

Three saved contexts each run480 carried updates, including gradual changes,
abrupt signed-speed requests, rest, zero/fractional deltas and catch-up:

- 1,440 complete descriptor/curve comparisons and four carried channel-state
  comparisons match. Allocation status, all blend fields and all64 stream bytes
  are checked; other channel metadata bytes before+30 are excluded. Only program
  pointers are translated to native file offsets.
- 78 starts,585 fade updates and72 completed fades match;174 requests are held
  during active transitions. Three final fades remain in progress in this
  deliberately abrupt sweep and their remaining state matches original stages.
- 6,768 actual original pose-list entries match slot order, next/previous frame
  IDs and every normalized weight exactly.
- The phase-only oracle remains passing, including its full rebind caller and
  exact cycle-boundary controls. Invalid fade slots and nonfinite pose fractions
  reject transactionally without changing outputs.

Expected original results are only comparison targets. Native allocation,
countdown, clips, weights, periods, frame poses and retirement are computed from
carried state and source data, never injected from expected checkpoints.

| Field | Primitive / byte order | Observed values and original reader/writer |
| --- | --- | --- |
| Descriptor+0/+1 / +2/+3 | uint8 IDs | Active and retiring slots0..3; FF after retirement; `0x8006475C` / `0x80064B04` |
| Descriptor+4 | float32,4,BE | Fade factor1/8..1 in default controlled duration8; `0x80064AF4`, reader `0x800648EC` |
| Descriptor+8 | float32,4,BE | Stored outgoing first-channel weight, range0..1; installed by `0x8006475C`, reader `0x80064B04` |
| Descriptor+3A/+3B | uint8 counts, interpreted signed by original callback | Duration8, remaining8→0; scheduling branch `0x800635B8` |
| Channel+1 | uint8,1 | Active2, retired0; stride124; original allocator/clone and `0x801740A8` |
| Channel+30/+34/+38/+3C/+40 | BE float32/float32/uint32/float32/uint32 | New pair starts0/target1/mode1/initial0/duration8, then weights and mode0 installed; `0x801791A8`, `0x80179288`, `0x800648EC` |
| Pose-list capacity and normalized weight | Source count24; BE float32 at each44-byte row+4 | Up to8 entries for four channels; first weight1, subsequent rawWeight/cumulativeWeight; `0x80176650` |

These are verified numerical/structural roles within the controlled source
stages; they are not universal addresses or fresh-match semantic discoveries.

## Windows integration

The actual Windows viewer passes a60-update run preserving all eight recorded
movement checkpoints and a528-update run covering all eight directions,
releases, braking and held stops. Both render the four-channel fades and require
completed retirement with no outgoing slots left at the final held stop.
The default eight-checkpoint run remains passing, and all five inspection BMPs
remain byte-identical to the previous baseline.
A rendered movement frame was visually inspected for a coherent posed player;
this is a native diagnostic check, not original gameplay pixel equivalence.

Private evidence: `W:\Daywater\research-temp\nflstreet-retiring-fade-20261009`.
The verification manifest records results and unchanged original DLL/runner
hashes. Build with the existing live-player and locomotion-check scripts.

Next: derive changing upstream variants and original curve decisions, then
connect other-player/contact/ball scheduling to this visible native runtime.
Fresh actor/match initialization, a complete football match, menus and audio
remain unfinished.

The subsequent [native curve-choice join](native_curve_choice.md) connects original primary/alternate/other identity decisions and durations to paired playback. Upstream variant generation remains unfinished.
