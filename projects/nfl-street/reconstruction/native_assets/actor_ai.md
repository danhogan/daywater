# Assignment-follow gameplay inputs

NFL Street, GameCube GNNE69 revision 0, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`.

**Evidence status: candidate reconstruction; Windows oracle execution pending.**
This agent is allowed to write/compile code and cannot execute Windows programs.
Compilation is not original-module verification. The external runner must run
the gate below before treating any field or behavior here as verified.
The gate's private report records exact source, executable, input and baseline
hashes, coverage, repeated process logs and actual mutant kills.

## Reconstructed boundary

`actor_ai.c/.h` expose four functions over a big-endian RAM span:

| Native function | Original | Behavior supported |
| --- | --- | --- |
| `nflstreet_actor_ai_decode(c, actor, output)` | `8008D5DC` | Decode signed assignment bytes to two offset floats |
| `nflstreet_actor_ai_enter(c, actor)` | `8008D954` | Clear `+150`, decode assignment into `+154/+158` |
| `nflstreet_actor_ai_target(c, actor, other, offset, output)` | `8008D764` | Scale the X offset near the field edge, add offsets to other's XY |
| `nflstreet_actor_ai_project(c, actor, target)` | `8008D864` | Target XY minus own XY, angle, distance, requested heading/strength/status |

`c = {span, memory, r13}`; `r13 = 0x803A9A40` in the saved contexts.
The span returns mutable bytes for an original address and requested size, or
NULL. Functions return 1 on success, 0 on unsupported memory/arithmetic. Native
rejection is a host interface, not an original game behavior. Inputs and outputs
must be valid nonaliasing spans. Finite, non-overflowing arithmetic is supported;
nonfinite positions, infinite distances, arbitrary NaN payloads/FPSCR and invalid
pointers are outside the boundary. Coincident finite XY is supported: the
original atan polynomial's `0/0` reaches `fctiwz`, then wraps to heading zero.

The decompilation of `8008D764` omits its live arithmetic. Its disassembly shows:
for positive offset X use `width - own_x`, otherwise `width + own_x`, subtract
the constant at `80250A6C`, divide by `80250A70 - 80250A6C`, clamp to
`[80250A68, 80250A6C]`, then multiply only offset X. Width is the float at
`*(*(r13-7078)+90)` (`801046CC`). `801DE560` writes **eight bytes**, not twelve.
No target Z is generated or read. The older bounded `actor_target_input/`
implementation is not modified or used.

`8008D864` uses the original `80188D98` polynomial, in instruction order;
`801DDFCC` squares Y, fuses X squared into that result, then calls `802165C8`.
The existing reconstructed MSL square root is reused. Scalar operations use
`gekko_float.h`, with `-ffp-contract=off`; target scaling, division, the distance
strength FMA and angle polynomial preserve the disassembled rounding points.
Constants and polynomial coefficients are read from RAM, not host substitutes.

With saved constants: distance <= 0.3 gives strength 0; (0.3,1] gives 0.14;
above 1, scale `(distance-1)*0.25`, capped at 1, then fused
`scale*0.8600000143051147 + 0.4000000059604645`. Maximum requested strength is
about 1.26; later gameplay routines may cap/commit it, outside this module.
`+20F` is only set above 0.7200000286102295 and is **preserved** otherwise.
Clearing it here would break the original input lifecycle.

## Callgraph evidence and remaining work

`callgraph.json` names `8008D988` as the caller of both `8008D764` and
`8008D864`. Its disassembly/decompilation orders them at `8008DA60/8008DA6C`.
This is one assignment-follow state, not a universal non-controlled actor AI.
`8008D954` is its assignment entry helper (indirect state dispatch is not
represented by a direct caller in the callgraph).

Unmodeled:

- `8008D988` itself: selected actor resolution (`800BE3F0`, `800BDDC4`,
  `800BE380`), initial predicate `8009AD80`, assignment lookup `8003B054`,
  timer clamp, no-other branch, self-target branch and transition `8007E650`.
- Eligibility `8008D654` (assignment types 1, 0x12 and 0x19 and its transitions).
- Following `8008D548`: facing `8005335C` and interaction `80021EF0`.
- Play calls, assignment changes, choosing which actors use this callback,
  choosing `other`, input clearing/committing (`+210..+22F`), tackle/pass/block
  policies, collision and football physics.
- Position advancement. Persistent oracle tests use supplied world samples;
  they are not multi-frame original game simulation or proof of live movement.
- Other input-producing callbacks. Do not apply this state to all 13 actors.

## Field evidence ledger

All rows below are **candidate** until the gate passes. Game/hash as above;
region USA; Dolphin build not recorded in the provided capture metadata.
The original-module build is pinned by `verify_controller_checks.BASELINES`.
All scalar multibyte fields below are big-endian. `a`/`b` are explicit gameplay
actor pointers; the saved roster is `*(*(r13-7C30))`, stride `0x14D4`, with count
14 at roster manager `+0A`. No fixed runtime actor address is required by the API.

| Field / pointer | Primitive / size | Readers/writers and observed interpretation |
| --- | --- | --- |
| `*(a+310)+1/+2` | signed i8 / 1 each | `8008D5DC`: offset source, saved scale 0.0625 |
| `a+150` | byte / 1 | `8008D954` clears on state entry; no boolean gameplay name assigned |
| `a+154/+158` | f32 / 4 each | `8008D954` writes, `8008D764` reads XY offsets; union in other states |
| `a+1A8/+1AC` | f32 / 4 each | `8008D764/864` own XY; no writes here |
| `b+1A8/+1AC` | f32 / 4 each | `8008D764` target actor XY; no writes here |
| `*(*(r13-7078)+90)` | f32 / 4 | `801046CC` supplies width term to `8008D764`; exact gameplay interpretation is probable from arithmetic |
| `a+200` | f32 / 4 | `8008D864` requested strength, tested close/ramped/capped ranges |
| `a+204/+208` | sign-extended i24 in i32 / 4 each | `8008D864` requested heading; output of `80188D98` |
| `a+20E` | byte / 1 | `8008D864` writes 1 |
| `a+20F` | byte / 1 | `8008D864` writes 1 above threshold, otherwise leaves old value |

Test procedure for every field: load a pinned saved MEM1, prepare the same
inputs for native/original, execute the addressed unhooked original and native
leaf, compare all 24 MiB outside the oracle stack `80F38000..80F46FFF`.
Saved-as-is inputs are included (only guarded scratch output is poisoned).
Edited cases test all signed assignment byte values, both offset signs, width
clamp endpoints/interior, randomized XY, zero distance, threshold neighbors,
far targets, dirty input bytes and edited RAM constants. Standalone target
case zero skips nonfinite saved union lanes; composed cases enter the state
first for all actors, including the saved-as-is case.
Coverage counters are diagnostic; the actual acceptance criterion is bytewise
all-RAM equality. Read `original-14-0.log` etc. for observed values/counts and
digests rather than inventing results before execution.

## Integration gate

From the repository root, by the Windows-capable external runner:

```sh
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_actor_ai.py
```

Private evidence/build directory:
`/mnt/w/Daywater/research-temp/nflstreet-ai-20261015/`.
Nothing in that directory is committed. The gate compiles its own checker,
checks original baseline and exact saved-input hashes before and after, hashes
`native_assets/`-prefixed SOURCES and their transitive headers, runs **two fresh
processes for each context 14,15,16**, and compares deterministic log hashes.
Each process has 401 cases per isolated/composed mode, 14 actors (subject to
the saved union exclusion above), plus 24 persistent sequences of 32 updates
over all 14 actors. No original functions are skipped or replaced.

`actor_ai_mutants.json` lists ten one-line faults. The gate copies sources into
private scratch trees, applies each edit exactly once, compiles and runs it,
and requires an actual all-RAM mismatch (exit 7 and mismatch diagnostic).
Compilation errors, crashes or native rejection do not count as a caught mutant.
The gate fails if any mutant survives all three contexts. Names cover unsigned
assignment, entry marker, width side, upper clamp, target Y sign, angle axes,
close threshold, strength saturation, split FMA and clearing the preserved byte.

## Exact integration contract

Build/link `actor_ai.c` plus the already existing
`gekko_quaternion/quaternion_kernels.c` once. Include `actor_ai.h`. Continue using
`-ffp-contract=off`. No shared file changes are included in this patch.

In `native/live/live_main.c`, initialize once beside the gameplay roster setup:

```c
nflstreet_actor_ai ai = {scene.draw.span, scene.draw.memory, R13};
```

For a gameplay actor `g` whose **actual state has been established to be this
assignment-follow state**, call `nflstreet_actor_ai_enter(&ai,g)` only when
entering that state, after its `+310` assignment pointer/bytes are installed.
Within the fixed 60 Hz update, before movement consumes its input, supply the
resolved other gameplay actor `other` and a reserved, nonoverlapping 8-byte
MEM1 scratch address `target`:

```c
if (!nflstreet_actor_ai_target(&ai,g,other,g+0x154u,target) ||
    !nflstreet_actor_ai_project(&ai,g,target)) failed=1;
