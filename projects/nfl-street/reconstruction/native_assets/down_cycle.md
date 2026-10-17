# Down cycle (original-module baseline verified; live repair pending)

NFL Street, GameCube GNNE69 North America rev 0, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. MEM1 is big endian;
r13 is `803A9A40`. Capture contexts 14/15/16 are private inputs. Their Dolphin
build is unavailable. This author writes and compiles code only, and cannot
execute Windows programs. No all-RAM, mutation or live execution result is
claimed for new integration changes until the external gate passes this exact revision.

The owner's gate on `cb6787a` passed all-RAM and defined-return comparisons
in contexts 14/15/16, twice each, and caught all six mutants. The reported
context-16 summary was 7,233 updates, 2,806 leaves, contact/bounds/touchdown
counts 88 each, 80 incompletions, 24 commits and 24 snaps (digest `f4442890`).
The live run logged a whistle and two subsequent play selections, then failed
on a controlled movement rejection and two held AI actors. This revision's
additional control cases and live wiring are **candidate**, pending that gate.

## Source trace and native boundary

The field object is `*(r13-7078)`. Down reader `80103964` reads its +30;
`8010397C` writes it. Target-line writers `80103C30/74` write +08 and preserve
+04. Spot writer `80103A28` writes +0C/+10, preserving +14/+18 and publishing
+1C/+20. Possession reader/writer `80103C88/BC` access +38; original possession
for the current down is +A4. Score readers/writers `80104274/94` use u16 +3C/+3E.

Those writers lead back to the result builder `800262B0` and commit `800263EC`.
The original phase table at `80245A80` has three function pointers per phase:
entry, update, exit. `8002E45C` calls exit, writes current/previous phase through
`*(r13-7CF8)+0/+4`, then calls entry. Phase 2 is pre-snap, 3 live, 4 whistle,
5 play selection. These interpretations are **probable** from source callers,
strings and state-table flow; equality against the original is still **candidate**.

`80024E50`'s live phase gets the active carrier through `800BE3F0`, then calls
`800F9C50`. It evaluates source geometry/boundaries (`80103EA8`, `800FA9D0`,
`800FDF2C`), contact state (`800FDAAC`), scoring and event flags. Incompletion
consumer `800FC2C8` emits event 10 hex, then `80103CF0` enters whistle phase 4.
Whistle entry `80043104` runs the result builder through `800266A4` and queues
its original commit callback. `80025028` folds actual events into that result;
`80025840` handles next down, first down, turnover, scoring, conversion, safety
and touchback conditions. There is no host replacement for those rules.

The implementation compiles source-addressed C functions, including actual
indirect phase, animation, event and sort callbacks from all three captures.
`build_down_cycle_native.py` and `down_cycle_sources.json` retain input hashes
and bindings. The broad transitive closure includes unexercised paths; that
inclusion alone is not verification. It contains no runtime opcode decoder,
DLL provider or copied game assets. Compile with `-ffp-contract=off`.

### Actor-command sixth callback (candidate; source/capture binding)

The owner's next run passed the earlier voice-start path and reached context
14, case 403/stage 570, then rejected `8007EA64 -> 8007EF14` while finishing
a contact-down queue. This is an omitted gameplay callback, not an audio/disc
service or an invalid fixture. `8007EA28` loads the command byte, multiplies it
by `18` hex, reads the row's function pointer at `+14`, then executes `BLRL`
at `8007EA64`. The shared AI builder's initial table walk collected only five
pointers per row (`+0/+4/+8/+C/+10`), omitting that sixth column.

The down builder now gathers all six columns of every registered command
table in captures 14/15/16 and includes them in both the native target set and
the construction-time decoder's known-entry boundaries. The shared builder
is unchanged. `8007DE88` owns the registry pointer at `*(r13-7860)` and records
its slot capacity at `r13-50A4`; `800B7CE8 -> 8007DF20` installs the descriptor.
Each descriptor contains u16 row count at `+0`, u16 command-stack capacity at
`+2`, and a table pointer at `+4`. All three captures have 30 registry slots,
with slot 0 pointing to `803A2638`: 95 rows at `8026EE1C` and command-stack
capacity 10. The other registry slots are NULL. These are observed bindings,
not a restriction on the randomized cases. `down_cycle_sources.json` records
the per-context descriptors, every callback column including NULLs, and table
hashes. The original callback registry and selectors are never edited.

