# Partial-strength native movement branch

The [native actor runtime](actor_runtime.h) now accepts nonzero partial
cardinal input vectors on the observed GNNE69 revision 0 branch. The
translator writes the vector's magnitude to actor `+0x200`. Before movement
preparation, the original game caps both `+0x200` and `+0x210` at the
`0x3F3851EC` (0.72f) constant stored in the hash-checked DOL at
`0x8025458C`. Live watchpoints stopped at the original strength stores
`0x800F8254` and `0x800F8618`. The native bridge reproduces this guarded
handoff and then runs the already reconstructed reset, preparation, command,
and movement stages.

A full-to-half transition exposed the original movement speed cap. When
the new maximum falls below the moving actor's speed, GNNE69
`0x80036C64..0x80036CD0` divides maximum by vector length, scales the XY
vector through `0x801DE170`, and uses the maximum as the new speed. The
[kinematics reconstruction](../kinematics/kinematics_replay.c) now follows
that branch. The other observed branch retains the fused length calculation
documented in the [input-vector bridge](input_vector_bridge.md).

**Reference:** NFL Street GameCube GNNE69 revision 0, ISO SHA-1
`813c4511ba3135059a13de424555647e8252254b`, DOL SHA-1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`, Dolphin 2603a,
saved-match state SHA-256
`ea3d1fa29797d1521b2ab83aee866c190ed092865b7739ceb537d15aba5e7f32`.
All raw traces and Windows builds remain private on W:.

The actual Windows x64 translator matched the full 0x30-byte original
translator return block on both later calls in each trace. The native actor
runtime matched all 0x30 actor-input bytes and all 21 movement words on all
three updates in each run. The standalone Windows kinematics replay also
matched each original movement entry/exit. The verifier checks the original
PC boundaries, the captured reset call, each command entry, each actor
block, and each movement exit. At a strength transition, the live intercall
state changes at both maximum `+0x24` and acceleration `+0x34`; the native
reset computed both.

| Private W: trace | Scenario | SHA-256 |
|---|---|---|
| `thomas-partial-right-input-function-three.jsonl` | settled partial right | `f896430169a487fbddd481f6c4cc64ba2c3341c94530c56505b4280f9341a7e2` |
| `thomas-full-to-half-right-three.jsonl` | full right to partial right | `a23687ad8d3be3f878ec00ad99f6f88dab86d06c45cdb7edf53241352d95e793` |
| `thomas-half-to-full-right-three.jsonl` | partial right to full right | `256fec0154c4e7d2b1f2cc08d6d5c39c057916d71ed8969eaff65684fe517736` |
| `thomas-full-right-to-half-forward-three.jsonl` | full right to partial forward | `1463a5a5c7273b20a0de0f3c7dc0108d314d68f20e483b79b8568f7522780bf0` |

Rebuild the three private Windows executables with
`build_windows_input_translation_replay.sh`,
`build_windows_kinematics_replay.sh`, and
`build_windows_actor_update_loop_replay.sh` in the parent reconstruction
directory. Their current SHA-256 values are, respectively,
`043b3b69cda5835a540e564b075e2f47753bef5948240038e1aef50f68ec1d7b`,
`4643d2265e32952b58597851b9f376eec2e3d9245da7b92ffcaf5eee6f1715b4`,
and `3b7cc1274c9d83b7b86be5d3cd95bd9c8d4b5d66fff3ef8fe5a77e3b91b29834`.

**Confidence:** `verified` for these four saved-match sequences and the
observed DOL/live write boundaries. The bridge still starts from a saved
actor state, accepts only nonzero cardinal normalized vectors, and covers
the measured strength and moving branches. Neutral input, diagonal input,
other actor states, and the full scheduler remain open.
