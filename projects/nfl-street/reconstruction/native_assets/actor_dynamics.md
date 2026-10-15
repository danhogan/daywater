# Actor head and hand dynamics

## Evidence status

The owner reports the previous hand check in commit `01c21d5` passed in
contexts 14, 15 and 16: `2828 push/load/update/pop runs` per context. That
check excluded inactive matrix slots as well as the oracle CPU stack.
The math leaves have their separate original-module evidence in
`actor_dynamics_math.md`.

This revision implements `8004C268` and strengthens the hand coverage. It
has been **compiled only**. The new head implementation, multi-update
comparisons and twelve mutation tests remain **probable / pending owner
execution**, not verified. No Windows executable or gate was run by the
agent for this revision. A successful `verification.json` from the gate
below is required before promoting this scope to verified.

## Implementation

`actor_dynamics.c/.h` uses the draw span and `nflstreet_matrix_stacks` from
`venue_stacks()`, as in `actor_palettes.c`. Both routines take the visual
actor, its state block and the third argument used by `80162760`:

```c
nflstreet_actor_dynamics_context dc = {c->x, c->stacks};
int enabled = ((flags ^ 8u) >> 3) & 1u;
nflstreet_actor_hand_dynamics(&dc, actor, actor + 0x11c, enabled);
nflstreet_actor_hand_dynamics(&dc, actor, actor + 0x198, enabled);
nflstreet_actor_head_dynamics(&dc, actor, actor + 0x214, enabled);
```

Each call requires the existing selected stack with its top loaded from
`actor+3AC`. Both enabled routines push that top, concatenate the bone
matrix selected by the state's `+01` byte, and pop internally. Disabled
calls only clear the state's `+02` byte. Return 1 means success; return 0
means a required span/stack operation failed. Failure is not transactional.
The implementation assumes valid game-owned pointers and executable data,
as the existing palette reconstruction does.

Hand turns are snapshotted before the new spring force is added to Z
velocity. Negative rest Z chooses the reversed initial axis, while rest
Z <= 0 selects constants `80246D0C` and limits `8026DA34`. Positive rest Z
selects `80246D08` and `8026DA1C`. The globals are at `803A1EF0/1EF4`
(`r13-7B50/-7B4C`), not `803A1AF0/1AF4`.

The head computes two transformed axes and two look forces before the
nine first-frame joint initializations. Its current look inputs at
`state+40/+48` overlap the stored joint-angle array; they must be read
before resetting that array. It then snapshots each outgoing turn before
fused velocity damping, velocity limits and per-joint angle limits. The
head spring, look and damping globals are `803A1EF8/1EFC/1F00`.

`fmuls` sequences are retained as separate single-precision operations,
`fmadds` use `nflstreet_fmadds`, and all float-to-int conversions use the
existing saturating `nflstreet_fctiwz`. Word sums use unsigned arithmetic
to preserve PPC wrap without signed-overflow undefined behavior. Local
head angles are held by a span overlay backed by twelve host bytes; its
token `FFFFFFE0` is a host implementation choice, never a game RAM address.
No shared matrix helper is modified. The square-root dependency
`802165C8` uses an integer square-root algorithm in the disassembly; this
path contains no `frsqrte`. The verified leaves use the matching `sqrtf`
result for the tested finite inputs.

## State evidence record

Common metadata: NFL Street, GameCube GNNE69 rev 0, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Dolphin capture build is not
available in this task; these are original recompiled-module comparisons,
not a new emulator integration claim. All multi-byte RAM values are
big-endian. These interpretations follow the listed instruction streams
and remain probable until the expanded check passes.

| State-relative field | Type / bytes | Readers and writers / interpretation |
| --- | --- | --- |
| `+01` | u8 / 1 | Both routines: bone matrix index, actor `+60` plus index * `40` |
| `+02` | u8 / 1 | Both: 0 initializes on enabled call; disabled writes 0; initialized writes 1 |
| `+04..+0C` | float32[3] / 12 | Both: previous transformed position |
| `+10..+18` | float32[3] / 12 | Both: previous position difference |
| `+1C..+24` | int32[3] / 12 | Both: matrix Euler words from `8018A14C(0)` / `80189F54` |
| hand `+28..+30`, `+34..+3C` | int32[3] each / 12 each | `8004BF1C`: stored turns and velocities |
| hand `+70`, `+78` | pointer / 4 each | Pose output at pointer `+6/+8/+A`; rest angles at pointer `+416/+418/+41A` |
| head `+28..+90` | int32[27] / 108 | `8004C268`: nine triples of stored turns |
| head `+94..+FC` | int32[27] / 108 | `8004C268`: nine triples of velocities |
| head `+130`, `+138` | pointer / 4 each | Pose output `+C+12*j+2*axis`; rest input `+41C+12*j+2*axis` |

