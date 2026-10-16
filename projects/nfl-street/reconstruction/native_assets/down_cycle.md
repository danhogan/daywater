# Down cycle (candidate, external execution pending)

NFL Street, GameCube GNNE69 North America rev 0, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. MEM1 is big endian;
r13 is `803A9A40`. Capture contexts 14/15/16 are private inputs. Their Dolphin
build is unavailable. This author writes and compiles code only, and cannot
execute Windows programs. No all-RAM, mutation or live execution result is
claimed until the external gate passes this exact revision.

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
entry/exit/event functions remain transcribed. The nine named audio/disc service
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
cuts at audio requests, above their generic resource, OS and device calls.
It removes the earlier DVD/ARAM zero-read/discard-write fallback. Native device
addresses now fail the span check, and the original checker rejects external
reads/writes. There is no DMA, completion interrupt, audio clock or disc model.

The original checker lists exactly these nine original entry addresses. It
returns to guest LR before executing each listed routine, without calling
native code or writing guest RAM. The native function returns at the same
entry. Void entries preserve incoming registers. The sound-cue start returns
`7FFFFFFF`, its own disabled-audio return in `8013D8A4`; this means no voice
handle, rather than an invented successful request.

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
- **not modeled: audio/disc service (`801ABA64`, streamed sound control).**
  This is also called directly by ambient stream controls `80137B84/80137C08`.
  It checks audio flags `8034859C`, takes `802292C0/80229300`'s audio lock and
  calls `8022A7B4` to change audio channel state. No game object is supplied.

The write audit follows each entry's disassembly/decompilation and direct
callee closure, then checks ownership at the audio initialization and request
callers. Generic allocation, resource containers and OS synchronization below
these APIs serve their audio objects. Their direct closures contain only two
leaves below `80130000`: replay predicate `8000C360` and field mirror predicate
`80103E64`. Both consist exclusively of loads/register operations/branches,
with no stores, calls or indirect branches. The builder enforces that property
and records closure hashes and those leaves in `down_cycle_sources.json`.
This source audit is not a substitute for an executed original-module check.

The broad event queue `80136F08/801EF484`, announcer decisions `80145724`,
sound event consumers `8013DE50/801433C4`, generic file/resource readers and
all phase entry/exit callbacks remain native/original calls. In particular,
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
compares all 24 MiB outside the oracle stack. The original has only the nine documented audio/disc entry returns and
never invokes native implementations. The original-side host schedule is
independent of the native wrapper. These are required checks, not executed
results in this author's environment.

The live app links `down_cycle.c` and its generated include; its builder copies
the include explicitly and MSVC uses `/bigobj`. Every update runs the cycle
after ball publication. At whistle it logs
`down over: <reason> down <n> to go <yards> spot <yard line>` using the proposed
original result, whose callback subsequently commits it. On phase 5 it selects
and installs both teams' plays, logs `play selected: ...` again, positions the
lineups and requests the next snap. Movement state is resynchronized after
lineup/snap; AI holds are cleared for fresh command queues.

`NFL_LIVE_SCRIPT=1` now continuously drives the controlled player's main stick
toward positive X. In all three saved contexts that player is the active
carrier. The purpose is to run it across the sideline within the owner's
40-second smoke window. It uses verified movement input; it does not teleport,
force a whistle, inject a tackle or modify score/down flags. Whether this
reaches a whistle and a second play selection in 40 seconds remains for the
external live gate to establish. No Windows executable was run by this author.
