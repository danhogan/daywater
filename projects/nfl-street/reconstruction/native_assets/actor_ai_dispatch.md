# Actor AI state dispatch and input commit

NFL Street, GameCube USA GNNE69 revision 0, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`.

**Confidence: candidate; Windows execution pending.** This author can write and
compile, but cannot run Windows executables. A successful build does not verify
original behavior. The external runner must run the gate; read its report before
promoting any claim below to verified. Dolphin build is not present in the
capture metadata. Saved contexts are pinned by SHA256 in the gate.

## Native scope and service boundary

`actor_ai_dispatch.c/.h` adds a big-endian RAM API. It uses `actor_ai.c` for the
assignment math and `locomotion_predicates.c` for the existing input selector.
No shared source was changed. The dispatcher is a reconstruction of the
state-table interpreter, **not a reconstruction of every policy in that table**.

| API | Original | Scope |
| --- | --- | --- |
| `nflstreet_ai_dispatch_step(c,index,commands,actor,controlled)` | `8007E0B4` | Optional controlled callback, update callback, completion, pop or removal after reentry |
| `nflstreet_ai_dispatch_enter(c,index,commands,actor)` | `8007DDE8` | Entry, recursive completed-entry pop, notification |
| `nflstreet_ai_dispatch_replace(c,index,commands,command,actor)` | `8007E650` | Exit acceptance, replace/enter or enqueue; command is a four-byte **value** |
| `nflstreet_ai_follow(c,actor,&result)` | `8008D988` | Selected-actor resolution, timer clamp, no-selected, self, eligibility, target/project, facing/interactions |
| `nflstreet_ai_eligible(c,actor,other,&result)` | `8008D654` | Types 1, 12, 19 and transition/service calls |
| `nflstreet_ai_face(c,actor,heading,weight,mode,slot)` | `8005335C` | Signed 24-bit delta, clamp, single multiply, fctiwz, signed product bits 24..55 |
| `nflstreet_ai_interact(c,actor,other,&result)` | `80021EF0` | Cone test, token/timer lifecycle; animation effects are services |
| `nflstreet_ai_commit(locomotion_context,actor)` | `800150E8` | Existing selector writes +210/+214/+218/+21E and requests original clips/events |
| `nflstreet_ai_input_finish(c,actor)` | `800F8214` | Requested floor, admission service, timer, code-specific caps, active code, transient clear |

The table descriptor is `*(*(r13-7860)+4*index)`. It holds a big-endian u16
capacity at +2 and a table pointer at +4. Each row is 24 bytes; entry/exit/
update/controlled callback pointers occupy +0/+4/+8/+C. Commands are four-byte
records. Pop and removal clear the **last type byte only**, preserving its other
three bytes. `+C0C` distinguishes an ordinary completed callback from one which
has already entered another state; the latter removes the original type instead
of blindly popping the new state. Entry re-reads the live table after callbacks.

`80039E14` supplies index zero, `actor+BE4`, and `actor+8 != FF` for the controlled
phase. Never replace every actor's state with assignment-follow: its row is
`5A` in the provided table, and the saved actors include rows `20`, `5C`, `1E`,
`02`, `1A`, `01`, and `21`.

The context is `{ram={span,memory,r13}, service,user,scratch,ok,unsupported}`.
`span` returns mutable bytes or NULL; `scratch` is caller-owned eight-byte RAM,
disjoint from actor/assignment/global data, used for target XY. Production code
must reserve that memory; the oracle's stack scratch is not a live allocation.
All APIs return 1 for supported execution and 0 for failure. This return value
is a host interface: original callback results are separate output parameters.
Writes before failure remain. Stop updates on failure. Recursion/capacity bounds
are host guards, not discovered gameplay constraints. Valid capacities 3..10
are exercised; larger invalid pointers/nonfinite arithmetic/FPSCR variations
are outside the check. Facing slots supported are 0 and 1.

Native callback bodies include `8007EA10/18/20` (constant return leaves),
`8008D954`, `8008DAC8`, and `8008D988`. Unknown PCs invoke the service provider;
a missing provider rejects with `unsupported=pc`. There is no idle fallback.
The production provider must implement original behavior on the same RAM.
The oracle DLL provider in the checker is testing infrastructure only.

Still external/unmodeled:

- All other state policy callbacks, including the complex saved `20`/`5C`/`21`
  behaviors. Play selection and assignment installation remain outside this
  module. Their policies must be supplied before all 13 non-controlled actors
  can run independently of the oracle. This patch alone does not enable that.
- `8009AD80` out-of-bounds policy in phases 4/7; other phases reproduce its
  rejection and `8002E4F8` cached-global write natively. `8009E590` eligibility
  event on matching type-19 assignment remains a service.
- Interaction resource creation `80021B58` and removal `80021AFC`, and reached
  notification services `80056348`/`800595BC`. Cone, handle and timer writes are
  native; effect internals are not. No guessed interaction animation is used.
- Clip requests/gameplay event effects from the existing locomotion selector
  require its existing providers. Input commit is not a universal memcpy:
  selector-specific behavior, guards, facing and clip requests must run.
- Contact admission `8005DCF8` during final input normalization, and reached
  avoidance policy `800F8088`. Both remain original-effect service boundaries;
  the common side-effect-free avoidance rejection gates are native.
- Movement, collisions, play clock, per-frame setup around
  `80039E14`, deferred command timer `80039900`, other match services and physics.
  Persistent tests retain state/inputs but do not advance original movement.

## Numerical and field evidence

Arithmetic follows the disassembly and `gekko_float.h`, with
`-ffp-contract=off`. Facing's weight multiply is `fmuls`, not double multiply;
its signed integer multiply selects bits 24..55. Cone angles use `80188D98`'s
polynomial. Eligibility distance squares Y first and fuses X squared into it,
then uses reconstructed `802165C8`; there is no host atan2 or sqrt substitution.

All ledger entries are **candidate** until the gate passes. Title/region/hash and
missing Dolphin build are recorded above. Multibyte scalars are big-endian.
Tests load each saved MEM1 and compare the whole 24 MiB outside the oracle stack
against the original after every actor operation, with fresh edited queues,
positions, angles, flags, inputs and table callbacks, then persistent sequences. Input finalization covers request codes 0..39,
dirty active output bytes, timers 0/global maximum/5 and finite strengths on
both sides of the original caps. Contact admission may change the input code
before normalization; the native code consumes the service result in RAM.
The saved observed types/bytes are described above; exact values and counters
are emitted in the private evidence logs. No fixed actor address is required.

| Field | Primitive / size | Readers/writers, interpretation |
| --- | --- | --- |
| actor+8 | i8 / 1 | `80039E14`: FF disables controlled phase |
| actor+BE4 / *(actor+310) | command records / 4 each | `8007E0B4`, `8007DD80`, `8007E650`, `8007E5A4`: current state/queue |
| actor+C0C | u8 / 1 | `8007DDE8` and `8007E0B4`: entry/reentry marker |
| actor+BE0 / +BDC | u8 / 1; i32 / 4 | `8008D988`: pending type 39 clamps timer above 30 |
| actor+150 / +154/+158 | u8 / 1; f32 / 4 each | entry/follow and existing assignment math; state union |
| actor+13C/+140, +144/+148 | i32 / 4 each | `8005335C`: scaled facing delta and supplied mode |
| actor+124 / +128 | u32 / 4 each | `80021EF0`: other actor token and timer 300; cone exit clears token |
| actor+1A8/+1AC / +1C0 | f32 / 4 each; u32 / 4 | XY and facing, readers only here |
| actor+200..+20F | fields / 16 | requested input; assignment callback writes through existing math |
| actor+20F / +21F | u8 / 1 each | `800F8214`: consumes requested code, writes active code, clears +20F |
| actor+21C | u16 / 2 | `800F8214`: code/flags/angular gates reset or decrement timer |
| actor+308/+309 | u8 / 1 each | `80060A50` in `800F8214`: codes 8/9 with nonzero speed write 1; 10/11 write 2 |
| actor+210/+214/+218/+21E | f32 / 4; u32 / 4 each; u8 / 1 | existing `800150E8`: active input commit, selector-dependent |
| r13-7CF4 | u32 / 4 | `8002E4F8`: observable cached phase word, including rejected policy |

The external runner's first attempt passed the saved suites and found a valid
edited-input mismatch in finalization (context 14, case 1, actor 3, actor+200).
The shared tail at `800F8654/8658` writes the normalized strength to **both**
+210 and +200 for all cap/default branches, including code 4. The native tail
now preserves both stores; code 30 first selects requested strength as its
source, then publishes the capped value to both. The generator is unchanged.
The `omit-finished-requested-strength` mutant restores the missing-store fault.
A fresh Windows gate result is still required; this is disassembly evidence
and a compile-checked correction, not a claimed successful execution.

The next external attempt reached context 14, edited dispatch case 2, actor 2,
then rejected nonfinite follow offsets. Its captured +154 word is `FFBBB4D5`
(NaN as f32), belonging to saved state 5C's union. The edited table had installed
`8008D988` without `8008D954` entry. Edited dispatcher/transition batches now
run native/original `8008D954` once on every actor before invoking the edited
table; persistent sequences do the same once before their first update. Each
entry is itself compared on all RAM. This supplies the actual state's required
initialization rather than discarding the resolved-target branch or allowing
nonfinite offsets. Saved-as-is dispatcher/transition cases remain untouched.
Standalone follow retains its existing real-entry setup. Production dispatch
still never initializes the union implicitly on each update. The edited table
also now contains valid original-leaf rows through command 44: eligibility
transitions to 21 and `8009E590` pushes 44, so an eight-row table was invalid
for these reached paths. Its empty-state exit accepts replacement, matching
the saved table, while nonempty rows still exercise exit rejection/enqueue.
These are edited interpreter fixtures, not reconstructed policies for those
states. Windows rerun is pending; native callback code and the finite-input
contract are unchanged.

The next context-14 process matched every RAM/effect comparison through all
401 cases and 24 persistent sequences (digest `eb430e61`), but the gate rejected
its removal coverage count of zero. The recompiled `8007E154/8007E1D0` calls
jump directly to the removal label within the same compiled chunk. Counting
only dispatch entries at `8007E5A4` therefore misses those calls. Removal
coverage now counts the original branch predicate at indirect callback return
PCs `8007E110` and `8007E190`: r3 must equal 1 and actor+C0C must be nonzero
(actor is the original preserved r30). This observes the original execution
without hooks, skipped instructions or native decisions. The gate requires
positive counts separately for controlled and update callback removal. Edited
resolved-actor cases now include odd as well as even indices to exercise both
phases. No saved input or acceptance comparison is weakened. That one process
is external evidence only; all-context repeats and mutant kills remain pending.

## Gate and private evidence

Windows-capable runner, from the worktree root:

```sh
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_actor_ai_dispatch.py
```

Private build/log/report directory:
`/mnt/w/Daywater/research-temp/nflstreet-ai2-20261015/`.
The author compiled the checker here but did not execute it. Gate result lines
and mutant kills are pending external execution, not claimed as passed.

The gate checks protected baseline hashes, exact input hashes, and transitive
source hashes over `native_assets/`-prefixed SOURCES. It runs two fresh processes
for each saved context 14/15/16 and requires identical logs. Each process checks
401 cases (saved plus 400 edits), eight modes, all 14 actors; then 24 sequences of
32 updates over 14 actors, state dispatch, selector commit and final input normalization, comparing after each call.
No final-frame-only comparison is substituted. Standalone follow enters its state
first because saved +154/+158 are unrelated state unions in some actors.
Edited dispatcher/transition batches and persistent sequences also enter follow
once before dispatch, since their edited tables install its callback. All entry
writes are compared. Other saved suites start from unedited saved input. Edited dispatcher tables use real
original callbacks; no synthetic PPC is injected.

**Explicit mixed boundary:** unknown state callbacks are executed by the
original module on the native side too, with normal original execution on the
oracle side. That establishes scheduler/transition behavior with original
policies, not native reconstruction of those policies. `original-backed policy
calls` in the summary makes this dependency visible. Edited callbacks and
follow/eligibility/facing/interaction lifecycle and final input code dispatch are native within the service
boundary above. The oracle never invokes a native implementation. Original
`80177EF4`, `80076C48`, `80076E18`, `80021B58`, `80021AFC` return at entry on both
sides, with full meaningful argument/weight traces compared. Everything else
runs normally. Those suppressed effects are not verified here.

`actor_ai_dispatch_mutants.json` lists ten one-line edits. The gate builds each
in its own private copied source tree and requires a RAM, return-value or effect
trace mismatch. Build failures, crashes, timeouts and native rejections do not
count. It tries all three contexts before declaring a mutant escaped. Mutation
results include edited-source, executable and log hashes in `verification.json`.

A subsequent external build failed when lld-link tried to overwrite the fixed
`actor-ai-dispatch-original-check.exe` path (permission denied). This is
consistent with Windows retaining an open executable. The gate now compiles
each baseline and mutant to a fresh `builds/checker-*/` directory underneath
its existing private output directory and runs that exact returned path. It
never overwrites or removes an older executable. The latest build diagnostics
remain in `build.log`; the report records the actual executable path as well as
its hash, including for mutants. This changes build artifact ownership only;
inputs, coverage requirements, all-RAM comparisons and live integration are
unchanged. Compilation can be checked without executing the gate or Windows
programs, as required for this author.

## Exact integration instructions (not applied to shared files)

In `native/live/CMakeLists.txt`, add `actor_ai_dispatch.c` and `actor_ai.c` to
`NFLSTREET_DRAWS` before its `list(TRANSFORM ... PREPEND ...)`. Retain existing
`locomotion_predicates.c` and `gekko_quaternion/quaternion_kernels.c` exactly
once. Apply the same live-only selector symbol definition to the new object:

```cmake
set_source_files_properties(reconstruction/native_assets/actor_ai_dispatch.c PROPERTIES
  COMPILE_DEFINITIONS "nflstreet_locomotion_select=nflstreet_live_locomotion_select")
