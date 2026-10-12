# NFL Street native Windows port plan

## Target and current baseline

The target is a Windows game that runs reconstructed NFL Street game logic
without a GameCube CPU runtime or a disc image at play time. It needs its own
game loop, input, rendering, audio, data-loading, and save boundaries. Game
data must be supplied by the local owner; keep the image and extracted data
outside Git.

The exact research target is GNNE69 revision 0, `main.dol` SHA-1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Do not generalize findings
to another revision without repeating the checks. [Status](STATUS.md) records
the current evidence. The [Windows recompilation](native/recomp.md) already
reaches a match and accepts movement input, but still uses ModernGekko/Dolphin
services and the original image. It is a behavior reference and an early
playable prototype, not the independent port.

## Division of work

| Area | Home | Purpose |
|---|---|---|
| GameCube binary and RAM research | Daywater | Automate Ghidra and Dolphin captures, identify functions and state, and retain reproducible evidence. Keep its tools generic. |
| Reviewed NFL Street behavior | `reconstruction/` | Portable, bounded source for functions and subsystems supported by binary and live-state comparisons. |
| Windows host and game runtime | `native/`, later a separate buildable project | Own the event loop and host interfaces. Replace researched game services one boundary at a time. |
| Whole-DOL static recompilation | Private W: output plus `native/` experiment notes | Keep a playable comparison target and, where useful, a temporary hybrid during migration. It is not the final runtime architecture. |

There are two useful techniques. Byte-matched PowerPC reconstruction checks
what source could emit specific original instructions. Host-native source
reconstruction checks behavior on captured inputs and outputs. A Windows port
needs the latter; byte matching is useful evidence but is not required for
every function. Generated whole-DOL C should remain separate from reviewed
source, and replacing small hot functions is not a measure of game completion.

## Completed slice: input to actor movement

The [native vector bridge](reconstruction/command_to_movement/input_vector_bridge.md)
now connects guarded cardinal input translation to the stateful
[actor runtime](reconstruction/command_to_movement/actor_runtime.h). The
Windows x64 replay matched every actor-input byte and all 21 movement words
on four three-update live sequences and a separate eight-update sequence.
The original translator returns were captured for the later calls of one
sequence. The longer trace exposed one rounding difference, which was fixed
by matching the original fused operation. The bridge still starts from a
saved actor and movement state and covers only the observed branch.
Four additional live three-update runs cover settled partial strength,
full-to-half, half-to-full, and a combined right-to-half-forward turn. The
Windows bridge matched complete actor and movement blocks in each. See the
[partial-strength evidence](reconstruction/command_to_movement/partial_strength.md).

The checks that established this slice were:

1. From one saved actor/movement state, feed right, forward, and left input
   vectors observed in live Dolphin traces. Compare the complete actor input
   and movement output blocks after each call, including fresh direction
   changes, with the original traces.
2. Feed a newly captured eight-update sequence. Its complete actor and
   movement blocks match the native replay on every call. The trace hashes,
   Dolphin build, state identity, and confidence are recorded in the bridge
   evidence.
3. Reject zero-stick, diagonal, and other unresearched paths explicitly until
   they have live evidence. A rejected step must leave the native state intact.
4. Build and run this slice on Windows x64 without ModernGekko, Dolphin, the
   ISO, or an extracted asset tree at replay time. The private trace inputs
   remain outside Git.

This proves one continuous piece of game behavior within the saved match.
It does not establish a playable independent game.

## Next research and implementation gates

1. Use the staged Dolphin **2603a** wrapper for the supplied savestate and
   confirm that the roster is present before interpreting RAM. The container
   default is **2609** and reached a frame without restoring that match.
   The [multi-actor probe](reconstruction/command_to_movement/actor_update_order.md)
   succeeded twice with the compatible build.
2. Obtain a state where the same actor survives several updates, then trace
   neutral input through the original translator, active-heading handoff,
   and strength writes. The tested nonzero partial-strength branch has
   complete live comparisons. In the current saved state, the actor is
   [cleared](reconstruction/command_to_movement/neutral_capture_limit.md)
   before the next comparable neutral update.
3. Trace where normalized controller input is produced and when the game
   schedules translation and actor updates. The
   [Thomas schedule](reconstruction/command_to_movement/update_schedule.md)
   now has eight consecutive calls with one dumped frame between updates;
   the [multi-actor preparation order](reconstruction/command_to_movement/actor_update_order.md)
   now repeats for nine actor pointers on two complete frames in two fresh
   runs. [Later captures](reconstruction/command_to_movement/actor_order_dynamic.md)
   show that eligibility changes across frames. The other actors' native
   movement outputs, input sampling, and
   game-service dependencies remain open.