Row 52's sixth entry is `8007EF14`. Ghidra has no separate cache file for it;
all three captures contain the same seven instructions ending at `8007EF30`,
the next cached function. They read command byte `+2`, calculate
`(40 hex - byte) & 7F hex`, store it back at `+2` and return. The builder pins
those 28 bytes before construction-time decoding and emits the full original
load/arithmetic/store/return path, with no runtime opcode decoder or callback
stub. The byte write remains part of the all-RAM comparison. Collecting the
whole sixth column also includes the other callbacks previously absent from
the native closure and bounds adjacent uncached entries correctly.
In particular, `8009DBF0` now ends at `8009DBF8` after its original
`LI r3,1; BLR`; the following callback gets its own native entry. Its formerly
unreachable trailing instructions are retained in that new callback, and the
constant-return path is unchanged. The other existing native bodies are
unchanged; this regeneration adds 11 callback functions.

No service entry, oracle hook, fixture input, comparison mask, timer or update
budget changed in this repair. Native/original execution agreement and mutant
coverage on this revision still require the owner's external gate.

## Candidate field ledger

All rows inherit the title/region/revision/hash, big-endian MEM1, unavailable
capture Dolphin build and private contexts specified above. The test procedure
is identical prepared RAM versus the original module after each operation,
plus retained multi-update sequences. No row is promoted to verified here.

| Address/path | Primitive / size | Observed context 15 | Reader / writer | Confidence / notes |
| --- | --- | --- | --- | --- |
| `*(r13-7CF8)+0/+4` | u32 / 4 each | 3 / 2 | `8002E4F8` / `8002E45C` | candidate; phase and previous phase |
| `*(r13-7078)+30` | u32 / 4 | 1 | `80103964` / `8010397C`, commit `800263EC` | candidate; current down |
| same `+08/+04` | f32 / 4 each | -20 / 0 | `80103C18/24` / `80103C30/74` | candidate; target and prior target |
| same `+0C/+10` | f32 / 4 each | 0 / -33 | `801039D4` / `80103A28` | candidate; current spot XY |
| same `+38/+A4` | u8 / 1 each | 1 / 1 | `80103C88/CC8` / `80103CBC/CD4`, `80103DAC` | candidate; current/original possession |
| same `+3C/+3E` | u16 / 2 each | 0 / 0 | `80104274` / `80104294`, `8002676C` | candidate; score words |
| same `+34` | u32 / 4 | 8 | `80103D4C` / `80103D70` | candidate; play flags, source bit tests retained |
| `*(r13-7B98)+104..123` | original result / 32 | only valid after whistle entry | `80043104`, `800262B0`, `800263EC` | candidate; delayed proposed result |
| active ball `+A4/+A8/+AC/+B0` | u32 / 4 each | 1 / 0 / 1 / 0 | `800C2684/269C` / `800C25FC` | candidate; opaque current/previous state and arguments |
| active ball `+B4/+C4` | actor reference / 4; u32 / 4 | 00010101 / 0 | `800BE380/BEC50`, `800F9C50` | candidate; carrier and contact mode consumed by end checks |

## Explicit host choices and omissions

`nflstreet_down_cycle_update(c,&event)` runs after the existing actor, channel
and ball updates, once per game tick. Phase 3 runs only the original carrier
check, avoiding a second AI/physics update. Phase 4 advances the original
`8004EBF0` callback queue (including the delayed result commit), then requests
original phase 5 when the queue drains. Replay, celebration, audio and menu
wait scheduling from the full phase-4 update is not modeled. Original reached
entry/exit/event functions remain transcribed. The twelve named audio/disc service
entries below return early on both sides; no gameplay routine is skipped.
A reached unsupported instruction, function or memory span stops the update.

