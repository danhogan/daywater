# Actor preparation eligibility changes during a match

The earlier [actor-order capture](actor_update_order.md) found nine actor
pointers at preparation entry `0x800F705C` on complete dumped frames `17`
and `18` under held right input. A later capture from the same saved state
recorded eight slots on complete frame `33` under held right input:
`2, 3, 5, 6, 7, 8, 10, 13`. Frames `34` through `38` then recorded seven:
`2, 3, 5, 7, 8, 10, 13`. A fresh forward-input run recorded slots
`0, 2, 5, 7, 8, 10, 13` on complete frames `33` through `37`. Calls were in ascending
slot order within each listed frame; Thomas was slot `8` throughout.

These are calls to one preparation function, not a complete list of active
actors or all work done per actor. The difference between early and later
right-input frames and the change within the later run show that a fixed
"update every roster slot" loop would not reproduce this boundary. Whether
the game scans all slots in ascending order with conditional entry, or uses a
different selection mechanism, remains unknown. The right and forward runs
also followed different game trajectories; their slot difference alone does
not prove a direct controller-input condition.

**Reference:** NFL Street GameCube GNNE69 revision 0, ISO SHA-1
`813c4511ba3135059a13de424555647e8252254b`, Dolphin 2603a,
saved-match state SHA-256
`ea3d1fa29797d1521b2ab83aee866c190ed092865b7739ceb537d15aba5e7f32`.
Private W: traces:

| Held input | Trace | SHA-256 |
|---|---|---|
| Right | `nflstreet-actor-preparation-order.jsonl` | `2a1c2929c33493e66b99b1beaa1c6dcd72ab978e65f01f4ebf3a4dc5dd49fb9d` |
| Forward | `nflstreet-actor-preparation-order-forward.jsonl` | `efee5e3c5276cab35546d59ccdac9f66b6a91dcbc536f64e11d50f28a5623d30` |

**Confidence:** `verified` for the recorded slots and order on the named
frames; `candidate` for any general eligibility or roster-scan rule. The
first and last frame of each capture may be incomplete; the comparisons
above use interior frames only.