```

This corresponds precisely to the calls at `8008DA60` and `8008DA6C` (the original
target lives on its stack). Scratch is caller-owned RAM; do not reuse the oracle
addresses blindly in live memory. Alternatively, when an independently verified
target selector already supplied target XY in RAM, call only `project`.
Do not clear `+20F` here. Do not invoke `nflstreet_live_movement_step(dx,dy)` for
this AI input: it regenerates the input from the stick. Do not invoke the older
`nflstreet_actor_runtime_step_target` either: it calls the bounded old projector
and supplies additional commit/cap behavior for one observed branch.

The missing connection is the actual input-lifecycle/commit/state dispatcher for
each non-controlled actor before `nflstreet_actor_runtime_step`. That connection
must be reconstructed/verified before enabling all-player AI movement. Feeding
these requested fields directly to that runtime as though `+210..+22F` were
already committed would be incorrect. This patch alone does not enable the
other 13 players to move.

In `scene_frame.c`, **no AI call belongs in the draw passes**. After gameplay
updates have advanced actor `+1A8` through a verified movement/commit path,
`live_main.c` must call the existing
`nflstreet_actor_place(&d,&scene.hud.stacks,g,R13-0x7078u)` for each moved gameplay
actor (currently placement is only done for `player`), before
`nflstreet_scene_players`. Keep channel manager/step/pose and world placement
ordering consistent with the existing controlled-player path. The scene then
uses those visual transforms unchanged. There is no honest unconditional
insertion that completes non-controlled gameplay until the missing dispatcher
and input commit are supplied.