The caller selects/installs actual plays, then calls
`nflstreet_down_cycle_lineup(c)` for original team positioning and transition
to pre-snap phase 2. The live app requests `nflstreet_down_cycle_snap(c)` after
60 updates in phase 2. That executes `800F946C` and the actual `800815B4` snap
consumer for the offensive play's +16 actor slot. Automatic menu confirmation
and snap timing are host choices; this is not the original interactive menu
or snap-button interpreter. The source owns assignments and ball ownership.

Calls use `ai_policy_host.h`'s scoped saved caller identity and original SDK
scheduler lock/unlock. CPUState starts with timebase zero in the oracle;
source `mftb` reads that same fixed zero at this synchronous host boundary.
Interrupts, audio hardware, disc streaming and original OS threads are not
simulated. Private call temporaries occupy owned context storage corresponding
to `80F38000..80F46FFF`; production never borrows gameplay RAM as scratch.

### Audio/disc service boundary (source audited, execution pending)

The project manager explicitly directed the hardware and sound/streaming layer
to remain outside this module. Earlier context-14 case 401/stage 570 exhausted
the budget inside the ARAM client's synchronous completion wait. This revision
cuts at audio/voice requests, the UI resource-load callback and the animation-bank
stream request, above their
generic resource, OS and device calls.
It removes the earlier DVD/ARAM zero-read/discard-write fallback. Native device
addresses now fail the span check, and the original checker rejects external
reads/writes. There is no DMA, completion interrupt, audio clock or disc model.

The original checker lists exactly these twelve original entry addresses. It
returns to guest LR before executing each listed routine, without calling
native code or writing guest RAM. The native function returns at the same
entry. Void entries preserve incoming registers. The sound-cue start returns
`7FFFFFFF`, its own disabled-audio return in `8013D8A4`; this means no voice
handle, rather than an invented successful request. The UI load callback
returns NULL, following the consumer's existing unavailable-resource branch.
The streamed voice-start API `801AB968` returns `FFFFFFFF`, its own
disabled-audio handle; the original speech caller still publishes that result.
The animation stream request returns zero without submitting a request or
publishing readiness; this is an explicit suppressed-service policy, not a
claim that an asset loaded successfully. Its original caller ignores the
status. Captured bank contents are retained; no replacement asset is supplied.

The first boundary revision checked only the PC returned by the DLL's
dispatcher. The owner's next run failed at context 14, case 2, stage 3:
`80284692` was `1A` originally and `00` natively. This byte belongs to a
sound-cue timestamp. The original generated chunk contains a forward
`BL 8013D8A4` at `8013D884`, emitted as `goto label_8013D8A4`; it therefore
does not pass through the caller's dispatch hook. Direct cross-chunk C calls
can bypass that hook too. Native skipping was active while the original
still ran that sound request. This is a checker boundary defect, not an
invalid touchdown fixture or evidence for excluding another RAM range.

The checker now also installs the unchanged DLL's exported
`ppc_set_mem_write_journal` callback. Every listed entry begins with
`STWU r1,negative-frame(r1)`. The DLL materializes guest PC and invokes the
journal **before** storing the stack backchain and updating guest r1. At
exactly one of the twelve named PCs the callback checks the captured opcode,
write width, effective stack address and private stack bounds, applies the
same void/disabled-handle return, and uses `longjmp` to leave the generated C
frames. No service RAM store commits and no native game implementation is
called. Ordinary writes and every other original instruction continue in the
original module. The protected DLL and its generated sources are not patched.

`setjmp` resides in a separate dispatch helper; the caller owns CPUState, so
guest state updates remain defined across the nonlocal return. Unwinding can
retain the DLL's host recursion acceleration depth. Its generated header
bounds that counter at 24 and then uses the normal dispatcher; it controls
only host call optimization, not guest instructions or MEM1. The checker
requires a positive in-chunk interception count and reports it separately.
That correction retained the nine native bodies, case generator, all-RAM mask
and time budget. Runtime agreement still awaits the owner run.

