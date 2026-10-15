# Locomotion predicates and request selection

Confidence: **candidate** until the owner-run gate reports success. This
agent writes and compiles code only and cannot execute Windows programs.
Compilation is not original-module verification. No Dolphin integration or
live-app execution is claimed. A passing private `verification.json` applies
only when its source/input/baseline hashes match the current files.

Game: NFL Street, GNNE69 revision 0; DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Dolphin build: not supplied.
Source evidence: the matching lowercase-address `decompiled/` and
`disassembly/` files and `callgraph.json` under
`/home/dan/daywater/cache/binaries/46e3de854998a0f66c9be8c72a1164d22cd29eb4/`.
Private evidence/build directory:
`W:\Daywater\research-temp\nflstreet-locopred-20261015`.

`locomotion_predicates.c/.h` reconstruct `800150E8` and all eleven entries
of `80244D28` on a big-endian RAM span. The source for dispatch and angle
decisions is the exact handoff in `clip_requests.md`, checked against the
individual disassemblies. The request boundary is original `80177EF4`, not
an individual clip id. No animation choice is inferred from speed.

## Previously missing predicates

| Routine | Reconstructed reads, decisions and side effects |
| --- | --- |
| `8006510C` | Actor type, team/control list membership, selected team slot, role, phase, status, mode bits and flags. Includes `80064C0C`, `800BE4F8`/`80020944`, `8003A4A0`, team accessors, `80103D4C`, `800B7998`, `800B78A0`/`800EF6C4`, `800B78C8`, `800CE6E4`/`800CE930`. `8002E4F8` copies its state word into `r13-7CF4`; this write is retained. Flag `+C & 400000` can be set or cleared. |
| `80014644` / `80061158` | Current group `21`, actor flag 4 and manager-record heading versus selected heading. Always clears `+20E/+21E` on the `21` route, including when blocked. `8017847C` returns record+10; `80061158` reads its **+4**, thus manager-record **+14**, not +10. |
| `800146CC` / `8001454C` | Status exemptions, selected magnitude, two wrapped heading distances, team ownership, speed and `8004F214` decide suppression. |
| `8004F214` | Saved RNG result scaled in the original single-precision order by the signed +BB4 value; status/parameter-index probability tables; possible second RNG call and signed threshold interpolation. `8002BD94` returns 1. Calls gameplay event service `80076C48` or `80076E18` with mode 3/4/5 when required. |
| `801EE1C0(0)` / `80188628` | All three original generator types (`80188008`, `801881D0`, `80188490`), their state writes, clamps and both wrapper counters. No host random source. |
| `80014750` / `80014898` | Neutral, forced heading, ownership, directional group 49/54/A0/9D/9F, magnitude floors, selected lanes and restart. Keeps signed H comparison and old selected heading. `80178638` reads the matching record delay. |
| `8005335C` | Wrapped signed 24-bit turn clamped to +/-155555, single multiply, `fctiwz`, signed 64-bit product and bit extraction; writes actor +13C/+144. |

The code uses `gekko_float.h` with `-ffp-contract=off`. It preserves the
multiply then divide in magnitude floors, random scaling, two single
roundings before the final suppression comparison, and the turn helper's
single rounding before integer conversion. It does not simplify these
expressions using real-number algebra.

## RAM evidence and interpretation

These are source-traced fields, all **candidate** until the full gate passes;
their names describe access roles rather than independently inferred game
semantics. All multi-byte values below are big-endian. The test procedure is
saved-as-is plus controlled edits compared with the original as described
below; observed saved values and resulting digests remain in private logs.

