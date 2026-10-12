# Native intercall movement setup

The original GNNE69 revision 0 actor update `0x800F86BC` calls `0x80036AF8`
to clear movement acceleration at block `+0x34`, then calls `0x800F7C90`
to prepare speed and acceleration for the next movement call. The original
DOL disassembly establishes those call and store locations. A write
watchpoint in one fresh right-input run was stepped at `0x800F8008` and
`0x800F8028`; each store wrote `0x3C25CC62` to Thomas's `+0x34` field, and
the value persisted to the next preparation entry.

The [bounded C source](reset_acceleration_replay.c) reconstructs the
normal-movement branch observed in fresh right and forward runs. It uses
actor input strength `+0x210`, signed rating `+0xBBC`, and constants checked
against the supplied MEM1 image. The 0x54-byte state block changed only at
`+0x34` across each captured `0x800F7C90` call. The native output matched
every word of both live return blocks on Linux and Windows x64. The
[sequence verifier](verify_update_sequence.py) also checks that this return
block equals the next actor-update preparation entry.

The [Windows native loop](actor_update_loop_replay.c) now has a calculated
reset mode. Between calls it applies the original `0x80036AF8` clears,
computes the observed `0x800F7C90` result, and runs the next actor update.
It matched two fresh two-update runs with right and forward input and a
fresh three-update right-input run, including every 21-word movement result.
The three-update run calculated the reset twice; only the first reset
function was separately captured at entry and return. The second reset is
validated by the following complete preparation and movement blocks.

| Live trace | Private W: SHA-256 |
|---|---|
| Stepped right-input reset stores | `dd06ae9f7646e5b3b3695c432eec1dfef410dcd63a29e565f3dc0e2b4d649784` |
| Right setup function and two updates | `8fa70d3d2f2689fba407a0bcf04f9b500c9c38581f43e41c4167f3dd6b5ce7ab` |
| Forward setup function and two updates | `738d4dc8a900e613a4784e5ef6242b64c5a6be383d5f323695dca3d6045e6787` |
| Right setup function and three updates | `269ef6e4cbbaf10c60caf5f2f6365e6c52751c110d62f331cf728599fc067a7f` |

The checked Windows executables remain on private W: storage. The reset
replay SHA-256 is
`6c227b076d11ac0d894e1c08de57c87ae7543bbefe595c1ed9ca04b8739fc8ae`,
and the current loop replay SHA-256 is
`3b7cc1274c9d83b7b86be5d3cd95bd9c8d4b5d66fff3ef8fe5a77e3b91b29834`.
Build them with `build_windows_reset_acceleration_replay.sh` and
`build_windows_actor_update_loop_replay.sh` in the parent reconstruction
directory. Temporary container tracing source was removed and the original
Daywater debugger/session hashes restored. The normal gameplay DLL was
unchanged.

**Confidence:** `verified` for the stepped stores and captured right and
forward state transitions, and for the exact outputs of the bounded native
calculation in those runs. The original setup function has other branches,
game-service calls, and clamps; the C source guards its tested input shape
and does not claim general replacement. Actor input still arrives from the
original game, so this is a native replay loop rather than a standalone game.
The later [partial-strength runs](partial_strength.md) also verify the
observed maximum-speed `+0x24` update when strength changes.
