# Source-backed contact inspection in the native scene

## What works

The two-player Windows viewer now accepts **C** to run one checked contact
context. It calculates contact parameters and candidate/RNG selection, builds
both requests, and performs the existing complete native paired actor and
animation application. No chosen contact, request, slot, metadata row or output
actor block is supplied to that application. It then advances the two selected
finite streams, executes their supported flag callbacks, decodes the original
poses and draws both participants. Another C while active is ignored; the
contact can be repeated after both streams end. Movement is paused during this
inspection and resumes afterward.

Launch `W:\Daywater\Start-NFLStreet-ContactPlayer.cmd`. WASD/arrows and IJKL
retain independent controls; C starts the inspection, R resets, Esc closes.
Both characters retain the shared checked reference appearance. This is a
source-backed inspection sequence, **not automatic contact gameplay**: the
selection uses a captured original contact context instead of arbitrary current
player geometry. Control return and positioning relative to player 1 are host
inspection scheduling. The original full contact/root-travel/event lifecycle
still needs to be connected before using this as a natural tackle.

The Windows runtime uses extracted assets plus small private captured context
fixtures. It loads no RAM snapshot, original DLL, Dolphin, ModernGekko or ISO.
Native match/actor startup remains unfinished.

## Reconstruction and original evidence

NFL Street GC GNNE69 revision 0. ISO SHA1
`813c4511ba3135059a13de424555647e8252254b`; DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`.
Two saved contact captures (right/forward) were made with Dolphin 2603a. They
share the documented original contact geometry and are not broad gameplay
coverage. Position-neutral-14/15/16 MEM1 snapshots provide the offline original
module environment for controlled stream comparisons.

`live_contact.c` reads the original selection/context and source tables, ignores
the pre-recorded request field in the older paired fixture, recalculates both
requests, and calls `nflstreet_contact_animation_complete`. Both complete actors,
the selection manager and initial shared globals are generated natively. The
common bank 00103's program bytes must match the corresponding supplied source
program exactly. Selected channel/stream state is derived from the resulting
active group-30 channels and the freshly computed request keys.

The two requests select clips `0249/024A`, bank key 1 / tag 0 in these contexts.
No additional football meaning is assigned to either clip or individual flag.
Native timing preserves finite-end behavior and freezes a completed stream
while the other continues. It carries flags and stream state back into the
owned actor bytes. Rendering uses the existing named-bone and weighted-cluster
adapter, checked metadata placement where present, and the existing field/actor
depth renderer. Root travel, complete original group weighting during the
contact, body orientation fidelity and exact original pixels remain open.

### New callback support

Original `8001532C`, accepted callback route: command `C00B` ORs `0x100` into
actor+0C (uint32, 4 bytes, BE). The new native handler preserves all other bits.
This is verified by actual original execution with four initial flag patterns;
all actor bytes agree. The same direct controls cover C004, C006 and C00C.
Eligibility uses the controlled accepted route (argument FFFF). This does not
reconstruct every natural callback eligibility branch.

## Verification and confidence

**Verified source selection/application:** fresh native initialization matches
both complete 5,332-byte original actors and the 256-byte manager in both
right/forward captures. Shared globals match the right capture; the forward
capture lacks that checkpoint and is not counted for it. These are original
capture comparisons, not comparisons to generated expected output blocks.

**Verified controlled joined playback:** six runs (two captured contact contexts
in three saved module environments) each execute 49 joined updates. Every byte
of both complete actors matches after each update: 294 joined updates and 588
full actor comparisons. Both finite ends and all five accepted callbacks per
pair agree. Actual original instructions run with a controlled accepted callback
context; earlier full gameplay scheduling is excluded. No callbacks are skipped.

The direct program oracle also checks both setups and 48 complete stream timing
states across three environments, including catch-up deltas and finite endings.
All 18,752 common-bank frames (78 components each) match the original decoder.
Invalid elapsed-time input leaves the native controller and output unchanged.

**Windows integration exercised:** both contact fixtures pass 200-tick runs.
Each starts the contact twice, rejects a busy request, completes both streams,
resumes movement and reaches held stops with retired movement fade channels.
The first eight full movement checkpoints remain exact. Visual captures show
both original contact poses in the field; they are not original gameplay pixel
comparisons.

The existing two-player eight-direction run, optional one-player set run,
default movement run and legacy inspection run pass. All corresponding prior
capture pixels remain identical. Generic Daywater behavior is unchanged.

## Reproduction and private files

Build `build_windows_contact_player.sh <private-output>`.
Viewer adds `--second-player --contact <selection-context> <paired-sources>`
to its existing movement/set arguments; `--contact-check` exercises the lifecycle.
`live-contact-check.exe` exports fresh initial actors/manager/globals for capture
comparison, then advances to both finite ends. `contact-playback-original-check.exe`
takes original DLL, MEM1, selection context, paired sources and common bank.
`contact-stream-original-check.exe` takes DLL, MEM1, common bank and clip index.

Private fixtures, original module, builds, captures, logs and verification hashes
stay in `W:\Daywater\research-temp\nflstreet-visible-contact-20261009` or their
existing private capture folders. Original DLL SHA1 remains
`dd521cc38b0db6bca98a0bb261570a2be5e6e0ed`; baseline runner SHA256 remains
`3f9e3a3eb4b5728d9036e9552c01e8c9dc084a1f53805fef95dcf3b1b86f73ea`.
Neither was edited or replaced; no restoration was required.

## Authorized work still open

Connect natural contact eligibility, original stream-driven event completion,
root travel and player-state handoff. Then connect ball ownership and
snap/pass/catch behavior, followed by fresh native actor/team/match creation.
Ball flight/bounce/rendering, rules, camera/appearance fidelity, audio and menus
also remain unfinished. The source-backed inspection is a visible connection
of the existing verified contact systems, not completion of the standalone game.
