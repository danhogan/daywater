# Ball ownership event consumers on MEM1

NFL Street, GameCube GNNE69 revision 0; DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Dolphin 2603a saved contexts
14/15/16, r13 `803A9A40`. Private executables, mutant trees and reports:
`/mnt/w/Daywater/research-temp/nflstreet-ballplay-20261015/`.

Confidence: **probable**, reconstructed from decompilation and instruction
order, pending the external original-module gate. Compilation is not
verification. Only a `passed` report with current source/input/baseline hashes
establishes the bounded original-module comparisons below. No live Dolphin,
fresh-match gameplay, automatic possession or window integration claim.

## Discovered routes and implemented boundary

The ball's current carrier handle is +B4. `80020988` writes actor headers
through pointers, so searching only direct `stw ... 0xb4` misses these writers.
`800BE2B4` assigns +B4/+B8; `800BE204`/`800BE264` clear +B4 **after**
`800C25FC` exit/entry callbacks. An exit from state 1 copies the current +B4
to +CC. Assignment writes the new header before that exit; release clears it
afterward. This ordering is observable ownership history, not a cosmetic detail.

`ball_handoff.c` supplies full span-based `800BE2B4`, retaining the earlier
file-based `ball_ownership.c` and `ball_lifecycle.c` kernels. It also reproduces
both original matrix-stack passes, including their popped scratch slot,
actor flags, cached context query, optional `8014629C` store, and visual pose.
`800815B4` is reconstructed completely: active-pool assignment, `8008131C`
target setup through `8008D278`, the signed actor-stat boundary, and byte
+180 clearing. Calling this the snap event is **probable**, not a verified
controller mapping. The explicit-speed branch is the one reached by the
original constant; the random/angle branch of general `8008D278` is rejected.
Target arithmetic uses `fadds`, `fsubs`, `fdivs` in disassembly order.

Bounded `800C25FC` supports exits 0/1/2/3/5/6/8/9, entries 0/1/2/3 and
entry 5 with argument 0 (its callback returns before the event services).
Both release variants preserve their distinct velocity behavior. State IDs
remain opaque. `8007BEE8` supports a null ball and queried context other
than 3; context 3 is rejected before mutation because it requires the full
event/log/controller services. This bound is explicit, not a skipped oracle
callback. All original routines run untouched, with **no hooks**.

The gameplay path `8002E144 -> 800BDFA0 -> 800C25B0` runs state/force
callbacks before the later motion phase `8002E198 -> 800BE040 -> 800C24C8`.
The event callback table starts at `8026F79C+state*1C+4` (equivalently
`8026F7A0+state*1C`); it is dispatched by `800C26B4` from collision code
`800BF168`. Those are separate phases; the motion step does not discover a
new owner on its own.

The source/callgraph trace identifies these further routes, **not modeled**:

| Route | Evidence and missing work |
| --- | --- |
| Snap/pitch producer | `80096D84` calls release with state 2 or 3, then trajectory generation `800C1410`, target/header +C8 and event services. These state numbers do not establish universal gameplay enums. |
| Handoff | `8007BEE8` calls assignment and clears +C4; queried context 3 also logs an event and updates controller ownership. `800C1DF8` is another assignment/event route. |
| Catch | `800C2374 -> 800FBC58 -> 800FD600 -> 800BE2B4`; eligibility uses actor channels, wrist geometry, random outcomes and match flags. `800FD600` also handles interception/scoring/teammate events. |
| Pitch/loose pickup | `800C2428 -> 800FC148 -> 800FD600` and `800C2264 -> 800FD4F8 -> 800FD600`, with geometry, clock and actor eligibility checks. |
| Fumble | `800FD334` transitions to state 5, argument 1, clamps velocity and calls `800C14E4` plus event services. It is **not** equivalent to simply clearing +B4. Entry `800C1874` may call the roster/event service `800786FC`. |

Consequently this module consumes known original events; it does not yet make
natural catch, handoff, pitch or recovery decisions. Do not synthesize those
decisions from distances, opaque states or button guesses. Full state/force
dispatch, collision enumeration, launch forces/spin and event/scoring services
remain open. The controlled gate is not an end-to-end gameplay-update oracle.

## Memory evidence

All fields are big endian; pointers/uint32/float32 have size 4 unless listed.
Confidence stays probable until current-hash verification passes. Addresses
refer only to this revision. The test edits these fields, observes native and
original results and compares the complete RAM, not just the listed outputs.

