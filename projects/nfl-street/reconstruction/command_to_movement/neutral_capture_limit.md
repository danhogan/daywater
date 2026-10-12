# Neutral-input capture limit

The saved-match actor-update sequence used for the cardinal movement bridge
does not currently provide a valid neutral-input comparison. In two fresh
Dolphin 2603a runs, the stick switched from full right to neutral after the
first Thomas movement exit. No second Thomas preparation event was captured.
A write watchpoint on the original Thomas position address stopped at
`0x801DE590`, but Thomas's 16-byte name field, 0x30-byte input field, and
0x54-byte movement block were all zero at the stop. The actor had been
cleared, so those later writes cannot be interpreted as Thomas movement.
The observed position-store PC alone does not identify why the actor was
cleared.

**Reference:** NFL Street GameCube GNNE69 revision 0, ISO SHA-1
`813c4511ba3135059a13de424555647e8252254b`, Dolphin 2603a,
saved-match state SHA-256
`ea3d1fa29797d1521b2ab83aee866c190ed092865b7739ceb537d15aba5e7f32`.
Private W: traces: `thomas-full-right-to-neutral-position-watch.jsonl`
(SHA-256 `8deea106fada24d7f9cb840f248996977bada79f1a48e74a1af6ce2af18eac13`)
and `thomas-full-right-to-neutral-lifetime.jsonl`
(SHA-256 `25cc0142c2f144049fdedaee4b80284a4882d0b28e3040232237b74e7ca2e58d`).
The trace script now records actor name bytes with position watchpoint hits.

**Confidence:** `verified` that this saved-state test loses the Thomas actor
before a second comparable update; `candidate` as to the reason. Neutral
input remains unimplemented in the native bridge. A future neutral test
needs a state where the same actor survives at least two updates, plus a
fresh original movement entry/exit trace.


## Later original-module investigation

The [native movement viewer investigation](../native_assets/native_movement_player.md)
found three other existing MEM1 snapshots with the Thomas identity intact.
Original neutral translation calls return with both branch predicates zero
and preserve the complete actor input block. This is a controlled call result,
not a newly captured consecutive stopping sequence. It does not resolve the
actor-clearing case above or establish native braking behavior.


## Bounded braking now reconstructed

The [neutral/braking milestone](../native_assets/native_braking.md) executes the
original zero-status actor caller on those naturally braking snapshots. All
192 complete actor/movement checkpoints match, including a controlled moving
release. The native viewer now brakes instead of pausing on neutral. This
establishes the bounded original status dispatch; other controller contexts
and the separate actor-clearing live-capture case remain unverified.