The subsequent owner run reached case 401/stage 570 with a complete source
backchain to the play-selection UI load request:
`80037528 -> 801D0264 -> 801D007C -> 801CFD90 -> 8012AC68 -> 8012A360 ->
80129E3C -> 801A69C0 -> 801A4424 -> 8017BB78`. The failing store was
`801FD65C -> CC005020`; the resource request was an ARAM read. This is UI
resource streaming, not a sound request. The new boundary is the registered
UI resource callback, without skipping the UI or football state machines.

The following owner run reached context 14, case 402/stage 146 while the
touchdown's queued commit requested a celebration animation bank. The logged
backchain resolves to `800263EC -> 80020704 -> 800200B8 -> 80016510 ->
80019354 -> 8001695C -> 80175448 -> 801A735C -> 801A4C40 -> 80183824 ->
80182E60 -> ... -> 801F748C`, ending in the DVD write to `CC006004`.
The new cut is the bank request `80175448`, above the shared resource reader.
This is a valid touchdown case; no fixture constraint, update window,
comparison mask or gameplay callback was changed to avoid it.

- **not modeled: audio/disc service (`8012AC68`, UI resource-bank load/stream request).**
  `8012AA2C -> 801CFD50` registers it at UI manager `+14`; all three saved
  contexts retain that registration. `8012BBD8` initializes its separate
  `*(r13-6CE8)` resource manager, including `uis_pops.dat` bank data. The
  request's writes are bank/block cache records, refcounts, request readiness,
  and heap/OS/stream state. `8012A168` owns bank records (`+0` bank index,
  `+4` resource descriptor, `+8` block count, `+C` refcount, `+10` block cache).
  `80129E3C` owns block records (`+0` block index, `+4` refcount, `+8` data,
  `+C` readiness). `80129D04/80129D94` search those caches; their comparators
  publish only search-result pointers to caller stack locals. `80129DDC`'s
  async callback likewise publishes only block data/readiness. `8012A360`
  forwards the request; `8012A3B0` returns ready data or NULL. The entry itself
  brackets those calls with the original heap context push/pop. No actor,
  ball, down, distance, score, possession, phase or game-config cache is written.

  Returning NULL is an explicit unavailable-service result, rather than a
  fabricated asset or DMA completion. The original consumer `801CFD90` tests
  that pointer and returns failure before creating a UI instance. Its parent
  `801D007C` handles that failure and rolls back its tentative UI transition
  record. Those functions, `801D0264`, `80037528`, play selection and lineups
  still execute and remain in the all-RAM comparison. In particular,
  `80037528` also moves actors and writes selection state, so skipping it
  would hide gameplay work. The host still chooses/installs the native plays;
  loading the game's interactive play-selection UI assets is not modeled.

- **not modeled: audio/disc service (`8013D4D4`, sound-bank load request).**
  Called by the sound-bank manager. Its publication writes a bank pointer to
  `*(8030A334) + slot*4`; its `801AB224` callee allocates/loads an audio bank
  through `801AAAF8`. The slot array belongs to audio initialization `8013D2EC`.
- **not modeled: audio/disc service (`8013D8A4`, sound-cue start request).**
  Writes the cue's last time/voice handle at `*(8030A31C)+cue*18+10/+14`,
  voice/mixer state through `801AB4B8`, and replay audio history through
  `80142CB0/80142BE8` in `*(r13-6A80)`. Positions are input copies for spatial
  audio; the actor/ball position is not written.
- **not modeled: audio/disc service (`8013D9C0`, sound-cue stop request).**
  Reads that audio cue's handle, then `801AB598 -> 8022ABD8` updates voice state
  under the audio lock pair. It does not clear a gameplay actor or event.
- **not modeled: audio/disc service (`8013DAD4`, streamed sound-bank load).**
  Publishes only the stream descriptor at `*(8030A338)+slot*4`; `801AB77C`
  allocates the audio stream record and loads its bank data through the shared
  resource reader. That reader remains available for non-audio assets.
- **not modeled: audio/disc service (`8013FFF8`, ambient bank transition/wait).**
  Owns the bank handoff wait and writes audio state `r13-6AC8`, sound choices
  `r13-6B04/-6B03`, audio bank request records and the audio RNG handle
  `*(r13-4FDC)`. It has no calls into the game-code address area.
