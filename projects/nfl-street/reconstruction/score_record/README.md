# Score record handler at `0x80025ECC`

**Status:** semantic C candidate; binary match untested. Target: NFL STREET
GameCube `GNNE69`, North America revision 0, DOL SHA-1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. The function is 204 bytes
at DOL file offset `0x22ECC`. `verify_score_targets.py` checks its target
bytes. One cached direct caller, `0x800263EC`, passes it a record pointer and
a byte from a separate local variable; the caller's broader game behavior is
not yet established.

Observed record reads and operations:

| Offset | Read | Observed use |
|---:|---|---|
| `+0x00` | 32-bit word | Passed to `0x80102F0C` on one conditional path; if zero, a code is formed from the low byte returned by `0x80103CA0`. |
| `+0x18` | 32-bit flags | For original amount `+2`, bit `0x4` and then bit `0x80000` select replacement amounts `4`, `1`, or `2`. |
| `+0x1C` | unsigned byte | Passed as the side argument; its low bit is flipped if the amount is negative. |
| `+0x1D` | signed byte | Supplies the amount before the special `+2` handling and negative-side flip. Original `-2` also gates the follow-up call when the second function argument is zero. |

The call to `0x8002676C` then enters the [score delta path](../score_delta/README.md).
For `-128`, the original `neg` followed by `extsb` still passes `-128`; the
C candidate handles that byte wrap explicitly. Names in `score_record.h` and
`score_record.c` are placeholders, and the roles of `0x80103CA0` and
`0x80102F0C` remain unknown. **Verified:** the DOL instructions, offsets,
branch conditions, and called addresses. **Candidate:** the C source and
record semantics beyond those operations. In two successful PAT traces, the
score-delta entry had LR `0x80025F30`, consistent with this handler's call at
`0x80025F2C`. This verifies the tested caller relationship; the record
contents at handler entry have not been captured live.

Run `build_windows_score_path.sh` from WSL to build a Windows x64 console
test executable on W:. The command accepts a different output path as its
first argument and `DAYWATER_ZIG` to select another Zig binary. Its test
services are fakes; it is not a playable game.

A separate [native event replay](../../native/score_event_replay.md) tested
this handler candidate against two records captured at its live entry in
fresh Dolphin PAT runs. One record led to no score and the other to a 6→7
update in both the game and the Windows C path. These two calls verify their
tested outputs, not all possible record values or a binary match.