```

Include
`actor_ai_dispatch.h` in `native/live/live_main.c` and initialize after the roster
and scene RAM are available:

```c
nflstreet_ai_dispatch ai = {
 .ram={scene.draw.span,scene.draw.memory,R13},
 .service=gameplay_ai_service,.user=gameplay_services,
 .scratch=reserved_target_xy
};
```

`gameplay_ai_service`/`gameplay_services` and `reserved_target_xy` above are
integration requirements, **not existing symbols or implemented providers**.
Do not link the checker/oracle DLL into the live app. An absent callback is a
hard unsupported state until its provider is implemented.

Inside the fixed 60 Hz update, after the rings update and before non-controlled
movement consumes this update's inputs, perform these phases:

1. For every `g=gameplay_actors+0x14D4*i` with its original actor+8 byte equal to
   FF, call `nflstreet_ai_dispatch_step(&ai,0,g+0xBE4,g,0)`. Leave its current
   queue installed; do not enter row 5A or decode assignments every frame.
   Stop if the return is zero and report `ai.unsupported`. If integrating
   controlled actor dispatch too, supply `(int8_t)actor[8] != -1` exactly.
2. Run `nflstreet_ai_commit(&selector,g)` for each supported AI actor, using the
   existing locomotion context and real clip-request/event providers. The live
   file renames the predicate selector symbol in its header, and CMake renames
   that module's exported symbol; the new CMake property above makes the
   wrapper call the same live symbol. Standalone oracle builds keep the normal
   public symbol. The existing
   controlled-player `nflstreet_live_locomotion_select(&selector,player)` stays
   in its established path; do not call both commit APIs on the same AI actor.
3. Call `nflstreet_ai_input_finish(&ai,g)` after selector commit and before
   movement. It normalizes the input as `800F8214`, including code-specific
   caps and clearing +20F. Report/reject a missing admission/avoidance service.
   Read **all 12 big-endian words** at `g+200` into that actor's own runtime
   input array. Use its own header, signed BB4/BBC ratings, +408 and 21-word
   +1A8 block; synchronize the runtime's retained state from RAM before each
   step because state/channel callbacks can write it. Call
   `nflstreet_actor_runtime_step(&ai_movement[i].runtime,inputs)` for supported
   moving branches. Zero-status/braking must use the verified brake branch. The full
   per-actor movement adapter is still required: the original movement tail
   clears +20E/+21E and publishes actor+1C0 at *(actor+31C)+8, while the bare
   runtime step only computes its bounded movement block. This module does
   not reconstruct or test that full movement wrapper. Propagate unsupported movement instead of feeding zero/host stick.
   Write runtime.actor[12] and runtime.state[21] back to +200 and +1A8. Never
   call `nflstreet_live_movement_step(dx,dy)` for AI: it regenerates the input.
4. Run the existing all-actor channel manager/step/pose passes. Keep per-actor
   runtime inputs/state synchronized after those callbacks, as the controlled
   path already does. The live file already places **all** actors with
   `nflstreet_actor_place(&d,&scene.hud.stacks,g,R13-0x7078u)` after pose and
   before ball motion; retain that order.

In `scene_frame.c`, **no AI call is needed**. Its draw-only
`nflstreet_scene_players` consumes the palettes/transforms published by the live
update; do not dispatch or advance gameplay from a draw pass.

These instructions identify the connection points. The remaining policy/effect
providers and movement adapter above must be supplied before claiming live
all-player AI. No shared-file edit or live smoke test is included in this change.