| Address/chain | Type and size | Readers/writers; observed inputs and effects |
| --- | --- | --- |
| `*(r13-73D0)+8+pool[1]*F4` | ball record, 244 bytes | `800BDDC4`, `800815B4`; active-pool record selection |
| ball +A4/+A8/+AC/+B0 | uint32 each | `800C25FC`; old states 0/1/2/3/5/6/8/9, new 0/1/2/3/5, argument/history writes |
| ball +B4/+B8/+C8/+CC | actor handles | `800BE2B4`, releases, `80020944`; all 14 actor headers, null old carrier and changed owners |
| ball +BC/+C0/+C4/+D0 | uint32 each | assignment, exit 1 and handoff event; counters, clock delta, reset/history effects |
| ball +08/+E0 | uint32 flags / float32 | lifecycle exit 6/8 and entry 1; edited sentinels |
| ball +0C/+18/+54/+7C | float32[3]/[4]/[3]/[3] | assignment attachment and release; edited positions [-80,80], velocity [-4,4], force [-10,10]; nonzero velocity deliberately reseeded before release |
| `*(r13-7C30)` | actor array descriptor | `8003A4A0`; +00 base, +08 uint16 row width, actor stride 14D4 |
| actor +00/+04/+0C | header/pointer/flags | assignment; all real identities, edited flag including 08000000 |
| actor +280 | float32 matrix[16] | attached update; translations [-40,40] |
| model +18/+32/+40/+50/+60/+3AC | scale / int16 selector / float32[3] / int32[3] / pointer / matrix | existing attachment kernels; original selectors and -1, scale [.75,1.25], local offsets [-.5,.5], edited root translations |
| visual +04/+6C | float32[3]/[4] | assignment model pose, separate from ball record orientation |
| `*(r13-61F0)` | pointer to uint32[2] | `801EE7F8`; null clock gives 0; end-start 82 and wrapping 48, incremented across updates |
| `*(r13-7CF8)`, `r13-7CF4`, `r13-6A00` | context query/cache/actor header | `8002E4F8`, `8014629C`; values 0/1/2/3, stale cache/header sentinels |
| `*(r13-7078)+0C/+10` | float32 XY | `801039D4`; [-40,40] feeds snap target offsets |
| actor +150..+17B, +180 | target block / byte | `8008131C`, `8008D278`, `800815B4`; offsets, speed and zeroed bytes/words |
| actor +1C0/+1C4, `r13-70C4` | angle / float32 / float32 | target setup; supplied angle, rate [0,12], divisor [.5,2] |
| actor +200/+204/+208/+20E | float32 / uint32 / uint32 / byte | `8008D278`; cached rate/heading and byte 1 |
| actor +BBC | signed int16, 2 bytes | `8008131C`; -32768/-1/0/149/150/151/32767, speed override only <150 |

## Gate

```
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_ball_handoff.py
```

The default builds the check and launches two fresh processes per saved
context 14/15/16. Each process has 401 cases, each retained across four
updates. Case zero's first operation consumes untouched saved RAM. Other
cases edit all 14 actors, carrier/history, position, velocity, flags, context,
clock, selector and snap actor state. Every transition, assignment, release,
snap and supported event is checked separately: 7,484 complete MEM1 comparisons
per process, excluding only `[80F38000,80F47000)` oracle stack. There are
1,068 context-bounded events. Original routines and their callbacks are never
patched. This exercises event services across successive updates; it does
not execute four complete original game frames or ball-motion updates.

Ten one-line JSON mutants are compiled separately and must exit with a real
RAM difference. Compiler failure, crash, original failure or native rejection
does not count. The gate checks protected baselines, immutable RAM inputs,
transitive source/header hashes (native_assets-prefixed entrypoints), and
repeat-log identity. `--build-only` compiles all eleven executables, marks
the report `compiled_only`, and never executes Windows code. That is the
only mode run from this worktree by the author; default execution belongs
to the external gate runner.

## Exact integration; shared files untouched

Link `ball_handoff.c`, `ball/ball_ownership.c`, `ball/ball_lifecycle.c`,
`ball/ball_attachment.c`, `ball/ball_orientation.c`, `matrix_stack.c`,
`draw_engine.c`, `gekko_quaternion/quaternion_kernels.c`. Reuse linked copies
when the motion module already supplies these dependencies.

In `native/live/live_main.c`, include `ball_handoff.h`. After the current
actors' movement, pose/skeleton and placement have updated, and **before**
the existing `nflstreet_ball_update` calls within each fixed 60 Hz update:

```c
nflstreet_ball_update_context ownership={&scene.draw,scene.hud.stacks,R13};
/* Select the original matrix stack before any attachment operation. */
nflstreet_render_state unused={0};
nflstreet_draw draw={&scene.draw,&unused};
if(!nflstreet_matrix_select(&draw,&scene.hud.stacks))failed=1;
```

At an **already decoded** original `800815B4` event, call
`nflstreet_ball_handoff_snap(&ownership,actor_address)`; it resolves the
active ball itself. At an already decoded `800BE2B4` event, call
`nflstreet_ball_handoff_assign(&ownership,ball_record,actor_address)`.
At a decoded `800BE204`/`800BE264`, call
`nflstreet_ball_handoff_release(&ownership,ball_record,state,argument,0/1)`
with the original state/argument registers. For `8007BEE8`, call
`nflstreet_ball_handoff_event(&ownership,ball_record,actor_address)` only
within its documented context boundary. Stop on any zero result. Then run
the existing motion, visual-pose and matrix publication in that order.
No host frame delta is passed to these event APIs; they read the original
clock object. Upstream original event decoding is not implemented, so these
calls cannot by themselves turn the current live host into playable football.
In particular, never call the snap/assignment once every fixed update.

In `scene_frame.c`, add **no ownership call**: ownership belongs in the
fixed gameplay update, not in drawing. Keep its existing ball draw and matrix
publication; the normal `ball_update` path consumes the newly written +B4.
No shared-file changes are needed to build the check. Live event decoding
and scheduling are the required future shared-file integration work.
