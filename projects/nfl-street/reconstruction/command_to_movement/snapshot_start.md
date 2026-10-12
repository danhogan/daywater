# Native pair loop from a saved RAM image

The [paired capture tool](../../tools/capture_paired_actor_updates.py)
now optionally saves a 24 MiB MEM1 image while Dolphin 2603a is stopped
at Stewart's first `0x800F705C` preparation entry. In the new held-right
run, that image contains Stewart's exact initial actor input and movement
blocks. Thomas has not yet reached preparation: the game subsequently caps
his input strength from `1.0` to `0.72` and changes movement `+0x34` from
`0x3C5FD3EB` to `0x3C25CC62`. The existing native input projection and
acceleration reset reproduce Thomas's complete preparation state from
the earlier image. The reset function was checked directly against the
live Thomas block before integrating the startup mode.

The [Windows pair loop](paired_actor_loop_replay.c) now accepts
`--mem1 <image> <updates> <input-vectors>`. It reads the roster manager,
selected record handle, and both actors' initial blocks from that private
image, then advances Stewart and Thomas in observed order. Its roster
layout, names, handle, and native branch checks guard this GNNE69 saved
match. Thomas begins with the observed intercall reset before his first
captured preparation. The game image and Dolphin are unnecessary during
this Windows replay; the captured RAM image is still required.

The [verifier](verify_paired_actor_updates.py) hash-checks the image's
identity against the live trace and compares all output lines from the
Windows executable. On dumped frames `32–34`, the two internally composed
targets, six actor input blocks, and six complete 21-word movement blocks
match the original. The executable's earlier trace-supplied mode also
continues to match the right and forward three-frame reference runs.

| Private W: artifact | SHA-256 |
|---|---|
| `nflstreet-paired-first-prep-right-20261007.mem1` | `e13f56387a7cece9be1e464d4a11a590c3f23e3662f71bd06f9a218e58b8dca6` |
| `nflstreet-paired-snapshot-right-20261007.jsonl` | `bdb9db4176656565209e56b83dcd3873933eeb3e2d257cb18279b208b1ccf592` |
| `nflstreet-paired-actor-snapshot-loop-20261007.exe` | `b91532f38f054b61a38970f4d9c87ae2580542d8343461c37d098de8b9e26efc` |

**Reference:** NFL Street GameCube GNNE69 revision 0, ISO SHA-1
`813c4511ba3135059a13de424555647e8252254b`, DOL SHA-1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`, Dolphin 2603a,
restored state SHA-256
`ea3d1fa29797d1521b2ab83aee866c190ed092865b7739ceb537d15aba5e7f32`.

**Confidence:** `verified` for this exact right-input snapshot and three
subsequent frames. The RAM image was produced by the original game, so
this is a native continuation from a fixed state, not native boot or a
complete standalone NFL Street. Other game states, actor eligibility,
services, rendering, audio, and a full play remain unmodeled.