4. The [second actor replay](reconstruction/command_to_movement/second_actor_replay.md)
   now matches slot 7's complete preparation, command, reset, and movement
   outputs across right and forward three-update runs. Its
   [target-position input path](reconstruction/actor_target_input/README.md)
   now computes the later actor blocks natively. Two live calls trace the
   upstream target-output handoff and a position offset from the passed
   Thomas actor. Identify how the other-actor pointer is chosen and
   determine actor eligibility before adding a native multi-actor scheduler.
   The observed composition
   now has a guarded C reconstruction checked against six original outputs;
   a two-actor replay also advances Stewart and Thomas across two
   three-frame runs. The Windows loop now uses that C function for Stewart's
   later targets. Selection of Thomas and general actor eligibility remain
   open.
   An integrated Windows executable now advances both actors in observed
   order and composes the inter-actor targets internally. It matches the
   right and forward three-frame saved-match traces.
   The next upstream trace verifies that the caller receives Thomas's pointer
   from a record-lookup call in six fresh cases. The original DOL and a
   private MEM1 snapshot verify that record handle `0x00010101` resolves
   to Thomas's slot 8; a bounded native C lookup now checks that choice in
   the paired replay. Six additional live calls now verify the record,
   handle argument, and returned actor pointer at each lookup boundary.
   The changing record index and other-state behavior are the next
   research boundary.
   A direct index store and two callers are present in the original DOL.
   Two saved-match watchpoint runs saw no write, so the next live test needs
   a transition that reaches that store before generalizing selection.
   Two first-down RAM series instead show the record handle changing while
   the index stays zero. The native decoder resolves those handles to three
   named roster actors; a live first-down choice call or writer capture is
   needed before treating that changing field as a proven selection policy.
   Two live first-down Stewart choice calls now verify the final Thomas
   handle reaches the target path after the snap. Earlier Urlacher and
   Stewart handle values still lack a live choice-call capture; the handle
   writer is the next trace target.
   The [snapshot-start replay](reconstruction/command_to_movement/snapshot_start.md)
   now takes one exact-boundary RAM image and runs the two-actor Windows
   slice for three verified frames without trace-supplied initial actor
   blocks. The remaining native-startup problem is to create a valid
   match state without Dolphin and then expand beyond this bounded loop.

## Milestones after the slice

The [record-handoff milestone](reconstruction/actor_target_input/record_handoff.md)
now closes the live copy-writer question for the documented first-down
transition. Windows C finds the record for the current actor and reproduces
Urlacher-to-Stewart-to-Thomas from captured handoff events in two fresh
runs. It does not generate those events or implement the wrapper's other
side effects. Event production and incoming-actor selection are the next
upstream boundary before integrating this into a wider native scheduler.
The newer [descriptor-to-event replay](reconstruction/actor_target_input/descriptor_event.md)
now calculates the incoming actor from its descriptor rather than taking
the expected incoming handle at each handoff. It matches three event builds
and two handoffs in each of two fresh runs. Descriptor production and
event-stage timing still come from the original game, so those are the next
dependencies before this can schedule its own state changes.
The [readiness-signal replay](reconstruction/actor_target_input/event_signals.md)
now verifies the command stores producing advance/ready bits and runs a
native stage-three poll that decides to wait or reassign the record. It
matches both wait/ready pairs in two fresh runs, including conditional old
actor flag cleanup. Command emission timing, earlier stages and other
transfer effects still need reconstruction before a native scheduler can
produce these inputs independently.
The [command-timing replay](reconstruction/actor_target_input/command_timing.md)
now emits the observed readiness commands from its countdown and program
cursor, matching 30 complete updates in two fresh runs. It carries six
consecutive updates per stream without reloading state. Program selection,
initialization/replacement, earlier event stages and other callback families
remain outside that timing replay. The newer
[stream setup and early-event slice](reconstruction/actor_target_input/stream_setup_early_events.md)
now handles supplied-table lookup, interpreter initialization and observed
early decisions/steering. It derives a replacement stream key from an already
selected paired-animation asset. The newer
[contact component](reconstruction/actor_target_input/contact_selection.md)
now scans all supplied candidates and derives the selected pair natively,
then performs verified stage/flags/link/request writes. The newer
[animation component](reconstruction/actor_target_input/animation_channels.md)
predicts channel and manager slots, starts the supplied clip, prepares its
group and reproduces blend helper state. Two fresh runs pass 26 complete
block comparisons. The subsequent
[callback/transition replay](reconstruction/actor_target_input/callback_metadata_transitions.md)
now reproduces both complete actor objects and the selection manager for
the tested paired contact in two fresh runs, including native metadata,
negative transition searches, default retirement and anonymous registry
selection. The [asset-loading/spawning extension](reconstruction/actor_target_input/asset_loading_transition_spawning.md)
now derives source tables from twelve disc bank blocks and starts two different
transition clips, matching six full actor checkpoints per controlled experiment
and both original durations in Windows C. This is bounded metadata/program
loading; initial registry construction, TERF manifest traversal, frame decoding,
wider descriptors and native startup remain open.
Join the bounded contact application with timing, event and movement updates
into a longer native segment.

1. **Deterministic simulation harness.** Drive the verified actor update at
   a measured cadence with a recorded input stream. Compare checkpoints with
   fresh Dolphin runs. Expand beyond one actor only as the actual update
   order and inter-actor dependencies become known.
2. **Game-service inventory.** Trace calls from the entry path and active
   match loop into disc I/O, memory management, timing, graphics, audio,
   controller, and save services. For each service record observed arguments,
   outputs, ownership, and ordering. Pick a thin native interface based on
   those observations.
3. **Windows vertical slice.** Boot into a small, fixed match state using
   locally extracted owner data. Show a frame, accept controller input,
   advance one play, update a verified score, and play its corresponding
   sound through native host services. Define a reproducible reference run
   for each step.
4. **Broader game coverage.** Replace menu, match, rules, teams, animation,
   rendering, audio, persistence, and asset loading paths. Track uncovered
   branches and regressions by game state, not by an estimated percentage of
   decompiled instructions.
5. **Independence check.** Run a fresh Windows build with no ModernGekko or
   Dolphin libraries and no ISO mounted at play time. Verify navigation,
   a complete match, scoring, sound, saving/loading, and exit/restart against
   the reference on the researched revision.

At every stage, record `candidate`, `probable`, or `verified` as defined in
the repository instructions. A passing bounded replay supports only its
tested inputs and state. Keep proprietary images, extracted assets,
savestates, generated binary translations, and builds in private storage.
