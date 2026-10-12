# Second actor native movement replay

The guarded native actor movement path has now been checked for a second
roster actor in the supplied match. The actor-order traces identified roster
slot 7 at `0x803C04EC`; the fresh captures read the name bytes as `Stewart`.
This pointer and name apply to the specific saved state below.

**Reference:** NFL Street GameCube GNNE69 revision 0, ISO SHA-1
`813c4511ba3135059a13de424555647e8252254b`, DOL SHA-1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`, Dolphin 2603a,
saved-match state SHA-256
`ea3d1fa29797d1521b2ab83aee866c190ed092865b7739ceb537d15aba5e7f32`.

The [single-call capture tool](../../tools/capture_thomas_kinematics.py)
and [sequence tracer](../../tools/capture_thomas_update_sequence.py) now
accept an optional observed roster slot. Their default Thomas selection is
unchanged. Both checked the known match before selecting slot 7. Raw traces
and Windows executables remain private on W:.

Two fresh single-call captures held right and forward input. The existing
[stage verifier](verify_actor_update.py) checked the original preparation,
command setter, and movement boundaries and compared all 21 state words
after each native stage. Both passed using a newly built Windows x64 replay.
The two preparation input blocks differed in 18 bytes and the state blocks
differed in 15 words, so this checks distinct live inputs.

Two further fresh runs captured three consecutive slot-7 updates under held
right or forward input. Preparations in the right run appeared in dumped
frames 32, 33, and 34. Both runs captured the original reset entry and
return between *each* pair of updates. The
[sequence verifier](verify_update_sequence.py) checked both resets, each
command-entry block, and all 21 movement words after each Windows native
loop step. The actor input block changed between calls and was supplied from
each live preparation; it was not reconstructed from a controller vector for
this actor.

To repeat the bounded right-input sequence with a compatible Dolphin 2603a
binary, pass the verified ISO and savestate to
`capture_thomas_update_sequence.py` with `--actor-slot 7 --stick-x 1
--stick-y 0.5 --updates 3 --capture-reset-all --capture-scheduler`. Use a
new private output path and GDB port. Run `verify_update_sequence.py` on
the trace with `--varying-input`, the Windows loop and single-call
executables, the reset executable, and `--windows`. The forward run uses
`--stick-x 0.5 --stick-y 0`. The single-call traces use
`capture_thomas_kinematics.py --actor-slot 7 --capture-preparation` and
`verify_actor_update.py`.

| Private W: trace | Input and scope | SHA-256 |
|---|---|---|
| `nflstreet-slot7-movement-20261007.jsonl` | Right, one complete call | `24977b5d76d37770377207edd7bd39e99a1e339c52dd1ef26f8e2e273f089b66` |
| `nflstreet-slot7-movement-forward-20261007.jsonl` | Forward, one complete call | `be9f445b7332dab3ad6f4bffd6a424ed741564ba88a07d4ff14b23b765811b4f` |
| `nflstreet-slot7-three-right-20261007.jsonl` | Right, three updates and two resets | `25948dca3eccd95d34622cf2d07505354fe561ae8308c3c89c36b12aac2cf82f` |
| `nflstreet-slot7-three-forward-20261007.jsonl` | Forward, three updates and two resets | `7d36eca88b3a218ea51d3fa0a99bddcdce04bd3961bbd7982bd4cc03c0eba364` |

The prior Windows single-call executable dated 2026-10-05 rejected this
actor at the movement stage because it predates the verified speed-cap
kinematics branch. The standalone current kinematics executable matched the
captured movement call. A separate rebuild from current source at
`W:\Daywater\research-temp\nflstreet-actor-update-current-20261007.exe`
(SHA-256 `39c07cf6d0137a4318e125bfeac0cf87f1d470e8ac52edc37f6326a4ccc5387d`)
then matched all three stages in both single-call captures. The older
executable was left untouched. The three-update checks used the existing
current actor-loop executable and this rebuilt single-call executable.

**Confidence:** `verified` for slot 7's tested preparation, command,
reset, and movement outputs in the four fresh saved-state runs. The native
loop still consumes game-populated actor input blocks for slot 7. This does
not establish controller translation, eligibility, update order, or matching
movement behavior for the other roster actors. A multi-actor native scheduler
remains unimplemented.
