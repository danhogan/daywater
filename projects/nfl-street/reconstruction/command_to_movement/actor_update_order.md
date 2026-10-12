# Observed multi-actor preparation order

**Reference:** NFL Street GameCube GNNE69 revision 0, ISO SHA-1
`813c4511ba3135059a13de424555647e8252254b`, DOL SHA-1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`, Dolphin **2603a**,
supplied match savestate SHA-256
`ea3d1fa29797d1521b2ab83aee866c190ed092865b7739ceb537d15aba5e7f32`.

The [actor-order probe](../../tools/capture_actor_update_order.py) stopped at
the observed movement-preparation entry `0x800F705C`, read the actor pointer
in `r3`, and recorded the latest dumped frame. It checked the known Thomas
actor before arming the breakpoint. The
[offline verifier](../../tools/verify_actor_update_order.py) requires the
expected PC, return address `0x800F721C`, stable name bytes, unique actor
pointers within each complete frame, and identical order in each trace and
across two fresh processes. Both traces used full-right held input.

The first recorded frame, number 16, began after the breakpoint was armed and
is partial. Frames 17 and 18 contain nine preparation calls each. The order
was identical in both frames and in a second fresh run:

| Order | Actor pointer | Observed name bytes through first NUL |
|---:|---:|---|
| 1 | `0x803B7320` | Rogers |
| 2 | `0x803B9CC8` | Harrington |
| 3 | `0x803BB19C` | Stewart |
| 4 | `0x803BDB44` | Bly |
| 5 | `0x803BF018` | Holmes |
| 6 | `0x803C04EC` | Stewart |
| 7 | `0x803C19C0` | Thomas |
| 8 | `0x803C4368` | Brown |
| 9 | `0x803C81E4` | Traylor |

Private JSONL traces are on W: in `Daywater/research-temp` as
`nflstreet-actor-order-20261007-c.jsonl` and
`nflstreet-actor-order-20261007-d.jsonl`. Each has SHA-256
`66bacbbdb56fd8ac6ee424471965ffac31bed25caf15ed52c66e13133a6cef5b`;
the byte-identical results reflect deterministic replay from the same state.
The verifier reported two complete frames per trace, nine unique actors per
frame, and equal order. The normal container session and debugger files were
restored after each run. Earlier failed setup attempts used the container's
default Dolphin **2609**, which did not restore this **2603a** savestate;
those attempts contain no actor-order evidence.

**Confidence:** `verified` for the order of these nine calls within frames
17 and 18 in the two fresh saved-state runs. This does not establish that
these are all active actors, a general update order in other game states, or
that every actor follows Thomas's guarded native movement branch. The next
step is to capture a second actor's preparation, command, and movement
blocks across consecutive frames and check them with the native replay
before scheduling multiple actors in native code. Later
[captures](actor_order_dynamic.md) show that the roster slots reaching this
boundary change across frames and runs.