| Address/chain | Primitive/size | Readers/writers and interpretation |
| --- | --- | --- |
| `g+200..22F` | 12 x u32/f32; selector/status bytes +20E/+20F | Input and selected movement lanes; all handlers above. +204/+208 are 24-bit wrapped heading words; only `80014898` masks the stored input words. +218 is intentionally unchanged on some selector-8 paths. |
| `g+1A8..1FB` | 21 x u32/f32 | Movement block; direct selector reads are +1C0, +1C4, +1C8, +1EC. `8006510C` can compare +1C4. |
| `g+0,+2,+3,+8,+C` | u32 header, u8 team/type/input slot, u32 flags | Actor lookup and predicates. Header's low byte is +3. Flag writes affect bits 4 or 400000 only. |
| `g+310 -> byte 0`; `g+B5E/+168` | pointer/u8; u8/u8 | Gameplay status and predicate inputs. |
| `g+318/+31C/+320` | 3 x u32 pointers | Manager, channel set, channels. Read the stored pointers; do not substitute actor-relative offsets. |
| manager `+4/+6/+C/+14+100*i` | u16 key/state, f32 delay, u32 heading | `801784B8`, `8017847C`, `80178638`, `80061158`; four records. |
| `g+408`, `g+30C -> +8`, `g+BB4` | u32, pointer/u32, s16 | Selector-5 restriction, suppression table index (default 3 if pointer null), suppression scalar. |
| `g+13C/+144` | u32/u32 | `8005335C` turn result and mode 2; real non-stack writes. |
| `r13-7C30 -> +0/+8` | pointer/u16 | Actor base/team stride count; original actor stride 14D4. |
| `r13-7078 -> +34/+38` | u32/u8 | Inhibition bits and side; ownership is +38 xor 1, or zero for null world in `80103CA0`. |
| `r13-5A98 + team*7878 +38/+D1C` | pointers to team records | Selected slot byte +16 and phase word +14 / byte +17. |
| `r13-73D0 -> byte0, +BC+F4*i` | u8 count, u32 actor token | Control membership lookup. Null list gives false. |
| `r13-7CF8 -> word0`; `r13-7CF4` | u32 pointer/cache | `8002E4F8` read and observable cached write. |
| `r13-7328 -> byte0` | u8 | `800CE6E4` bit0 and `800CE930` bit1. |
| `r13-70C4/-70C0/-70BC` | f32 x3 | Predicate speed scale and directional thresholds. |
| `r13-7AF8/-7AF4/-7AF0` | s16/f32/f32 | Suppression secondary thresholds. |
| `r13-61F8 -> +14 -> word0` | pointers to RNG state | Generator type u32; state layout follows each original generator. Wrapper +8/+C are u32 counters. |

## API and limits

Initialize `nflstreet_locomotion_context` with span, memory, request, effect,
user. `nflstreet_locomotion_select(&ctx,g)` executes exactly one actor's
handler. Return 1 means completed; 0 is a memory/provider failure; -1 means
an unsupported selector outside 0..10. `ok` and `unsupported` diagnose a
failure. Writes before a failure remain. Callers must propagate failure.

`nflstreet_locomotion_gameplay(&ctx,g,&value)` and
`nflstreet_locomotion_suppression(&ctx,g,&value)` expose the two stateful
predicates for independent checks. They return access/provider status and
put the original boolean in `value`. Do not pre-call them before selection;
that would repeat their writes/RNG calls.

The request callback receives weight and the actual manager, set, channel,
key and actor arguments. A changed key or restart invokes it immediately
after the handler's writes. A suppressed/unchanged request does not. The
callback must call the existing clip-request implementation with its
existing group/event providers to produce channels; this module does not
repeat the clip manager or callback internals.

**Unmodeled:** the two downstream gameplay event services `80076C48` and
`80076E18` and their state-machine, sound and notification effects. The
effect provider receives the exact function token, actor and mode. A null
provider rejects an invoked service; it does not silently return success.
The checker explicitly records and returns at these original entries.
Consequently its all-RAM result proves predicate and selector writes up to
these service boundaries, not complete execution of those event systems.
Live integration needs a real event provider or must stop on this branch.
Input acquisition, physics, gameplay state progression, arbitrary new RNG
types, clip callbacks, channel/pose stepping and drawing are outside scope.