- **not modeled: audio/disc service (`801405B8`, replay sound-cue request).**
  Requests the cue above and advances only the audio cursor `r13-6AD0`.
  Its two game-code callees read replay/mirror flags; neither contains a store.
- **not modeled: audio/disc service (`8014164C`, music/stream control request).**
  Resolves an audio descriptor in `803877F8` and supplies audio control values
  through `801ABA64 -> 8022A7B4`. Writes are audio channel/command state.
- **not modeled: audio/disc service (`8014368C`, announcer bank prefetch).**
  The wrapper calls `801AD66C`: audio's `chatdata.dat` bank manager supplies
  the resource/index, then the loaded bank is passed to `801728F8`. Its writes
  are resource-cache/request, heap/OS, and speech-engine state.
- **not modeled: audio/disc service (`80175448`, animation-bank stream request).**
  `80016278` opens `animdata.dat` and initializes this subsystem through
  `80173B10/80174FD8`. The latter allocates its separate bank cache at
  `*(r13-4F6C)` (records of `1C` bytes) and request records at
  `*(r13-4F5C)` (8 bytes each). The request changes only the selected cache
  record's bank index `+0` and readiness `+12`, and the request record's slot
  `+0` and asset descriptor `+4`. It supplies the bank's private destination
  buffer at cache `+C` to `801A735C` with completion callback `80174B20`.
  Its other work releases a previous cached bank (`80174BC4`) or waits for
  an outstanding resource request; those paths operate on resource/heap/OS
  state, not actors, ball or down state.

  The callback audit includes indirect calls, not just the direct callgraph.
  All three captures bind `r13-6724` to completion `80175FAC`, `r13-5798` to
  relocation `80151928`, and `r13-6720` to release `80151968`, matching their
  original initialization. The resource observer at `r13-652C` and timeout
  callback at `r13-6524` are NULL in all three captures; the builder rejects
  unreviewed replacements. `80174B20` publishes bank readiness/asset IDs and
  forwards the asset descriptor to `80175FAC`. That callback relocates pointers
  inside loaded asset/animation descriptors, fixes embedded `BFCM/BFFM`
  resources and publishes cache entries. `80174574/80151834` likewise fix
  resource-local offsets; `80151620` moves asset data through heap allocation;
  `80151968` releases that allocation. `8017959C` invalidates the asset lookup
  tables at `*(r13-4F48)`. None writes a gameplay actor, ball, field object,
  phase, possession, score, down/distance or game-config cache. The builder
  pins these callback bindings, their disassemblies and their callee closure;
  neither the direct nor callback closure reaches the game-code address area.

  Both sides return zero at the request entry, before any cache/request write.
  Zero is also the original already-resident/no-request return, but suppression
  does **not** mark a missing bank resident. Caller `8001695C` ignores the
  status and still writes its own celebration bank references; `80019354`
  retains its own result/descriptor publication and `8001BD74` retains its
  actor/ball-dependent eligibility work. Skipping those higher callers would
  hide gameplay writes. Their complete implementations, the score commit
  and the whistle queue remain compared on all RAM. Streaming celebration
  assets and their asynchronous readiness are unmodeled; the host already
  omits the full celebration/replay wait schedule described above.
- **not modeled: audio/disc service (`801ABA64`, streamed sound control).**
  This is also called directly by ambient stream controls `80137B84/80137C08`.
  It checks audio flags `8034859C`, takes `802292C0/80229300`'s audio lock and
  calls `8022A7B4` to change audio channel state. No game object is supplied.
