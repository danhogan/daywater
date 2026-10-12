# NFL Street (GameCube): Thomas actor ground position

## Finding

**Name:** Ground-plane X/Y coordinates in the roster actor named Thomas  
**Confidence:** `verified` for the tested game image and Dolphin build  
**Date:** 2026-10-01  
**Researcher/agent:** Codex

## Game identity

| Item | Evidence |
|---|---|
| Title / platform | NFL STREET / GameCube |
| Game ID / region / revision | `GNNE69` / North America (`E`) / disc 0, revision 0 |
| ISO SHA-1 | `813c4511ba3135059a13de424555647e8252254b` |
| DOL SHA-1 | `46e3de854998a0f66c9be8c72a1164d22cd29eb4` |
| Dolphin build | Official Linux 2603a |
| Test states | Supplied `19bb4a7c` first down and `8287f395` second down, Bears–Lions on EA Field |

## Memory locator

The static big-endian pointer slot at `0x803A1E10` is in DOL initialized
data section 10 (`0x803A1A40..0x803A3D3F`). The DOL initializes PPC `r13`
to `0x803A9A40`; `FUN_8003A4A0` accesses the slot as `r13 - 0x7C30` and
computes actor addresses from a roster array and `0x14D4`-byte stride.

```text
manager    = read_be_u32(0x803A1E10)       # observed 0x803B7300
actors     = read_be_u32(manager + 0x00)   # observed 0x803B7320
per_team   = read_be_u16(manager + 0x08)   # observed 7
thomas     = actors + (1 * per_team + 1) * 0x14D4
name       = read_ascii_z(thomas + 0xB80)  # "Thomas" in tested states
ground_x   = read_be_float32(thomas + 0x1A8)
ground_y   = read_be_float32(thomas + 0x1AC)
ground_z   = read_be_float32(thomas + 0x1B0)
```

The observed Thomas actor is `0x803C19C0` (Bears array slot 1), with X at
`0x803C1B68`, Y at `0x803C1B6C`, and Z at `0x803C1B70`. Each is a 4-byte
big-endian IEEE-754 float32. The X and Y fields are `verified` ground-plane
coordinates. Z was `0.0` in the recorded flat-field states, including a
frame where Thomas appears airborne, so it must **not** be used as player
height without another test. Array address, team order, index, and actor
address are observations for this match, not universal constants. The
[controller 0 selection mapping](selected_player.md) now provides a tested
team and actor index through the roster manager. The separate
[visual-object actor reference](selected_actor_candidate.md) correlated with
the marker in the supplied match but is not the chosen general locator.

## Test procedure

1. Restore `19bb4a7c` in five independent Dolphin 2603a processes. Before
   input, capture a frame and live 24 MiB MEM1 image. Press/release `A` to
   start the play, then hold the main stick left, neutral, right, forward,
   or backward. Capture MEM1 and frames once per real second for 18 seconds.
2. Visually confirm the selected runner marker names **Thomas** after the
   snap. The actor in Bears slot 1 contains the ASCII string `Thomas` at
   actor `+0xB80`; nearby bytes contain `Anthony` in the same record.
3. Read the actor array through the static slot on every capture. Compare
   the named actor's `+0x1A8/+0x1AC` values across opposite stick inputs and
   over time. The actor position is near, but distinct from, the ball
   position. The ball fields are documented in [ball_position.md](ball_position.md).
4. Restore the different second-down midplay state `8287f395` twice. Hold
   left or right immediately, and capture at 0, 2, 4, and 6 seconds. Confirm
   the same named actor and pointer chain, with X separating in opposite
   directions. No game memory was patched.

## Observations

In the first-down starting frame, Thomas is at `(0.041, -38.531, 0.000)`;
the ball is at `(0, -33, 0.097)`. They start in distinct locations. Later,
Thomas becomes the selected runner and moves with the held input.

| Input | Thomas X at 18 s | Thomas Y at 18 s | Thomas Z at 18 s | Visible motion |
|---|---:|---:|---:|---|
| Left | -10.952 | -35.285 | 0.000 | Left |
| Neutral | -1.941 | -35.094 | 0.000 | Near middle |
| Right | 5.614 | -35.636 | 0.000 | Right |
| Forward | -2.672 | -27.863 | 0.000 | Upfield |
| Backward | -2.041 | -43.634 | 0.000 | Back toward camera |

The left run's X progresses from `0.041` before input to `-3.727` at 10 s,
`-7.218` at 14 s, and `-10.952` at 18 s. Neutral settles near `-1.94`;
right reaches `+5.614`. In the second-down state, Thomas starts around
`(-12.384, -31.192)`. After six seconds, his X is `-16.563` with left input
and `-7.965` with right input; Y stays near `-31.192` in both. The actor
record still contains `Thomas`.

## Ghidra relationships

