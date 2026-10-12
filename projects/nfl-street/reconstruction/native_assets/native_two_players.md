# Two independent native player instances

## What works — 2026-10-09

The optional native Windows viewer now renders and moves two player instances
on the checked field/sky. Each owns a separate movement runtime, stream phase,
curve/pair choices, four animation channels and fades. Pose evaluation uses
shared immutable assets and sequential scratch geometry; no actor consumes the
other actor's output yet.

`W:\Daywater\Start-NFLStreet-TwoPlayers.cmd` opens this build:

- WASD/arrows control player 1.
- IJKL control player 2.
- V changes player 1's supported inspection animation set. Player 2 retains the
  initial supported set independently.
- R resets both instances to their private starting seeds; Esc closes.

Both use the same checked reference appearance and movement seed. The second
instance has a host placement offset of (1.5, 2) from the first spawn position.
This offset is an inspection choice, not recovered original team placement.
They can pass through one another: contact scheduling and ball gameplay are
not connected. No original roster identity is assigned to the duplicate.

The executable runs without Dolphin, ModernGekko, the original DLL, an ISO or a
RAM snapshot. Extracted assets and small private movement/appearance/camera
profiles are still required. Fresh actor/match initialization remains open.

## Scene connection

`nflstreet_field_layer` retains both color and depth after rendering an actor.
The second pose then draws onto that surface. It preserves field/actor and
actor/actor depth tests instead of independently overlaying each character on
the original background. Returned surfaces own allocations; source and output
must be distinct. The original one-player overlay path is unchanged.

The test independently constructs a combined mesh and draw list containing
both posed instances, using disjoint host draw identifiers. Rendering both in
one pass over the cached field must match sequential layers in **every color
and depth word**. These host draw identifiers are not original RAM addresses
or new game enum values.

## Evidence and confidence

Original game: NFL Street, GC GNNE69 revision 0; ISO SHA1
`813c4511ba3135059a13de424555647e8252254b`; DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`.

**Verified native integration:** actual Windows key-handler runs cover separate
controls, reset-to-seed, movement starts, release/braking/held stops, independent
animation state and retained scene depth. The 60-tick run preserves all eight
full original player-1 actor/movement checkpoints while player 2 follows
opposite input. Both stop and retire outgoing animation channels. The 528-tick
run sends opposite directions to the two players through all eight segments;
all 16 starts and 16 stops pass. Player 1 completes 36 fades and player 2
completes 34. V does not change player 2's set. Scene color/depth equivalence
passes 590 checks across both runs, including their initial frame. Player 2
shades visible samples in all 590 frames. Visual capture shows both posed
characters in the field.

**Reused original evidence:** the unchanged movement controller was previously
checked against original source stages. Its original verifier was rerun with
the untouched DLL in saved MEM1 contexts position-neutral-14/15/16 from Dolphin
2603a: all 192 complete directional movement states and 24 projection/rest
heading controls agree. This is an offline original-stage comparison, not a new
live Dolphin run or a complete original two-player gameplay comparison. The
animation decoder, phase transfer, curve and set joins retain the confidence
and limitations documented in the preceding milestones.

**Candidate broader fidelity:** original second-player identity, appearance,
team placement, automatic behavior, contact eligibility and ball ownership are
not established by duplicating this checked instance. Existing original paired
actor/contact reconstructions remain separate until joined to this scene.

Regression: the optional one-player set path and default movement checks pass.
All 12 one-player set-run BMPs and all five legacy inspection BMPs remain
byte-identical to the preceding build. No generic Daywater code changed.

Build: `build_windows_live_player.sh <private-output>`.
Add `--movement <fixture> --sets <bank> <catalog> --second-player` to the usual
viewer asset/profile arguments. `--set-check` performs the short integration;
`--direction-check` extends it to eight directions. Build, captures, logs and
hash manifest are private in
`W:\Daywater\research-temp\nflstreet-two-player-20261009`.

Original DLL SHA1 remains `dd521cc38b0db6bca98a0bb261570a2be5e6e0ed`;
baseline runner SHA256 remains
`3f9e3a3eb4b5728d9036e9552c01e8c9dc084a1f53805fef95dcf3b1b86f73ea`.
Neither was edited; no restoration was needed. Existing launchers retain their
previous checked builds; the two-player launcher is an additional inspection.

## Next connection

Join the reconstructed paired contact selector/application and original bank
programs to these visible instances. That requires original actor-pool/handle
facts and compatible callback/animation state, rather than inferring tackles
from visual proximity. Automatic animation context, original roster assembly,
ball launch/catch/bounce/rendering, fresh match startup, rules, menus and audio
remain unfinished.
