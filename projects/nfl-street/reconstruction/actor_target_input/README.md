# Target-position input for a second actor

Latest milestone: [readiness signals and native handoff polling](event_signals.md)
now identify the live command stores and reproduce waiting/ready decisions
plus conditional old-actor flag cleanup. The
[native descriptor-to-event handoffs](descriptor_event.md)
now derive incoming actors from captured descriptors and construct events
before reproducing the two later record changes in Windows C. The
[live record writes](record_handoff.md) establish the first-down copy writer.
Historical unknown-writer notes below describe the earlier captures;
descriptor production, event timing, and full wrapper behavior remain open.

GNNE69 function `0x8008D864` receives the actor in `r3` and a target
position pointer in `r4`. In the observed branch, the original DOL calls
`0x801DE5B8` to subtract the actor's XY position from the target, uses
`0x80188D98` for heading, and uses `0x801DDFCC` for vector length. Live
write watchpoints stopped at `0x8008D920` for requested heading (`+0x204`)
and `0x8008D930` for requested strength (`+0x200`); the hash-checked DOL
contains those stores. The later active-heading and strength-cap handoff is
separate from this function.

The [portable source](actor_target_input.c) implements the measured
distance-at-least-one branch. It subtracts XY coordinates with single
precision, computes the original fused vector length, and applies the DOL
constants at `0x80250A74..0x80250A90`. Both observed strength cases are
covered: below the `0x3F3851EC` cap and above it with the original extra
status byte set. Distances below one or far enough to reach the function's
next clamp branch are explicitly rejected pending live tests.

Two fresh saved-match traces captured function entry and return twice each
for roster slot 7, whose name bytes read `Stewart`. The Windows x64 function
matched every byte of all four original 0x30-byte return blocks. Its output
then feeds [the native actor runtime](../command_to_movement/actor_runtime.h).
The runtime carries the prior movement position, applies the observed
heading/strength handoff, and runs the already checked reset and movement
stages. The [sequence verifier](../command_to_movement/verify_update_sequence.py)
matched all 0x30 actor bytes and all 21 movement words on each of three
updates under held right and three under held forward input. It also checked
both captured reset calls in each run. The first update still begins with a
game-populated actor block; the two later updates use captured target
positions rather than copied actor input blocks. The Windows executable ran
without Dolphin or the game image during replay.

| Private W: item | SHA-256 |
|---|---|
| `nflstreet-slot7-source-three-20261007.jsonl` | `f257fdf73ac6e4dc37744a778aeb3b481ebdeedd0b48edce566e8c7ca01b05d3` |
| `nflstreet-slot7-source-forward-three-20261007.jsonl` | `47f569f6e7181e14204e2b709a898e49ab59b45a3c587a6c98e09fe6ca3819a6` |
| `nflstreet-actor-target-input.exe` | `08d70893fd1b77d0d199b7572f12295796e2dad132f58964f738567160274498` |
| `nflstreet-actor-update-target-loop-20261007.exe` | `141e96dbb84e5ecfa1253c6b0dbd7aab90b701f6f7d583747b37cd491e2bc4da` |

The one-hit writer traces are also private on W: as
`nflstreet-slot7-strength-source-20261007.jsonl` (SHA-256
`3234280b4a8980779c0bc6ffcc8343f939ddab6487cabba255a5afa725c44df5`)
and `nflstreet-slot7-heading-source-forward-20261007.jsonl` (SHA-256
`be2082205fe243f42df70af873957759224f8a034287e36a7f0765282e911492`).
The watchpoint was removed after its first hit; both subsequent live
three-update sequences still passed the native verifier. Longer-held
watchpoints produced unreliable RAM snapshots and are not used as state
evidence.