- `FUN_8003A4A0` reads the roster-manager pointer at `r13 - 0x7C30` and
  returns `actors + (team * per_team + index) * 0x14D4`. This independently
  establishes the actor stride and lookup calculation.
- `FUN_8003AB68` reads `actor + 0x1A8`, `+0x1AC`, and `+0x1B0` as floats.
- `FUN_80019A84` has a code path that adds three displacement values to
  `param_2 + 0x1A8/+0x1AC/+0x1B0`. `FUN_800CDA28` has a branch that copies
  `+0x1B4/+0x1B8/+0x1BC` into those fields. These are **static candidate
  writer paths** for this actor layout; their execution on Thomas during
  the controlled runs was not separately proved.
- `FUN_800173BC`, the only indexed caller of `FUN_80019A84`, also has direct
  stores to an object at `+0x1A8/+0x1AC` (including instructions at
  `0x8001760C` and `0x80017614`). This is another **static candidate**. The
  object passed through that path has not been tied to Thomas at runtime.
- A live Dolphin GDB write watchpoint on Thomas X reported 15 hits during
  three seconds of left movement. A control watchpoint on a constant field
  in the same actor memory page reported zero. The stop packets gave PCs
  from several functions, including instructions that are not direct
  stores. This watchpoint result alone did not identify a writer.

## Executed X writer in the first-down play

With Dolphin debugger mode enabled, a fresh replay of supplied state
`19bb4a7c` snapped the ball with `A` and held the main stick left. An
execution breakpoint at `0x80068730` stopped before the DOL instruction
`stfs f0, 0x1A8(r27)`. When the breakpoint targeted Thomas,
`r27=0x803C19C0`, the independently verified roster actor address. A
single PowerPC instruction step advanced to `0x80068734` and changed
Thomas X from `-0.9198098779` to `-0.8880179524`, while Y stayed
`-36.3248596191`. A separate fresh first-down replay reproduced the same
target and single-step values. This **verifies** `0x80068730` as a direct
writer of Thomas X in this tested play; it does not establish that it is
the only writer or that its per-frame change always follows the stick
direction.

The [reusable trace tool](../../projects/nfl-street/tools/trace_thomas_position_writer.py)
verifies the ISO hash, identifies Thomas through the roster manager, waits
until after the snap, skips other actors with single-instruction steps, and
records the exact Thomas store. Its validated first-down JSONL is on W: as
`thomas-x-writer-first-down.jsonl` (SHA-256
`72f466998bd88d72c76daeb9e011ea5a7808a1c200a223717a10c54af2deb296`).

The same first-down function has a separate `stfs f13, 0x4(r26)` at
`0x80068748`. A diagnostic Y watchpoint suggested this PC but disturbed
Thomas's position, so it was tested again with a clean execution breakpoint.
In two fresh `19bb4a7c` replays, the breakpoint hit first for another
object, then with `r26+4=0x803C1B6C`, the independently located Thomas Y
address. Single-stepping to `0x8006874C` changed Thomas Y from
`-36.7148323059` to `-36.6327514648` in the forward run, and from
`-36.8652496338` to `-36.7751770020` in the backward run; X stayed fixed
across each Y store. This **verifies** `0x80068748` as a direct Thomas Y
writer in the tested first-down path. Both individual Y steps increased
the value despite opposite held input, so these steps alone do not show
per-instruction directional control. The forward and backward JSONL traces
are on W: as `thomas-y-first-down-forward.jsonl` (SHA-256
`8a88798001203734623e04af4fba832d762f902c470bd579355bdb6220629375`)
and `thomas-y-first-down-backward.jsonl` (SHA-256
`bfa872718f6ffef574908f4953854c88778aea1ea1d86933077953df4236b70b`).

An independent replay of the supplied second-down midplay state
`8287f395` held right input. Thomas X moved from about `-11.37` to
`-5.98` during the trace, but 34 hits at `0x80068730` all targeted another
actor (`r27=0x803BC670`). This is a **verified limit** of the first-down
writer finding. The control is
`thomas-x-writer-second-down-control.jsonl` (SHA-256
`c02eafabaef5875cb9c31b58d8fa1449849df770f9e892269d40eaad0139a240`).
Four earlier static candidate stores at `0x8001760C`, `0x80017614`,
`0x80019AEC`, and `0x80019AF0` did not execute in a 12-second first-down
left-movement window while Thomas X changed from about `-0.86` to `-8.72`.
The control is `thomas-static-writer-control.log` (SHA-256
`6351f0bb0a4a61f6df2a87736b8d66365151f47c3186a86807b564b2bfc29d77`).
A Thomas-X write watchpoint perturbed the field to zero in a separate run;
that diagnostic is `thomas-watchpoint-perturbation.log` (SHA-256
`387ec8cea77d3bad08813f331dbfb1bca0d4734398231a4120bb8231c6a7c1ca`).

## Executed X writer in the second-down play

