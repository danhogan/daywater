# Native idle stream and accepted C00C callback

The independent movement viewer now runs both chosen cycles through the
reconstructed original command-stream setup and timing driver. Idle cycle 18
no longer uses the inspection animation clock in native movement mode. Run
cycle 575 keeps the preceding original timing integration. Inspection mode
remains unchanged.

Original actor callback `0x8001532C` handles accepted command C00C by setting
actor +0x0C flag bit `0x40000` and returning 1. This requires no argument words
or other service effects on the accepted route. The bit's gameplay meaning
remains unknown. `event_signals.c` now supports this effect alongside C004 and
C006. It is an accepted-command handler; callback context routing/eligibility
is not implemented by this API.

Native viewer animation flags persist across idle/run stream switches and
reset with R or M. They are separate owned animation context state, initially
zero; they are not a reconstructed fresh game actor's flag initialization.
The visible run/idle choice, instantaneous cycle switch and mapping of one
stream unit per 60 Hz host update remain inspection decisions. This work does
not reconstruct original clip selection, transition blends or root travel.

## Original comparison and confidence

Game: NFL Street, GameCube GNNE69 revision 0. DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Source RAM captures: Dolphin 2603a.
Comparison uses the unchanged original module offline on saved MEM1 snapshots;
no new live Dolphin integration is claimed.

`stream_original_check.c` executes original `0x8001532C` directly with accepted
wildcard group context and flag words 0, FFFFFFFF, 10004600 and ABCDEF01. All
0x14D4 actor bytes match the native flag effect, including every untouched byte.
It then clears bit 0x40000 so stream playback must actually set it. Original
callback routing is controlled using stack index 0 at r13-0x579C, actor identity
at `0x8032BF10`, and wildcard group FFFF at `0x8032BF1C`; callback pointer at
r13-0x6734 remains original `0x8001532C`. This is a controlled accepted route,
not evidence for every original callback eligibility branch.

For three saved snapshots, both real common-bank programs 18 and 575 are set
up and advanced by original `0x8017724C` / `0x80177374`. Six runs match all
1,200 complete 64-byte stream states and complete actor blocks. Each idle run
emits 19 C00C commands, including repeated loops; native flag words match every
step. Twelve direct flag-pattern controls also match. **Verified** applies to
these accepted effects and stream mechanics; no gameplay name is assigned to
the flag and no original selection claim is made for either chosen cycle.

Nine Windows rendering checks reproduce all previous 20 moving and 192 braking
checkpoints. An additional 528-update Windows check exercises all eight
start/release/stop directions through WM_KEYDOWN/WM_KEYUP and idle/run switches.
All five inspection BMPs remain byte-identical to the prior baseline. The
native viewer uses no Dolphin or original module while running.

All private assets, snapshots, programs, module comparisons, Windows builds,
images, logs and hashes stay in
`W:\Daywater\research-temp\nflstreet-idle-stream-20261009`. Baseline DLL and
runner source hashes remain unchanged. Build with the existing
`build_windows_live_player.sh` and `build_windows_direction_checks.sh`.

## Original selection boundary to continue

Static original code now identifies a concrete chain to replace the chosen
cycle switch. This chain has **candidate** runtime confidence until captured
or compared in the required movement context:

- `0x80014CD0` selects a requested group and calls group preparation
  `0x80177EF4` when the current group differs.
- Its normal helper `0x80014898` derives groups 4A/49/A0/9D/9F from requested
  headings, strength and original team/speed services. These are raw group
  IDs, not names or common-bank clip indices.
- Group49 callback `0x80066490` uses direction/speed/context bytes at actor
  +0x3F0..+0x3F2 and descriptor matching `0x800211D8` to select bank/clip keys
  before original channel startup `0x80173FCC` / `0x8017413C`.
- `0x800211D8` compares four metadata bytes, scores exact matches with weights
  10/9/8/7 and preserves the first row on a tie. Native contact callbacks
  currently cover different descriptors; locomotion callback/data resolution
  must be added rather than assuming the chosen common-bank cycle is correct.

Resolve original source descriptor rows and bank identities first, join them
to existing native allocation/asset loading, then render the selected channel
and original blends. Continue other-player/contact scheduling and ball
launch/catch/bounce integration alongside that work.

The subsequent [source-backed locomotion selection](../actor_target_input/locomotion_selection.md)
resolves the original descriptor hierarchy and speed-dependent adjacent clip
indices. It also identifies the different registered normal group4A callback
and validates the probable source bank's compressed frames. Viewer integration
of original paired-channel weights/rates to the viewer remains unfinished;
the bounded native arithmetic now matches 972 original controlled calls.
