# Source-driven paired animation in the native viewer

## Result — 2026-10-09

The initial build described below used immediate pair restarts. The later
[phase-rebind milestone](native_phase_rebind.md) now preserves checked original
cycle duration and elapsed position in the experimental launcher. Original
retiring-channel crossfades and upstream variants remain unfinished.

The later [retiring-channel milestone](native_retiring_fade.md) connects the
bounded four-channel fade and original numeric frame-list ordering as well.
The initial two-channel build's limits below are retained as historical evidence.

The independent Windows viewer has an optional paired playback path. It joins
the checked original source-curve selector, paired weight/rate updates, stream
setup and timing, compressed pose decoding, wrapped-angle interpolation and
the cached player renderer. Its clip IDs and rate come from source data and
current movement speed; they are not the earlier chosen idle18/run575 switch.

This is a bounded connection into visible native movement, not the complete
original normal-locomotion callback. The upstream variant keys and primary curve
are fixed to the measured source context. Two host-owned channel slots restart
immediately when the selected pair changes. Original channel allocation,
phase-preserving transitions, alternate-curve decisions, root travel and
body/velocity sign selection remain unfinished. At rest the primary curve is
still evaluated; this does not establish the original idle-state decision.

The existing movement launcher retains the passing idle/run baseline. A separate
private `W:\Daywater\Start-NFLStreet-PairedPlayer.cmd` opens this new path.
No Dolphin, original module, ISO or saved RAM is loaded by the viewer. It reads
extracted assets, the existing private appearance/camera and movement seed
profiles, and a small NLC1 source-curve export. Fresh actor/match initialization
is still required for a complete game.

## Original evidence and confidence

Game: NFL Street, GameCube GNNE69 revision0. DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Source RAM was captured with
Dolphin2603a. This milestone executes the unchanged original module offline;
it does not claim new live gameplay comparisons.

**Verified within controlled joined stages:** three saved contexts each pass
480 carried updates, totaling1,440. Every update compares the entire curve and
both complete124-byte channel records after original selection `0x80064638`,
setup `0x8017724C`, blend/rate `0x800648EC` and timing `0x80177374`. Only the
program identity word is translated between original emulated addresses and
native file offsets for comparison. All other bytes must match.

The controlled sweep includes negative/positive speed, zero/rest, abrupt pair
changes, zero delta, fractional deltas and catch-up. Across three contexts,
396 pair starts and792 actual original `0x8017756C` tag-count scans match the
native loop-only counter. Programs containing callbacks, arguments or other
controls are rejected; none are silently skipped. Expected original state is
used only by this oracle, never to advance native playback.

**Verified interpolation primitive:** 1,440 original `0x80166730` comparisons
match every output angle. Inputs include decoded poses, extreme signed-short
differences and fixed-point weights across0..4096. This verifies the primitive
used for time interpolation and cross-channel mixing. It does not prove the
complete original model-channel ordering or descriptor assembly in a live match.
Both selected channels have total weight1 in this bounded primary-curve path;
the second weight is converted to a12-bit blend weight.

**Probable source-bank identity:** the measured resident bank2 header uniquely
identifies unpacked `animdata/00111.bin` by type/tag/frame count. Its4,641 frames
were already verified against the original decoder. Complete resident-to-disc
mapping and arbitrary bank compatibility remain open. The adapter explicitly
checks this measured signature and refuses other banks.

**Windows integration exercised:** one60-update run preserves all eight
recorded actor/movement checkpoints while rendering paired poses. Another
528-update run exercises all eight key directions, releases, braking and held
stops. It performs35 pair starts and529 rendered playback evaluations including
initialization. Both runs return success. Moving and stopped diagnostic frames
were inspected for a coherent posed player over the field; they are not original
gameplay pixel comparisons.

The default viewer still passes its recorded moving/braking checks; all five
inspection BMPs remain byte-identical to the previous baseline. Private logs
and manifests give exact run results and baseline hashes.

## Native interface and bounds

`live_locomotion.c` owns its curve and channel state. `open` accepts supplied
bank bytes plus the NLC1 export, and `step` accepts signed speed and stream delta.
It derives pairs, weights, rates and frame poses itself. Unsupported inputs fail
transactionally; reset restores source curves and initial host channel weights.
NaN rollback and reset are checked against all state/output bytes.

The viewer passes movement speed from its existing semantic adapter. Because
the displayed body currently faces velocity, it supplies nonnegative speed;
the original actor heading/velocity angle threshold in `0x80062748` has not been
connected. Negative-speed selection is covered by the controlled oracle,
not by a claim of original backward-facing body animation in the window.
One stream unit per60Hz host update remains the existing host time mapping.

Build with `build_windows_live_player.sh`. Enable the optional path with
`--locomotion private-bank private-source-curves` plus `--movement`.
Build the joined original oracle with `build_windows_locomotion_selection.sh`.
All private files, builds, original comparisons and diagnostic images remain in
`W:\Daywater\research-temp\nflstreet-paired-playback-20261009`.
Neither baseline DLL nor original runner source is modified.

Next: replace provisional pair restart with original phase/transition handling,
derive changing upstream variant keys, and connect other-player/contact/ball
scheduling to the visible native runtime.
