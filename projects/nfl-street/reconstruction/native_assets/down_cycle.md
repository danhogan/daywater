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
entry/exit/event functions themselves are transcribed, never silently skipped.
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

### Outstanding ARAM completion wait

The owner's failed context-14 run reaches case 401, stage 570 and exhausts
the native instruction budget at `80213CB4`. That address is inside the
original signed 64-bit division helper `80213C18`. **Probable cause from
source:** synchronous ARAM wrappers `8017BA98` and `8017BB78` call that helper
while polling time and `*(r13-5788)` after submitting an ARQ request through
`801FF290`. With fixed time and discarded DMA register writes, neither a
transfer nor its completion occurs. Raising the budget cannot complete this
wait. This report does not yet identify which wrapper/caller the failed run
reaches, or whether it requests a read or a write.

`801FF154` is the original ARQ completion service, registered through the
ARAM interrupt callback at `*(r13-5408)` in the saved contexts. It calls the
queued callback and advances the original queues. A future hardware boundary
must preserve that path and supply actual transfer data; simply changing the
completion counter would falsely acknowledge work. The MEM1 inputs do not
include ARAM. No zero-filled ARAM, forced counter change, skipped game
function, reduced case set, timer edit or increased budget is introduced.

On budget exhaustion the checker now logs the current active/pending ARQ
request words and candidate source stack backchains from its owned PPC stack.
These are failure-only observations: the request may live on that stack,
and the low-priority driver may already have advanced its addresses/count.
The backchain candidates are explicitly diagnostic, since older stack frames
can survive a call. The next owner run is needed to identify the actual
transfer and caller before selecting a data/completion model. The transfer
wait remains unresolved; all-RAM, mutant and live gate success is not claimed.

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
compares all 24 MiB outside the oracle stack. The original has no hooks and
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
