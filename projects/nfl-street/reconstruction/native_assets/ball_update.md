# Ball update on saved MEM1

Game: NFL Street, GameCube GNNE69 revision 0, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Saved contexts 14/15/16
are the existing Dolphin 2603a position-neutral captures. Private evidence,
executables, mutant source trees and logs:
`/mnt/w/Daywater/research-temp/nflstreet-ball-20261015/`.

Confidence: **probable from source/disassembly** pending the external gate.
This worktree compiles Windows executables but does not execute them. A
`verification.json` with status `compiled_only` is not original-module evidence.
Only a `passed` report covering the current source hashes establishes the
controlled original-module verification described below. No live-emulator or
native-window integration is claimed by compilation or that controlled check.

## Source boundary

`8002E0A4` calls `8002E144` for the earlier ball state/force pass
(`800BDFA0` -> `800C25B0`). Later, its `8002E198` phase calls
`800BE040` -> `800C24C8`: the callback at `8026F7A4 + state(+A4)*0x1C`
is `800C0FE4` for saved states 0 through 8. `8004EDFC` runs later and has
no direct ball physics call. This identifies the actual per-record motion
phase independently of the earlier state/force dispatch. No meaning is
assigned to opaque state IDs.

`nflstreet_ball_update` reconstructs `800C0FE4`: copy seven history words,
resolve association via `80020944` / `8003A4A0`, execute `800C0ECC` attached
or the bounded `800C0684` detached path, clear six force/angular words,
run the actual spatial triangle lookup `801087B8` / `8010862C`, and store
the resulting lighting-record pointer on the visual. Both attachment matrix
stack passes preserve the original scratch writes, including popped slots.
The reused file-based ball modules supply record/pose math; the new motion
kernel retains the original Gekko multiplier rounding for a double delta.

Detached coverage requires marker +9C > 0, +E0 = 0, +D0 = 0, angular input
+88 = positive zero, delta quaternion +60 = (0,0,0,1), finite values, and
0 <= delta < 28. It adds source-scaled forces to velocity, integrates
position using the instruction-order single fused operation, clamps the
field bounds, normalizes no-spin orientation and samples the target plane.
This is not a gravity producer: supplied force is integrated once and the
caller clears it. Unsupported wind/spin/ground-rest inputs return failure.

`nflstreet_ball_update_visual` reconstructs `800C09D8`: apply the original
side flip from `80103E64`, copy position and correct the ball orientation
into visual +04/+6C. It is a separate original phase from attached motion's
model attachment pose. `nflstreet_ball_update_matrix` supplies the live
connection: the original `801A3008` rotation stores at visual +20, followed
by position in the translation column. Translation publication is a **host
choice**, not a reconstruction of the complete scene-node transform walk.
The fourth matrix row is preserved.

## Fields and test procedure

All addresses below are evidence for this revision, r13 = `803A9A40`.
Pointers, integers and float32 lanes are big endian. Confidence remains
probable until the current-hash gate passes; interpretations are limited
to the measured operations, not gameplay enums or eligibility.

| Location | Primitive / size | Reader/writer, observed cases |
| --- | --- | --- |
| `*(r13-73D0)+8` (saved `803B6F08`) | 244-byte ball record | `800BDFA0`; saved first record, same record across 4 updates |
| record +00 | pointer / 4 | `800C0FE4`, visual drawn by `8015CB60` (saved `80DEEE24`) |
| +0C, +18 | float32[3], float32[4] / 12,16 | `800C0ECC`, `800C0684`, `800C09D8`; positions in [-80,80], supplied orientation and attached outputs |
| +28..+40 | float32[7] / 28 | `800C0FE4` copies +0C..+24 before update; edited sentinel history |
| +54 | float32[3] / 12 | attached displacement/delta or detached velocity; edits [-4,4], zero and nonzero delta |
| +60 | float32[4] / 16 | `800C0088`; identity delta branch only |
| +70, +7C, +88 | float32[3] each / 12 each | acceleration, supplied force, angular input; `800C0684`, `800C0FE4`; force [-10,10], attached angular [-2,2], clearing verified by mutant |
| +9C, +D0, +E0 | float32, uint32, float32 / 4 each | detached guards, original meaning deliberately unnamed |
| +B4 | uint32 handle / 4 | `80020944`; 14 actors, ignored high byte, zero and non-class-1 handles |
| `*(r13-7C30)` | actor-array descriptor pointer / 4 | `8003A4A0`; +00 base, +08 BE u16 row width, 0x14D4 stride |
| actor +04, +280 | pointer / 4, float32 matrix[16] / 64 | `80020C54`; model and attachment world matrix |
| model +18,+32,+40,+50,+60,+3AC | float32 / 4, int16 / 2, float32[3] /12, int32[3] /12, pointer /4, matrix /64 | scale, selector (-1 included), local offset, rotation, clusters, model root; `800D4BF4` / `80020C54` |
| `r13-73B0` | float32 / 4 | `800C1300` target plane, edits 0.5/-1.25 |
| `*(r13-7054)` | spatial-world pointer / 4 | `8010862C`; tree +98, triangles +1C, XY vertices +14 |
| visual +294 | pointer / 4 | `800C0FE4`; full unhooked triangle lookup, stale-pointer sentinel |
| visual +04,+6C,+20 | float32[3], quaternion[4], matrix[16] | `800C09D8`, `801A3008`, `8015CB60`; mirrored/unmirrored publication |