## Gate

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_locomotion_predicates.py
```

The default command builds a fresh executable, runs it twice in independent
processes for each saved context 14/15/16, compares complete logs, then
**builds and runs all ten mutants**. Compilation and Windows execution logs
and provenance go only to the private directory. `--build-only` compiles the
base and all mutants, writes `compilation.json` and never runs Windows code
or records a verification pass. Every link uses a new private directory.

Each context has 5,614 isolated full-selector comparisons (14 actors x 401
cases), plus 401 independent comparisons of each stateful predicate. Case
zero uses the saved RAM unedited. The 400 edited cases per actor change
every word of +200 and +1A8, selected lanes, header/status/flags, manager
keys/states/delays/heading, indirect manager/set/channel pointers, control
membership, world mode/phase/cache, both directional thresholds, parameter
index, signed scalar and RNG state. Boundary angles and directed guard,
suppression and flag-clear paths complement the deterministic random edits.
Distinct handler weight constants prevent equal saved 1.0 values from
hiding a wrong load. Probability-table edits force both event outcomes and
the no-event result. All three RNG types and all eleven selectors must run.

Only `80177EF4`, `80076C48` and `80076E18` return at their original entry;
their ordered function/arguments and request float bits compare. No native
stand-in executes in the oracle. Each invocation compares **all 24 MB RAM
outside 80F38000..80F47000**, not just known writes; boolean returns also
compare for independent predicate cases. Baseline and input hashes are
checked before/after, along with transitive source hashes using
`native_assets/`-prefixed entrypoints. Mutation success requires an actual
comparison mismatch (exit 7); crashes/rejections/build failures never count.

## Exact integration (shared files intentionally unchanged)

Compile `locomotion_predicates.c` into the live target, add its include, and
keep the existing `live_clips` request/group/event adapters. Define:

```c
static int selected_request(void *u,const nflstreet_locomotion_request *r){
 nflstreet_live_clips *clips=u;
 return nflstreet_clip_request_group(&clips->requests,r->weight,
   r->manager,r->set,r->channels,r->key,r->actor)&&clips->callbacks->ok;
}
```

After `nflstreet_live_clips_init` in `native/live/live_main.c`, initialize:

```c
nflstreet_locomotion_context selector={
 channels.span,channels.memory,selected_request,gameplay_effect,&clips,0,0
};
```

`gameplay_effect` must implement the two services named above, returning 0
for an unsupported service. Its user argument is `&clips`; use a containing
app context instead if the real service provider needs other state. Passing
NULL deliberately rejects reached event branches; it is not complete live
gameplay support.

In the fixed-update loop, replace the single call
`nflstreet_live_clips_movement(&clips,player)` with
`nflstreet_locomotion_select(&selector,player)` **after both loops restoring
`movement.runtime.actor[12]` to player+200 and `state[21]` to player+1A8,
before every `nflstreet_actor_channels_manager/step/pose` call**. Treat any
result other than 1 as a diagnosed failure. Remove the `request<0` host-pose
selection and its speed-derived fallback from this path. The selector uses
RAM world state plus the actor blocks; do not pass host speed as a selector.
Its writes to +200/+210 must survive until callbacks/physics consume them;
sync these words back into persistent `movement.runtime.actor` before its
next step if that runtime otherwise overwrites them from an old copy.

`scene_frame.c` is a render-only API, so add **no selector call inside
`nflstreet_scene_players` or any draw pass**. Its caller should run the
selector at the 8003A1F4 update phase, then channels at 8003A24C, then visual
placement/scene rendering as already ordered. For a full actor update,
iterate `u32[u32[r13-7C30]+0]`, u16 count at table+0A, stride 14D4, invoking
the selector once per gameplay actor after inputs are available. The live
controlled-player path can use `player` directly. No shared-file changes
were made; the changes described here require the owning integration task.