**Reference:** NFL Street GameCube GNNE69 revision 0, ISO SHA-1
`813c4511ba3135059a13de424555647e8252254b`, DOL SHA-1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`, Dolphin 2603a,
saved-match state SHA-256
`ea3d1fa29797d1521b2ab83aee866c190ed092865b7739ceb537d15aba5e7f32`.

**Confidence:** `verified` for these four original function returns, their
observed writer PCs, and the two complete target-driven three-update replays.
The selection of the other actor, other distance branches, other actors,
and a native multi-actor scheduler remain unknown.

## Upstream target handoff

The hash-checked DOL calls `0x8008D764` at `0x8008DA60`, passing Stewart in
`r3`, another actor pointer in `r4`, and a 12-byte output pointer in `r6`.
The next call at `0x8008DA6C` passes that output pointer to `0x8008D864` as
`r4`. Two fresh Dolphin 2603a calls, one under held right input and one
under held forward input, stopped at selector entry `0x8008D764`, return
`0x8008DA64`, and input entry `0x8008D864`. In both, the other actor pointer
was roster slot 8, with name bytes `Thomas`. The output pointer and all 12
bytes were identical at selector return and input entry.

For these two calls, the output vector retained Thomas's X and Z and had Y
exactly 2.0 less than his position at both selector entry and return.
The [offline verifier](verify_target_source.py) checks the PCs, actor
pointers, byte-for-byte handoff, unchanged other-actor position, and the
observed offset. It passed both traces. The raw traces have been copied to
the owner's private W: research storage; they were not added to Git.

| Private W: trace | SHA-256 |
|---|---|
| `nflstreet-slot7-target-source-right2-20261007.jsonl` | `64754e8eff1b327fc33749b3dd0ae1b5db9331f98ae0cded0c5b896d9dbc8694` |
| `nflstreet-slot7-target-source-forward2-20261007.jsonl` | `2626edb2dc8244e69482cc77233c258521be545f0f8918e5d65467d6e602ba9f` |

**Confidence:** `verified` for this exact handoff and vector relationship
in the two tested calls. The general rule in `0x8008D764`, how slot 8 is
selected, and behavior in other match states remain unknown. The capture
tool is [here](../../tools/capture_actor_target_source.py); it requires the
compatible Dolphin 2603a build and the owner's private game image and
savestate.

## Bounded native vector composition

The [portable C function](target_selection.c) reconstructs the tested
single-precision addition of Thomas's position and the observed XY offset
`(0, -2)`. It accepts only that offset, finite XY inputs, and the observed
zero Z field. On rejected inputs it leaves the output untouched. It does
not select Thomas or infer a general targeting rule.

Built locally with `cc -std=c11 -O2 -Wall -Wextra -Werror -fPIC -shared` and
checked with [the native verifier](verify_native_target.py), the function
matched all six output vectors in two independent three-handoff Dolphin
2603a traces. The verifier also changed the offset to `(0, -1)` for each
input and confirmed rejection without an output write. The trace hashes
are `57f7daf97166dd2e055bdfffb30f3439dbf08f0b6bf87aff068bc675779af938`
for held right input and
`628cace966732ab07a9fbe990a5a19e8f655e9b7cadd8490346f7bcfcfaf4f09`
for held forward input; raw files remain in private W: storage. The same
traces support the [paired actor replay](../command_to_movement/paired_actor_replay.md).

The C function is now linked into the Windows actor loop through its
guarded `P:` input mode. The [paired replay](../command_to_movement/paired_actor_replay.md)
passes Thomas's prior native position into that mode; the C code forms
Stewart's target before his next step. Both three-frame paired traces match
all actor and movement output blocks. The original `T:` mode still matched
its three-update Stewart replay after this change.

**Confidence:** `verified` for these six C outputs against original calls
and the two tested Windows paired replays. Actor selection and a general
multi-actor scheduler remain outside this branch.

## Other-actor pointer source

At the original caller's `0x8008D99C`, the DOL calls `0x800BE3F0` with
Stewart in `r3`. Two new three-call Dolphin 2603a traces stop at that call,
its return at `0x8008D9A0`, and the subsequent target-output entry at
`0x8008D764`. In all six calls, the returned `r3` pointer was the same
pointer passed to the target-output function in `r4`: the observed roster
slot 8 actor with name bytes `Thomas`. The [choice verifier](verify_target_choice.py)
checks the call/return sequence, pointer identity, target handoff, and
vector relationship. A deliberately changed return pointer was rejected.

The same verifier checks four direct calls and the relevant record-index
instructions in the hash-checked DOL.
`0x800BE3F0` calls `0x800BDDC4`, then `0x800BE380`; the latter calls
`0x80020944`. Disassembly shows `0x800BDDC4` selecting one `0xF4`-byte
record using an index byte and `0x800BE380` passing that record's `+0xB4`
field to `0x80020944`. Fresh live boundary captures below now verify the
record and handle arguments for this saved-match branch. The rule that
changes the index and thereby chooses another actor remains unknown.

| Private W: choice trace | SHA-256 |
|---|---|
| `nflstreet-slot7-choice-right-three-20261007.jsonl` | `00b11d7790fc7ccd763daeec25cef14155b8ccb214529a9a15a73a6769dce2dd` |
| `nflstreet-slot7-choice-forward-three-20261007.jsonl` | `d96c8b73e80c9cc881d053e4a09ba30493dace0c98cd24aa8854ead484576fe8` |

**Confidence:** `verified` for the six immediate call-return-pointer
handoffs on this GNNE69 revision-0 saved match, Dolphin 2603a, and the
four DOL call instructions. Selection behavior in other states remains
unknown.

### Handle to roster slot

The original DOL gives the remaining calculation for this branch. At
`0x80020944`, the low handle byte must be `1`; the next two bytes become
team and member indices. `0x8003A4A0` multiplies team by the roster's
actors-per-team count, adds member, and multiplies the result by the
`0x14D4` actor stride. The `0x800BDDC4` branch reads an index byte at
selection-manager `+1`, uses it to choose a `0xF4`-byte record, and passes
the record's `+0xB4` handle through this lookup. The
[choice verifier](verify_target_choice.py) checks these DOL instructions,
the complete pointer arithmetic, and the six live return pointers.

In the original private second-down MEM1 snapshot (SHA-256
`8c0152c92796e9783674be81703025ede544fcb71c1d62fe84ced9680facdd0a`),
the selection-manager index is `0`, its selected record is `0x803B6F08`,
and the handle is `0x00010101`. The roster has 7 actors per team and base
`0x803B7320`. The DOL formula yields slot `1 * 7 + 1 = 8` and pointer
`0x803C19C0`, exactly the Thomas pointer returned in all six live calls.
The private RAM snapshot is `original-8287f395.mem1` on W:.

The [bounded C lookup](target_selection.c) accepts a handle and roster
bounds, returns the calculated slot, and rejects unsupported tags or
out-of-range slots. Compiled as a private native library on W:, it matched
all six live choice traces; changing the tag to zero was rejected without
writing an output slot. The same source is compiled into the
[Windows paired loop](../command_to_movement/paired_actor_replay.md).

**Confidence:** `verified` for this handle-to-pointer chain in the
specified DOL, original MEM1 snapshot, and six calls from the same saved
match. The rule that changes the selection-manager index, other handle
tags, and actor eligibility across match states remain unknown.

### Live record and lookup arguments

The capture tool's `--trace-choice --trace-choice-internals` mode now stops
inside the choice call: after `0x800BDDC4` returns, before and after the
`0x80020944` lookup, after `0x800BE380`, and at the original caller return.
Two fresh Dolphin 2603a runs captured three complete calls each under held
right and held forward input. The [choice verifier](verify_target_choice.py)
checked all intermediate PCs, pointer identities, the record `+0xB4`
argument, the handle bytes, the native handle-to-slot result, and the final
target handoff. All six matched. It rejected a deliberately changed handle
argument in a copy of one trace.

| Field | Observed value and evidence |
|---|---|
| Game and build | NFL Street GameCube GNNE69 revision 0; ISO SHA-1 `813c4511ba3135059a13de424555647e8252254b`; DOL SHA-1 `46e3de854998a0f66c9be8c72a1164d22cd29eb4`; Dolphin 2603a |
| State | Saved match SHA-256 `ea3d1fa29797d1521b2ab83aee866c190ed092865b7739ceb537d15aba5e7f32` |
| Record index | `r13-0x73D0` points to manager `0x803B6F00`; byte at manager `+1` is `0` (unsigned 8-bit; endianness not applicable) |
| Selected record | Manager `+8+index*0xF4` = `0x803B6F08`; `0x800BDDC4` returned this pointer in all six calls |
| Actor handle | Record `+0xB4` = `0x803B6FBC`; unsigned 32-bit big-endian `0x00010101`; passed to `0x80020944` through `0x800BE380` in all six calls |
| Result | `0x80020944` returned `0x803C19C0`; the same pointer reached the caller and target-output function; observed roster slot 8 has name bytes `Thomas` |
| Related code | Readers: `0x800BDDC4` (index and record), `0x800BE380` and `0x80020944` (handle), `0x8003A4A0` (roster pointer arithmetic). Writers of index and handle have not been established. |
| Procedure and confidence | Restore the listed saved match in Dolphin 2603a; hold right or forward, stop at the listed boundaries for three successive Stewart target calls, and run `verify_target_choice.py` against the trace, hash-checked DOL, private MEM1, and bounded native library. `verified` for these values and pointer handoffs in the six calls; changes across other states are untested. |

| Private W: live internal trace | SHA-256 |
|---|---|
| `nflstreet-slot7-choice-internals-right-three-20261007.jsonl` | `5e11f5dd2ece7731c35ccb433827158cf5ac5afe496cd550b56dfbded18fa63e` |
| `nflstreet-slot7-choice-internals-forward-three-20261007.jsonl` | `0eeea6fe5e379fd4614c2f88e1dff82f2489c7a76178f202212877792207a936` |

The selected index stayed `0` and the handle stayed `0x00010101` in these
calls. Their live writer paths and other-state values are not established.

### Selection-index write boundary

The hash-checked DOL has a direct store to the verified index byte at
`0x800BDD98`: it loads the selection-manager pointer from `r13-0x73D0`
and stores the caller's `r3` low byte at manager `+1`. A scan of the DOL
text sections finds exactly two direct calls to this function, at
`0x8000AFC8` and `0x800BDF0C`. The former loads its
argument from another structure's `+0x59D` byte; the latter passes literal
zero. The [offline verifier](verify_selection_index_writer.py) checks these
exact instructions and call destinations against DOL SHA-1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. This establishes a
specific code path that *can* write the index; the function's role and
when each caller runs still need live evidence. Indirect calls are not
covered by that static caller scan.

The [isolated watchpoint probe](../../tools/trace_selection_index_writer.py)
watched byte `0x803B6F01` in two fresh Dolphin 2603a restores of the
saved match. Neither a held-right nor a held-forward 25-second observation
reported a write. Both started and ended with index `0`. These bounded
no-hit results do not show that the index is constant in other states or
that either caller executes during the observed play.

| Private W: watchpoint trace | SHA-256 |
|---|---|
| `nflstreet-slot7-index-watch-right-20261007.jsonl` | `962d92a3e3b6b91545245025b165bb33871476d923b3fbfd68d2699bf635a700` |
| `nflstreet-slot7-index-watch-forward-20261007.jsonl` | `3ba91f095ecfc6397d7e0a16fc970d0aef7e72cc93b3f50c339fc068380af32f` |

**Confidence:** `verified` for the static DOL store and the two direct call
instructions; `candidate` for those paths as the relevant live index writers
outside this saved match. A state transition that reaches the store is the
next test.

### First-down record-handle transitions

The previously retained first-down MEM1 series provides a different lead.
The [snapshot verifier](verify_selection_snapshots.py) hash-checks four
samples each from independent held-forward and held-right captures,
reads the manager index and selected record handle, and runs the bounded
native handle decoder. In **both** series the index is `0` at all four
samples, while the handle changes:

| Sample | Record handle | Native slot | Name bytes at calculated roster entry |
|---|---|---:|---|
| `00` | `0x00000000` | rejected | — |
| `01` | `0x00050101` | 12 | Urlacher |
| `02` | `0x00000101` | 7 | Stewart |
| `07` | `0x00010101` | 8 | Thomas |

The private 24 MiB files are `position-longforward-{00,01,02,07}.mem1`
and `position-longright-{00,01,02,07}.mem1` on W:. Their SHA-256 values
are fixed in the verifier; no RAM bytes are in Git. The native C decoder
matched the selected slots and the corresponding roster name bytes in all
eight checked snapshots. The `position-long*` capture procedure and
first-down reference state are described in
[player_position.md](../../../research/nfl_street/player_position.md).

A first-down Dolphin 2603a write watchpoint on record `+0xB4` produced
no hit in a bounded 25-second held-right run, including one A-button snap;
the handle was still zero at the final sample. Its private W: trace is
`nflstreet-handle-watch-firstdown-snap-right-20261007.jsonl`, SHA-256
`fd5743bb83f1b60a079001009e94260b9fce3b669536d9b38b8a92fe5ef1b047`.
The run was repeated with the existing debugger-mode and background-input
session fixes; it also produced no hit and ended at zero. That trace is
`nflstreet-handle-watch-firstdown-fixed-right-20261007.jsonl`, SHA-256
`af993830ed75e4902e99d541a761eb3c337fd837210b79cd058b2d88427ea3a7`.
This no-hit probe does not identify the writer or refute the earlier
snapshot transitions. Debugger watchpoints have previously slowed or
disturbed this state.

**Confidence:** `verified` for the exact handle/index bytes and roster
names in these eight hash-checked snapshots, and for the bounded C slot
calculation on those bytes. The original DOL establishes the handle
lookup formula. Whether a live target-choice call reads each changing
first-down handle is still `candidate` until captured at that call.

### First-down live choice calls

The first-down preplay savestate `19bb4a7c` (SHA-256
`e2faeb8f9481096ff87ae261c8c2943452fbc023cd27510f641808e307603d93`)
was restored twice in Dolphin 2603a. The capture tool armed its choice
breakpoint before pressing A, then held right in one run and forward in
the other. Its first complete Stewart choice call in each run returned
record `0x803B6F08`, passed big-endian handle `0x00010101` from record
`+0xB4` to the lookup, and returned Thomas's roster-slot-8 actor pointer
`0x803C19C0`. The target-output function received that pointer and
produced the same observed position plus `(0, -2, 0)`; all 12 output bytes
reached `0x8008D864` unchanged.

The [choice verifier](verify_target_choice.py) checked both complete
record-to-input call chains against the original hash-checked DOL and the
bounded native handle decoder. This is a second saved-match state that
reproduces the Thomas choice, beyond the earlier second-down tests.

| Private W: first-down choice trace | SHA-256 |
|---|---|
| `nflstreet-firstdown-choice-right-20261007.jsonl` | `20e81dff609feccd9dc52c0b3200f141957b029de4b64e1e9260fcdb78a9e37d` |
| `nflstreet-firstdown-choice-forward-20261007.jsonl` | `29647334dbc36c7c274c07759a9c476e67eb3d8f2f56fda8dc6a84fa2ecf8d6d` |

**Confidence:** `verified` for these two first-down Thomas choice calls.
The calls did not sample the earlier Urlacher or Stewart handle values in
the RAM series; whether those values feed this particular target-choice
path remains `candidate`.

### Static record-handle write paths

The GNNE69 revision-0 DOL contains a small handle helper at `0x80020988`:
when `r3` is nonzero it copies the 32-bit word at `r3` to `r4`; when `r3`
is zero it stores zero at `r4`. In the function beginning at `0x800BE264`,
the call at `0x800BE29C` passes zero and a destination at incoming `r3`
plus `0xB4`. In the function beginning at `0x800BE2B4`, the call at
`0x800BE2F8` passes incoming `r4` as source and incoming `r3` plus `0xB4`
as destination. The [offline verifier](verify_handle_writer.py) checks the
helper, argument setup, and both call destinations against the hash-checked
DOL. It passes locally without Dolphin or private game data.

These are concrete code paths capable of clearing or copying a record
handle. No live call has yet shown either path receiving the observed
record pointer `0x803B6F08` or writing its handle at `0x803B6FBC`.
The next probe should break on the calls and record the source, destination,
and before/after words during a first-down transition. It should also
confirm that the play advances, because the earlier write watchpoints
disturbed this state.

**Confidence:** `verified` for the static instruction semantics and direct
calls; `candidate` that these calls cause the observed first-down handle
transitions. No selection policy is inferred from this static path.

## Native readiness-command timing

The [command timing replay](command_timing.md) derives readiness emission
from its countdown and command cursor. Windows C matches 30 complete
updates in two fresh runs, including four readiness emissions. Stream
selection, earlier event stages and a combined match loop remain open.

## Stream setup and early events

The [stream setup and early-event slice](stream_setup_early_events.md) looks
up source programs and initializes their interpreter in Windows C. It also
reconstructs observed stage-zero/one decisions and pre-contact stage-two
steering. A selected paired-animation asset supplies native pair requests
and a derived stream key. The contact asset matcher, full contact effects
and a combined simulation remain open; rejected branches are counted
explicitly by the integration verifier.

## Contact selection and completion

The [contact component](contact_selection.md) now generates contact parameters,
checks eligibility and scans all supplied candidate assets. Three fresh runs
match 15 selection calls and three successful stage-three transitions with
native requests, flags and links. The selected asset is no longer an input.
The verifier reports remaining animation-service state changes explicitly;
channel allocation, source asset resolution and full animation application
remain outside this component.

## Animation allocation and state updates

The [animation component](animation_channels.md) now reconstructs channel
allocation, bounded manager allocation/preparation, clip initialization and
blend writes. Windows C matches 26 full block comparisons in two fresh runs,
including different channel choices for the two actors. Source bank data and
programs remain providers; callback metadata and transition orchestration
remain outside this component. Full-manager eviction is rejected without
mutation. The evidence distinguishes live verified branches from static
compaction/fallback reconstruction.

## Complete bounded contact animation

The [callback and transition reconstruction](callback_metadata_transitions.md)
now joins both actors' links, requests, group/channel allocation, clip setup,
metadata, transition decisions and event completion in one native paired
application. Two fresh runs match both complete actor objects and the
selection manager. The selector supplies native-generated requests; raw
bank/asset/registry/profile tables remain providers. The live transition
searches have no eligible row; spawning, mirroring and other branches remain
unsupported. Paired failure rolls back both actors, manager and globals.

### Native animation asset loading and transition clip spawning

[Evidence and scope](asset_loading_transition_spawning.md): Windows C loads
12 original disc bank source blocks, resolves metadata/program/transition
sources, and starts two different transition clips. Two fresh controlled
Dolphin experiments match six full actor checkpoints each and both original
clip durations. Initial registry/state, frame payload loading and native startup
remain open.