In supplied midplay state `8287f395`, the earlier writer at `0x80068730`
did not target Thomas. A diagnostic write watchpoint suggested another path,
but perturbed the position field, so the writer was confirmed with execution
breakpoints. The DOL instruction at `0x80036E08` calls `0x801DE584`; its
caller register `r30` held Thomas X address `0x803C1B68` on the seventh hit
after six calls for other actor fields. At `0x801DE590`, the callee's
`stfs f13, 0(r3)` had `r3=0x803C1B68`. A single instruction step advanced
to `0x801DE594` and changed only Thomas X in the three-float position read.

Two fresh Dolphin 2603a replays using the
[second-down trace tool](../../projects/nfl-street/tools/trace_thomas_position_add.py)
reproduced this store from opposite main-stick inputs:

| Held input | X before store | X after store | Y before and after |
|---|---:|---:|---:|
| Right | -11.2555341721 | -11.1404190063 | -31.1916427612 |
| Left | -13.6239118576 | -13.7403068542 | -31.1916427612 |

This **verifies** `0x801DE590`, reached from `0x80036E08`, as a direct
Thomas X writer in the tested second-down play. The opposite signed changes
agree with the controlled input and earlier position captures. The result
does not establish that this is the sole writer or that this caller is used
in every play. The right and left JSONL traces are on W: as
`thomas-x-second-down-add.jsonl` (SHA-256
`1bbe33802c4d2759e1feda1f92116584e20e26006f65fe8a7bd5e590b77df988`)
and `thomas-x-second-down-add-left.jsonl` (SHA-256
`81bf4237e6f64475742f61026151b5f463362b783f11913cdbc7f97206ba3d2b`).
The exploratory caller log is `thomas-x-second-down-caller-prototype.log`
(SHA-256 `9535ba3267768704fc565be94dec36aa1bb036933af93ed9db6e85509adf08d8`).

The indexed DOL disassembly shows that `0x801DE584` adds three adjacent
float32 values: its stores are X at `0x801DE590`, Y at `0x801DE5A0`, and Z
at `0x801DE5B0`. The trace tool's `--trace-vector` option stepped through
all three stores in two more fresh replays of `8287f395`, with main-stick X
neutral and opposite Y input:

| Held input | Y immediately before `0x801DE5A0` | Y immediately after | Z after `0x801DE5B0` |
|---|---:|---:|---:|
| Forward | -30.0204238892 | -29.9050483704 | 0.0 |
| Backward | -32.3541641235 | -32.4692840576 | 0.0 |

The live single steps **verify** `0x801DE5A0` as a direct writer of Thomas
ground Y in this second-down path. `0x801DE5B0` executed with the expected
Thomas Z address, but Z remained `0.0` in these flat-field samples. That
observation does not establish a height interpretation. The forward and
backward JSONL traces on W: are
`thomas-position-second-down-forward.jsonl` (SHA-256
`e6e324d75210ab93f698463ed8efea5a727b206bf0c4c15f94124fe60826f712`)
and `thomas-position-second-down-backward.jsonl` (SHA-256
`3e9739656b551bd41369fb07a28686359caa85874f36dee80fc67914cb2db25a`).

An interpreter-CPU probe was also attempted, but Dolphin did not expose
MEM1 within the 30-second setup window. It yielded no writer observation.

No Ghidra functions have been renamed. The static names above are their
original auto-generated names, not inferred function meanings.

The DOL Ghidra index was regenerated on 2026-10-01 with function body ranges
and disassembly. The tested X/Y fields, source evidence, and separately labeled
static candidates are recorded in the executable-scoped campaign graph at
`sessions/58b4dd7af31f/campaigns/46e3de854998a0f66c9be8c72a1164d22cd29eb4/knowledge/graph.json`.
The runtime-to-static CLI was exercised for Thomas X without a live session;
it correctly reported that no writer PC could be resolved from that invocation.

## Retained evidence and limits

Raw 24 MiB MEM1 images and corresponding frames are outside the repository
under `W:\Daywater\research-temp\`. The first-down series are
`position-longleft-00..18`, `position-longneutral-00..18`,
`position-longright-00..18`, `position-longforward-00..18`, and
`position-longback-00..18`. The second-down images are
`position-alt-second_left-*` and `position-alt-second_right-*`. Each index
has `.mem1` and `.png`. The original supplied states are
`original-<savestate-id>.mem1`. No ISO, proprietary asset, savestate, RAM
image, or authentication state was added to the repository.

The X/Y interpretation is `verified` for Thomas in these two plays: the
actor is identified by its in-memory name, its fields change smoothly under
controlled input, opposite directions separate on the corresponding axis,
and the visual motion agrees. The actor's Z field is only a candidate for
ground-plane elevation; physical jump height is elsewhere. Team/index
selection, player switching, other stadiums, revisions, and allocation
histories remain to be tested.
