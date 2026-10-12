# Live stick-to-movement path

The game-specific research below links controlled stick input to the
[native kinematics replay](kinematics/README.md). The observed cardinal actor
field writes now have a [bounded native replay](input_translation/README.md).
The complete input translator and update loop remain unreconstructed.

**Game:** NFL Street GameCube, GNNE69 revision 0, ISO SHA-1
`813c4511ba3135059a13de424555647e8252254b`, DOL SHA-1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`.  
**Live reference:** Dolphin 2603a, supplied second-down state `8287f395`
(savestate SHA-256
`ea3d1fa29797d1521b2ab83aee866c190ed092865b7739ceb537d15aba5e7f32`).

The [capture tool](../tools/capture_thomas_kinematics.py) with
`--capture-input` followed Thomas through these live breakpoints:

```text
normalized stick vector
  -> 0x8003DDEC (r3 = Thomas actor; r4 = input vector)
  -> actor input fields at +0x200..+0x22F
  -> 0x800F705C calls 0x80036B18 (movement command setter)
  -> 0x80036B28 (native-reconstructed kinematics branch)
  -> Thomas position at actor +0x1A8/+0x1AC
```

| Held input | Float32 input X,Y at `r4+4/+8` | Actor `+0x204` at command setter | Setter return PC |
|---|---|---|---|
| Right | `1.0, 0.0` | `0x000000` | `0x800F71E0` |
| Forward | `0.0, 1.0` | `0x3FFFFF` | `0x800F71E0` |
| Left | `-1.0, 0.0` | `0x7FFFFF` | `0x800F71E0` |

In each fresh run, `0x8003DDEC` entered with r3 equal to the independently
located Thomas actor. On return to `0x8003E85C`, actor `+0x200` held float
`1.0` and byte `+0x20E` held `1`. The heading at actor `+0x204` was already
the displayed value **at function entry** in those three settled-input
captures. Two additional fresh runs armed the nonzero-stick DOL store at
`0x8003E0A4` before changing input. With r31 equal to the Thomas actor, a
single PowerPC step to `0x8003E0A8` changed actor `+0x204` from `0x7FFFFF`
to `0x000000` under right input, and from `0x7FFFFF` to `0x3FFFFF` under
forward input. This **verifies** the direct heading writer for these two
transitions. A separate `0x8003E13C` store handles a zero-stick branch;
its live behavior was not tested.

The later `0x800F705C` call to setter `0x80036B18` passed actor `+0x204`
as r4. The setter's original DOL instructions copy r4 to kinematics block
`+0x38`, store r5 to `+0x18`, and store f1 to `+0x34`. Its return-state block
was byte-identical to the following `0x80036B28` entry block in all three
traces. Only `+0x18` changed across each observed setter call because
`+0x34` and `+0x38` already held the same values. The Windows C kinematics
replay matched all 21 output words for these three new blocks.
The two heading-transition runs also reached that setter with r4 equal to
the newly written heading, and their complete kinematics outputs matched
the Windows replay bit for bit.
The [native command-to-movement replay](command_to_movement/README.md) now
matches both the setter's full output block and the following movement output
on these five traces and three earlier command captures. It calculates r5
from the observed branch and saved turn-limit value; f1 acceleration is still
supplied from the capture.
For three newer fresh runs, the
[actor-update replay](command_to_movement/actor_update.md) also calculates
f1 from the actor and movement state before the command setter.

The raw traces remain private on W: as
`thomas-kinematics-input-{right,forward,left}.jsonl`, with SHA-256 respectively
`7f12ee6f766e42673adf7469965da041e9a49cdf61eb2aa8cafa93583fe76281`,
`114956da2326aaf4ddd9012f7d9dbba7386b58f2cc3539f7b990e15690890ea5`,
and `7d69d8fd40b320f89521d9a61283d54a375a631e992107403d962d1da4a9664a`.
The two transition traces are
`thomas-heading-transition-{right,forward}.jsonl`, SHA-256
`eb7f9da728a132d5cff77c2f9e89bbb84c6262ef55c56ec25940bc331591c104`
and `b9aeb72b514087aeb2faa191e863cde8b234d37dec38a761afbe31d34fe0658a`.
The Daywater container files were restored to their original hashes after
capture. The normal Windows gameplay DLL was not changed.

**Confidence:** `verified` that the three live stick vectors reached Thomas
at `0x8003DDEC`, that the observed actor heading was passed through the
command setter into `0x80036B28`, and that the guarded Windows C replay
reproduced each resulting block. The two direct store steps **verify**
`0x8003E0A4` as the changing heading writer for the tested right and forward
transitions. A continuous native input/update loop, other state branches,
and the renderer are still absent.