- **not modeled: audio/disc service (`801AB968`, streamed voice-start request).**
  The next owner run again reached context 14 case 402/stage 146, now after
  the animation request. Its complete backchain passes through speech event
  `801433C4 -> 8014321C -> 8016F464 -> 801713B0 -> 80170D1C -> 80143ACC`,
  then `801AB968 -> 8022A488 -> 8022A284 -> 801AE6FC -> 801ADDB8 ->
  801ADEE8 -> 80183824 -> ... -> 801F748C` and `CC006004`. This is an
  announcer voice's streamed audio data, not another celebration-bank request.
  The fixture remains a valid touchdown; no generator restriction was added.

  `801AB968` is the audio API immediately below the speech caller. It
  returns `FFFFFFFF` when `(8034859C & FFFF00) != 10100`; both sides use that
  existing disabled-audio result without changing the global audio flags.
  `801441F4/8014159C` supply audio channel descriptors from `803877F8`.
  `8022A284` resolves only an audio voice owner through `802297B8`'s
  `80362E68` table. Its stores initialize a voice queue record, advance the
  owner's voice-handle counter and link that record into the audio queue under
  `802292C0/80229300`'s audio lock. `801AE6FC` validates a `STRM` owner,
  allocates its stream request, records the bank/block key and disc range,
  then links the request through `801AD8D0`. `801ADDB8/801ADEE8` update only
  that owner's request list, ring-buffer positions, pending byte counts and
  file request token; `80183824` receives a destination inside that audio
  ring buffer. Completion `801ADD94 -> 801ADC98` advances those same byte
  counters, chunk/parser state and stream readiness. The builder now audits
  and hashes that named callback's closure as well as the direct closure.
  Neither closure contains a game-code entry; writes are audio/stream/OS
  state, never actors, ball, phase, field, down/distance, possession or score.

  The higher speech caller cannot be skipped: `80143ACC -> 801439B0 ->
  801436AC` calls phase reader `8002E4F8`, which writes `r13-7CF4`.
  `80143ACC` and the speech engine therefore remain complete source/native
  routines. They still perform their decisions, phase-cache publication and
  audio history/handle stores after the API returns the disabled handle.
  Only the original `801AB968` entry returns early, including when reached
  inside a generated chunk. The case schedule, RAM mask, gameplay callbacks
  and hardware-access rejection are unchanged.

The write audit follows each entry's disassembly/decompilation and direct
callee closure, then checks ownership at the audio initialization and request
callers. Generic allocation, resource containers and OS synchronization below
these APIs serve their private audio/resource objects. The audio entries'
direct closures contain only two
leaves below `80130000`: replay predicate `8000C360` and field mirror predicate
`80103E64`. Both predicates consist exclusively of loads, register operations
and branches, with no stores, calls or indirect branches. The UI entry
additionally reaches the seven exact resource-cache
functions listed above; these have a separate write ledger and are allowed
only in that entry's audit, without exempting any address range. The animation
entry additionally audits its four named callback roots and their closure;
the streamed voice entry separately audits its file-completion callback.
The builder enforces that property
and records closure hashes and those leaves in `down_cycle_sources.json`.
This source audit is not a substitute for an executed original-module check.

The broad event queue `80136F08/801EF484`, announcer decisions `80145724`,
sound event consumers `8013DE50/801433C4`, generic file/resource readers below
the UI load service and all phase entry/exit callbacks remain native/original
calls. In particular,
`8002E4F8` publishes a phase-read cache, `8010B60C` can publish a config cache,
and `8005C284` can drive game/menu work: none is hidden by a broad audio hook.
All down, distance, possession, score, spot, snap, actor and ball writes remain
in the full-RAM comparison. Audio RAM is also compared; there is no new RAM
exclusion. The gate checks that the independent original list and generated
native early returns agree before building/running the checker, and reports
per-entry call counts in successful original logs.

Only compilation and static audits have been performed by this author.
Original all-RAM agreement, mutant kills and the 40-second live run require
the owner's automatic gate on this exact revision. No case or retained queue
completion window has been removed, and the budget and timers are unchanged.

## Check and integration

Windows-capable owner runner, from the worktree root:

```sh
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_down_cycle.py
```

Private evidence/build directory:
`/mnt/w/Daywater/research-temp/nflstreet-down-cycle-20261016/`.
The author only invokes `build_windows_down_cycle_original_check.sh` to compile.
The gate pins inputs/protected originals/transitive sources and live integration;
runs two fresh processes per context; requires identical logs and positive
contact, bounds, touchdown, incompletion, commit and snap coverage; then builds
six one-line mutants from JSON in private source copies. Only an actual RAM or
defined-return mismatch counts as a kill. Crashes/rejections do not count.

