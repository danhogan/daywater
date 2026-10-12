# Thomas movement: native reconstruction boundary

This boundary records the executed movement call and its live fixture. A
[guarded native C replay](kinematics/README.md) reproduces the tested branch
on Windows, and the [bounded actor-update replay](command_to_movement/actor_update.md)
now calls it after native preparation and command stages for three live calls.
The target remains a decompiled NFL Street running on Windows without
Dolphin or the game image at runtime.

## Executed path and scope

In the tested GNNE69 revision 0 DOL (SHA-1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`),
`FUN_800f86bc` passes the actor's `+0x1A8` block to `FUN_80036b28` at
`0x800F8740` when the branch at actor `+0x221` is zero. That function
updates the first three floats of the block and other nearby fields before
calling `FUN_801de584` at `0x80036E08`. The latter adds three floats and
stores the resulting position. The Thomas call was observed live in Dolphin
2603a with r3 equal to the independently verified Thomas position address
`0x803C1B68`. The same actor update also calls game-state dispatch and
preparation functions before this branch; those remain outside the captured
boundary.

The direct call targets of `FUN_80036b28`, as read from the original DOL
disassembly, are `0x80188D98`, `0x801DDFCC`, `0x801DE170`, `0x801DE460`,
`0x801DE560`, and `0x801DE584`. Their transitive direct calls add
`0x80188B6C`, `0x80188A70`, `0x80188AEC`, `0x8018889C`, and `0x802165C8`.
This is a bounded 12-function static call graph including the root. It also
reads DOL constants and at least one initialized data table. Static direct
calls alone cannot rule out every hidden runtime dependency; a native
implementation must account for the actual data and branch behavior.

## Live input/output fixture

The [capture tool](../tools/capture_thomas_kinematics.py) breaks at the
function entry `0x80036B28`, filters by the independently located Thomas
actor, reads the `0x54`-byte block, then breaks at its `blr` at `0x80036E58`
and reads the same block. It does not patch memory. Four fresh Dolphin 2603a
replays used the same supplied second-down state `8287f395` (savestate
SHA-256 `ea3d1fa29797d1521b2ab83aee866c190ed092865b7739ceb537d15aba5e7f32`).
The input begins before breakpoint arming, so the four entry states differ;
the fixture tests each exact input/output pair, not identical starting RAM.

| Held stick | Entry X, Y | Exit X, Y | Private W: JSONL SHA-256 |
|---|---|---|---|
| Right | -11.52526951, -31.19164276 | -11.41484737, -31.19164276 | `71d64b7d8f040cd7ae3282cf97fe29a4a45003a54e423dd49d87a7ea3a52465c` |
| Left | -13.21200085, -31.19164276 | -13.32342148, -31.19164276 | `1db5102ee70ec35216ef5789835e7e117eca5a2426ab48290eaafb26c99ea079` |
| Forward | -12.50345898, -30.26292038 | -12.50929451, -30.15154839 | `1a758d0df84718dd62bd27fba1cc62dcb164270c891e647b6d6aa3e0520a774f` |
| Backward | -12.51366615, -31.78201485 | -12.52751541, -31.88217545 | `1a589689f01f7d3c036cbc63146e15db7c30a307edb27cb6b2eee590eceb9e2f` |

The files are `W:\Daywater\research-temp\thomas-kinematics-{right,left,forward,backward}.jsonl`.
They contain actual game RAM and stay outside Git. Each W: SHA-256 matched
the original container capture before its temporary copy was removed. The
Daywater container's debugger, session, and score tracer were restored to
their original hashes after the runs.

**Confidence:** `verified` that the observed call changes Thomas's position
and adjacent block fields in these four fresh replays, and that the guarded
Windows C replay matches their full blocks bit for bit. Two later live calls
also match; see the native replay README. These records do not prove that
this is the only movement path, that it handles other players, or that it
suffices for a standalone game loop.

## Next reconstruction boundary

The movement branch is now called from a bounded native actor-update replay
with acceleration and command preparation. The next integration step is a
controlled handoff from semantic input through consecutive actor updates;
state dispatch, rendering, audio, and assets are still separate dependencies.
