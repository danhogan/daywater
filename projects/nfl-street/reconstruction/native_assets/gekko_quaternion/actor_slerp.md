# Actor body slerp (801EAD98 / 801CA900)

Confidence: **probable**, transcribed from GNNE69 r0 instructions, compiled
with the original-module checker; execution of this stage is pending the
main checkout's checks. The independently verified numerical helpers retain
their evidence in [STATUS.md](STATUS.md). No Windows executable or gate was
run after the switch to compile-only work.

`nflstreet_801ead98_slerp_body` accepts the existing palette context and
draw, a model address, palette descriptor, compact pose and per-bone enable
table. It implements the model/skeleton/cluster null checks, cache-enable
byte, even pool allocation, initial ordinary `801CA4E8` palette and DMA,
selected slerp rows, and final DMA (or the RAM-neutral flush).

`8028DD70` supplies enable bytes; the parent matrix is row byte 1. For each
enabled bone, compact pose and rest Z/Y/X angles become arguments r4/r5/r6
of `801A36EC`. Its quaternion results feed `801A383C` with the unsigned row
weight divided by 4096. `801A340C` returns signed 24-bit angles; arithmetic
shift by eight supplies `80188BB0`. Translation comes from the bone's
16-byte skeleton translation record. `801FFDF0` concatenates the local
matrix onto the row's parent, retaining the extra zero-lane FMA for column
2. Disabled rows retain the initial ordinary palette, even if a parent's
row is subsequently replaced.

The old draft incorrectly treated enable bytes as parent indices and
omitted the Euler shift; both are corrected in this version. That draft's
failed comparison is not evidence for the extracted implementation.

## Actor call

`actor_palettes.c` includes `gekko_quaternion/actor_slerp.h`. Its private
`body()` now takes an extra `int slerp` and starts with:

```c
if (slerp)
    return nflstreet_801ead98_slerp_body(c, d, m, desc, pose, 0x8028dd70u);
```

Remove the old early `if (flags & 0x4000) return 0;` rejection. At the
existing body call, after pushing the actor transform, use:

```c
if (!body(c, d, rd32(a + 0x4b8), actor + 0x1008, pose,
          (flags & 0x4000) != 0)) return 0;
```

Hand and held-item calls pass zero for the new argument. Link both
`gekko_quaternion/actor_slerp.c` and `gekko_quaternion/quaternion_kernels.c`.
The palette-check and player-render build scripts include them.

## Compile and external verification

```bash
bash projects/nfl-street/reconstruction/native_assets/build_windows_actor_palettes_original_check.sh /mnt/w/Daywater/research-temp/nflstreet-actor-slerp-20261014/
```

The following is for the main checkout/operator to execute, once per
context 14, 15 and 16 (replace the snapshot suffix):

```bash
/mnt/w/Daywater/research-temp/nflstreet-actor-slerp-20261014/actor-palettes-original-check.exe 'W:\Daywater\research-temp\position-neutral-14.mem1' 'W:\Daywater\research-temp\nflstreet-native-windows-local\gGNNE69_recomp.dll' slerp
```

The `slerp` argument selects 401 cases over all 14 actors. It varies the
flag per actor, full compact-pose angles, equal pose/rest angles, weights
in [0,4096] including endpoints, noncanonical nonzero enable bytes, disabled
bytes, scratch versus in-place output, and the pre-existing camera/scale/
LOD/attachment edits. Case 0 forces all actors onto the new path using the
saved pose and enable table; cases 2/3/4 exercise disabled rows and endpoint
weights. The oracle executes the original `80162760`, including its
`801EAD98`/`801CA900` calls. It compares all RAM outside its stack and all
locked-cache bytes. Without `slerp`, the historical palette and placement
suite remains selected. Mutation candidates are in
[actor_slerp_mutants.json](actor_slerp_mutants.json); none have been run.

## Limits

This implementation inherits the existing oracle boundary: enabled head
and hand dynamics return immediately; cache hardware enable/flush have no
RAM effect; DMA is served from the context's locked cache. It requires
valid processed skeleton/cluster tables with at most 256 entries, ordered
parent rows, and finite matrix inputs. The modeled slerp weights are
[0,4096]; extrapolation can leave the reconstructed MSL sine's bounded
domain. General MSL large-argument reduction and square-root denormals
remain outside the helper proof. No live Dolphin integration is claimed.