The checker starts from saved RAM as-is, then 400 edited carrier, position,
ball state/contact, down, target, spot and score cases. Conditions missing from
the saved frame are constructed by editing actual fields, never by injecting
event-log records. There are 24 retained 96-update sequences crossing whistle,
queued commit, next-play lineup and snap. Separate dirty-destination and
threshold cases exercise result/down/distance writers. Every call/update
compares all 24 MiB outside the oracle stack. The original has only the twelve
documented audio/disc entry returns and never invokes native implementations.
The original-side host schedule is
independent of the native wrapper. These are required checks, not executed
results in this author's environment.

The live app links `down_cycle.c` and its generated include; its builder copies
the include explicitly and MSVC uses `/bigobj`. Every update runs the cycle
after ball publication. At whistle it logs
`down over: <reason> down <n> to go <yards> spot <yard line>` using the proposed
original result, whose callback subsequently commits it. On phase 5 it selects
and installs both teams' plays, logs `play selected: ...` again, positions the
lineups and requests the next snap. AI movement state is resynchronized after
lineup/snap; AI holds are cleared for fresh command queues. Controlled movement
reads the actual actor RAM each update and needs no partial-runtime reseed.

The failing live log was
`/mnt/w/Daywater/research-temp/nflstreet-live-smoke-codex-downs/smoke.log`.
After `play selected: 121 35_All Zone` and `play selected: 31 35_Posts`, frame
810 rejected the controlled player's requested movement. The old observed
movement wrapper only supports equal requested/active strengths and a limited
set of statuses; it is not a complete post-lineup actor update. Frames 1572
and 3383 separately held AI actors at `80144404 -> 8016ED94`: live AI still
used the older `ai_policy.c` closure, whereas the extended down closure already
contains that original speech callback and the audited audio service entries.

`nflstreet_down_control_init(&control,ram,actor)` now owns a full native policy
context, 32 input bytes and a MEM1 transaction. Once per tick,
`nflstreet_down_control_step(&control,dx,dy)` normalizes the host cardinal or
diagonal stick request, then runs original input `8003DDEC`, commit `800150E8`
and complete movement `800F86BC`, in that order. Neutral input also goes through
the original controller routine. The full wrapper handles resets, unequal
strengths, contact/animation states, kinematics/root motion and heading
publication. It runs once; no partial movement result is written back over it.
Source `8003DDEC` reads an explicit host token `FFF00000` mapped only to the
context's owned input bytes, outside MEM1. No gameplay address is used as live
scratch. A failed sequence restores every MEM1 write and emits its exact
call/PC/address/target; it is not concealed as a successful or idle update.

The live AI dispatcher, clip requests and channel/locomotion callbacks now use
`nflstreet_down_policy_init/call/service`, thin public entries into the same
extended native closure. Each AI context retains its existing transaction span,
SDK caller scope and rollback diagnostics. The old shared AI and movement
modules are unchanged. Source instructions, twelve service entry returns and
the established down corpus are unchanged; the host uses the already included
complete paths. No sound/disc service or gameplay event is added to the skip list.

The checker additionally retains its first real post-lineup and post-snap
states, then checks 401 saved/edited controlled-input cases against independent
original `8003DDEC -> 800150E8 -> 800F86BC` calls, plus two 96-update runs from
those retained states. Position and heading edits use real actor fields;
status bytes are retained from the original transitions. All MEM1 outside the
existing oracle stack is compared after each complete input/movement update.
The oracle constructs its own input bytes in its existing checker fixture;
it never calls the native controller or a native gameplay replacement. The
gate requires this extra coverage line and pins `live_ai.c/.h` along with the
other live sources. These added checks have been compiled, not executed here.

`NFL_LIVE_SCRIPT=1` now continuously drives the controlled player's main stick
toward positive X. In all three saved contexts that player is the active
carrier. The purpose is to run it across the sideline within the owner's
40-second smoke window. It uses verified movement input; it does not teleport,
force a whistle, inject a tackle or modify score/down flags. The owner's prior
run reached a contact whistle and subsequent selections; whether this revision
completes the 40-second run without movement or AI rejection remains for the
external live gate. No Windows executable was run by this author.
