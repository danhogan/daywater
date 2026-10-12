# Strength handoff investigation

**Target:** NFL Street GameCube GNNE69 revision 0, DOL SHA-1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`.

The [native input-vector bridge](input_vector_bridge.md) now uses the
observed 0.72f cap for nonzero cardinal input on the saved-match branch.
Four completed [partial-strength captures](partial_strength.md) supersede
the earlier incomplete attempts below. This is not a general model of the
original strength calculation.

## Checked original instructions

The hash-checked DOL contains these PowerPC instructions in the function
beginning at `0x800F8214`:

| PC | Instruction | Direct observation |
|---|---|---|
| `0x800F822C` | `lfs f13, 0x456C(r9)` after `lis r9, 0x8025` | Loads a float whose DOL word is `0x00000000`. |
| `0x800F8230` | `addi r30, r29, 0x200` | Forms a pointer to actor `+0x200`. |
| `0x800F8238` | `addi r31, r29, 0x210` | Forms a pointer to actor `+0x210`. |
| `0x800F8254` | `stfs f0, 0x200(r29)` | Writes actor `+0x200` after a compare and possible selection of the loaded zero. |
| `0x800F8604..0x800F8610` | `fcmpu`, `cror`, conditional branch, `fmr` | Selects between `f0` and `f13` before the final pair of stores. |
| `0x800F8614` | `stfs f0, 0(r31)` | Writes actor `+0x210`. |
| `0x800F8618` | `stfs f0, 0(r30)` | Writes the same value to actor `+0x200`. |

The adjacent DOL float words at `0x80254578..0x80254590` decode to
`0.65`, `0.92`, `1.0`, `0.70`, `0.80`, `0.72`, and `0.55` as float32
(rounded here for display). Multiple branches immediately before
`0x800F8604` load different words from this table. Their conditions and
game meanings have not been established. The saved actor block used by the
verified vector bridge begins with float32 word `0x3F3851EC`, which is the
table's `0.72` value. Later live partial-strength captures show that both
strength fields are capped at that value before movement preparation on this
saved-match branch.

**Confidence:** `verified` for the DOL bytes, addresses, pointer arithmetic,
same-value final stores, decoded float words, and the saved-match cap observed
in the later partial-strength runs. `candidate` for applying the same table
selection in other game states. No general strength algorithm is claimed.

## Live capture gate

Earlier on 2026-10-07, a bounded partial-right stick capture and a known full-right
control were attempted with the existing Dolphin 2603a container, the
GNNE69 image, and supplied savestate `8287f395`. Both reached a rendered
frame, but the roster root at `0x803A1E10` remained zero, even after a
20-second bounded wait. Neither captured an actor update. The Dolphin
process log showed EGL/DRI3 warnings but no explicit savestate failure.
Those failed runs provide no evidence about partial-strength behavior.
Subsequent fresh sessions restored the known match, located Thomas, and
completed the four sequences documented above. Diagnostic
logs and empty trace files remain in the container's temporary storage.

The completed captures checked translator entry/return, movement preparation,
and complete actor/movement blocks against full-strength controls. The DOL
stores at `0x800F8254` and `0x800F8618` were separately observed by
write watchpoints in the saved match.