The default gate runs two fresh processes in each context. Each process
prepares 401 cases (case 0 starts from untouched saved RAM), with four
successive updates on independently retained original/native RAM. Every
RAM byte outside `[80F38000,80F47000)` (the oracle stack) is compared after
each of the physics, visual-pose and rotation-matrix phases. There are no
original hooks or native stand-ins. Matrix translations are copied equally
into the original fixture before the rotation-only oracle call and are
explicitly excluded from claims about original scheduling. Nine one-line
mutants are compiled in private source copies and must fail with an actual
RAM difference; crashes, compiler failures and native rejection do not count.
The gate hashes protected originals, inputs, local transitive sources and
ModernGekko ABI headers before/after; repeat logs must match.

```
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_ball_update.py
```

## Exact integration (shared files deliberately untouched)

Compile/link `ball_update.c`, `ball_update_motion.c`, `ball/ball_frame.c`,
`ball/ball_attachment.c`, `ball/ball_orientation.c`, `ball/ball_state.c`,
`matrix_stack.c`, `draw_engine.c`, and
`gekko_quaternion/quaternion_kernels.c`. Do not also link `ball_flight.c`:
`ball_update_motion.c` exports the same record motion API.

In `native/live/live_main.c`, include `ball_update.h` and initialize after
`scene_fixture`:

```c
nflstreet_ball_update_context ball_step={&scene.draw,scene.hud.stacks,R13};
const uint32_t ball_record=ram32(ram,R13-0x73d0u)+8;
```

Inside each fixed 60 Hz iteration, after movement, channel pose/skeleton
and actor placement are current, select the matrix stack and call:

```c
nflstreet_render_state unused={0};
nflstreet_draw d={&scene.draw,&unused};
if(!nflstreet_matrix_select(&d,&scene.hud.stacks)||
   !nflstreet_ball_update(&ball_step,ball_record,1.0)||
   !nflstreet_ball_update_visual(&ball_step,ball_record)||
   !nflstreet_ball_update_matrix(&ball_step,ram32(ram,ball_record)))failed=1;
```

Move the current controlled-player `nflstreet_actor_place` block into that
fixed-update loop before these calls so the ball consumes the current
player root. Other carriers need the same placement after their pose update.
`1.0` is one game tick, consistent with the live channel step; do not
substitute wall-clock seconds without reconstructing the original clock
producer. For a pool, iterate its byte count with record stride 0xF4 rather
than assuming only the first record. Ownership remains the saved or supplied
handle; this API does not make passing/catching decisions.

In `scene_frame.c`, keep simulation out of drawing. To refresh a visual's
matrix immediately before drawing, include `ball_update.h`, form
`nflstreet_ball_update_context u={&sc->draw,sc->hud.stacks,sc->r13}`, and call
`nflstreet_ball_update_matrix(&u,sc->ball.ball)` in `nflstreet_scene_venue`
before `nflstreet_ball_draw(&sc->ball,s)`. Omit this duplicate refresh if
live_main already publishes the matrix every update. Never call the physics
step once per rendered frame; rendering may run faster than 60 Hz.

Not modeled: full state dispatch, force generation, gravity scheduling,
spin/wind, ground-rest branch, contacts/bounce, release/launch/catch
eligibility, automatic ownership changes, full node traversal or fresh
match startup. Native failure may leave matrix scratch changed: callers
must stop/discard a rejected update, not treat failure as an atomic rollback.