Procedure and observed values to be recorded by the owner: run the gate
on the three supplied saved states, with the edited values below; retain
its context digests, all-RAM comparison counts and mutation logs in the
private report. Input byte values include 0, 1 and 255; hand rest Z inputs
include -1200, 0 and 1200. No claim about gameplay meaning beyond these
instruction-level data flows is made.

## Original-module checks

All builds, logs, mutated source copies and reports stay in
`W:\Daywater\research-temp\nflstreet-actor-dynamics-20261014`.

The three executables share `actor_dynamics_check.h`: hand-only,
head-only, and combined. No original routine is hooked or skipped.
Original push (`8018941C`), load (`8018962C`), update and pop (`8018949C`)
execute directly in the preserved DLL, including the update's callees.
Both sides start from identical prepared RAM with the viewport stack
selected. Every byte outside `[80F38000,80F47000)` (the oracle CPU stack)
is compared after **each routine**, before the next actor/part can erase
a difference. Inactive matrix slots are included.

Defaults per context: 14 actors * 301 cases * 2 enabled flags * 4 consecutive
updates. Case 0 preserves the captured world, bone and state data before
the first update. Edited cases change world and camera matrices, selected
bone matrices, first-frame bytes, stored vectors and angles, velocities,
rest-angle sources and pose outputs. Limits are read from original tables;
their input values are set below/inside/above the ranges. Quiet cases keep
forces below saturation so damping errors remain visible. Later steps move
each world translation; selected sequences disable then re-enable dynamics.

The combined run should print 67,424 hand updates, 33,712 head updates and
101,136 all-RAM comparisons per context. Digests are intentionally not
invented or pinned before owner execution. The gate requires exact counts,
identical logs in two fresh processes per context, unchanged private inputs
and original baseline hashes, and unchanged sources.

`actor_dynamics_mutants.json` has twelve independent `{name,file,old,new}`
edits. They cover hand and head first frames, both hand scale and limit
branches, disabled reset, the ninth head joint, damping, look-force sign,
pose offset and velocity limits. The compile-only script validates each
old fragment occurs exactly once and creates an isolated executable for
each edit. The gate accepts a kill only for exit 6 with a real `RAM differs`
diagnostic; compilation errors, crashes or native rejections do not count.
None of the twelve is claimed caught until the owner runs them.

From the worktree root, compile (no Windows execution):

```bash
out=/mnt/w/Daywater/research-temp/nflstreet-actor-dynamics-20261014
bash projects/nfl-street/reconstruction/native_assets/build_windows_actor_dynamics_original_check.sh "$out"
python3 projects/nfl-street/reconstruction/native_assets/build_actor_dynamics_mutants.py
```

The owner can run the full gate, reusing those compiled binaries:

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 \
  projects/nfl-street/reconstruction/native_assets/verify_actor_dynamics.py --no-build
```

Omit `--no-build` to rebuild the baseline and mutants. For diagnostic runs,
each executable accepts `RAM DLL [cases [updates]]`, e.g.:

```bash
"$out/actor-dynamics-head-original-check.exe" \
  'W:\Daywater\research-temp\position-neutral-14.mem1' \
  'W:\Daywater\research-temp\nflstreet-native-windows-local\gGNNE69_recomp.dll' 17 4
```

## Exact palette integration (owner action after verification)

In `actor_palettes.c`, include `actor_dynamics.h`, then construct
`nflstreet_actor_dynamics_context dc={c->x,c->stacks};` inside
`nflstreet_actor_palettes` after the existing `dynamic` calculation.

Replace `(!dynamic&&!dynamics_off(c,actor+0x214))` inside the head's existing
push / load(`actor+3AC`) / pop expression with:

```c
!nflstreet_actor_head_dynamics(&dc,actor,actor+0x214,dynamic)
```

Replace `(!dynamic&&!dynamics_off(c,state))` inside the LOD-0 hand's
equivalent push / load / pop expression with:

```c
!nflstreet_actor_hand_dynamics(&dc,actor,state,dynamic)
```

Here `state=actor+0x11C+0x7C*i` for i=0,1. Keep `dynamics_off` in the
nonzero-LOD branch (or call the hand routine there with enabled=0); the
original disables both hand states at nonzero LOD. Keep the existing head
model and LOD gates and all later angle copies. Link `actor_dynamics.c` and
`actor_dynamics_math.c` alongside the existing `matrix_stack.c` and
`draw_engine.c`, compiling with `-ffp-contract=off` and linking math as in
the supplied build script. No other context fields are needed.

## Boundaries

The head and strengthened hand check await execution; no new gate or
mutation pass is claimed. This work does not alter palette generation,
animation, model construction, runtime policy or shared files. It models
RAM/matrix effects on valid saved-game data, not CPU registers, FPSCR,
exceptions or arbitrary malformed/nonfinite input behavior. NaN payloads,
subnormal edge cases and invalid pointers are outside the new randomized
coverage. Palette integration remains an explicit owner change, as the
shared files are protected.
