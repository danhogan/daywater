# NFL Street (GameCube): ball-position record

## Finding

**Name:** Live ball-position fields and a team-relative copy  
**Confidence:** `verified` for the recorded ball-position behavior; source-versus-copy authority remains unknown  
**Date:** 2026-10-01  
**Researcher/agent:** Codex

## Game identity

| Item | Evidence |
|---|---|
| Title / platform | NFL STREET / GameCube |
| Game ID / region / revision | `GNNE69` / North America (`E`) / disc 0, revision 0 |
| ISO SHA-1 | `813c4511ba3135059a13de424555647e8252254b` |
| DOL SHA-1 | `46e3de854998a0f66c9be8c72a1164d22cd29eb4` |
| Dolphin build | Official Linux 2603a |

## Corrected locator

The static big-endian pointer slot at `0x803A2670` is in DOL initialized
data section 10 (`0x803A1A40..0x803A3D3F`). The DOL initializes PPC `r13`
to `0x803A9A40`; Ghidra code accesses this slot as `r13 - 0x73D0`.
`FUN_800BE848` allocates a record of `count * 0xF4 + 8` bytes through
`FUN_801EF08C`, passing the literal tag `0x62616C6C` (`ball`). In all
tested snapshots, the slot points to `0x803B6F00`, with count `1`.

```text
ball_table = read_be_u32(0x803A2670)
count      = read_u8(ball_table)
ball_obj   = read_be_u32(ball_table + 0x08)  # observed 0x80DEEE24
world_x    = read_be_float32(ball_obj + 0x04)
world_y    = read_be_float32(ball_obj + 0x08)
world_z    = read_be_float32(ball_obj + 0x0C)

copy_x     = read_be_float32(ball_table + 0x14)
copy_y     = read_be_float32(ball_table + 0x18)
copy_z     = read_be_float32(ball_table + 0x1C)
```

| Field | Observed address | Type / size / endian | Confidence |
|---|---|---|---|
| Source X / Y / Z | `0x80DEEE28`, `0x80DEEE2C`, `0x80DEEE30` | IEEE-754 float32 / 4 bytes each / big | `verified` as ball-position values |
| Record copy X / Y / Z | `0x803B6F14`, `0x803B6F18`, `0x803B6F1C` | IEEE-754 float32 / 4 bytes each / big | `verified` as a live copy; coordinate transform possible |

The source object's lifetime, owner, and role in the game's update loop are
not yet known. Read both pointers at runtime; the observed heap addresses
must not be hard-coded. The record copy was initially mistaken for a
controlled-player coordinate because Thomas carried the ball during the
first five experiments. It is **not** a player-position locator.

## Test procedure and observations

Restore supplied 0–0 first-down savestate `19bb4a7c` in fresh Dolphin 2603a
processes. Press/release `A` to start the play, then hold left, neutral,
right, forward, or backward main-stick input for 18 real seconds. Capture
live MEM1 and frames before input and at one-second intervals. The selected
player becomes Thomas and carries the ball. Opposite inputs visibly move
him and the ball in opposite directions. The source and record-copy floats
match in these five runs.

| Input | Source X at 18 s | Source Y at 18 s | Source Z at 18 s |
|---|---:|---:|---:|
| Left | -11.199 | -35.572 | 1.097 |
| Neutral | -2.454 | -34.943 | 0.917 |
| Right | 6.168 | -35.448 | 1.101 |
| Forward | -2.693 | -27.443 | 1.719 |
| Backward | -1.844 | -44.184 | 1.100 |

Repeat from supplied second-down midplay savestate `8287f395`: left input
changes source X from about `-12.84` to `-17.15`, while right input changes
it to about `-7.38` after six seconds. The record copy agrees in both runs.
This is a different play from the first-down tests.

In supplied 7–0 first-down state `27bb155a`, the selected player is defender
Kreutz, while the ball source object reads `(0, +33, 0.097)` and the record
copy reads `(0, -33, 0.097)`. The copy therefore cannot be assumed to share
the source's world orientation in every state. The difference is consistent
with a side-of-field transformation, but its exact rule is unverified.

The other four supplied states and the controlled captures all had the same
pointer values and count. This establishes the chain for these recorded
states, not for every allocation history or revision.

## Ghidra relationships

- `FUN_800BE848`: allocates the `ball`-tagged table at `r13 - 0x73D0`.
- `FUN_800BE36C`: computes `table + index * 0xF4 + 8`, confirming the
  first record layout used above.
- `FUN_800BEB20`: negates multiple coordinates in each record. This is a
  possible contributor to the source/copy sign difference; a call trace
  has not established when it runs.
- Exact live writer of the source object coordinates: not identified.

A live Dolphin GDB write watchpoint on the record-copy X address fired 13
times in three seconds of movement, while a watchpoint on the static pointer
slot fired zero times. The GDB stop PCs included instructions unrelated to
direct stores, so they are retained as leads only and no writer function is
assigned from them.

## Retained evidence and limits

The 24 MiB live MEM1 images and screenshots are outside the repository under
`W:\Daywater\research-temp\`. The five main series are
`position-longleft-00..18`, `position-longneutral-00..18`,
`position-longright-00..18`, `position-longforward-00..18`, and
`position-longback-00..18`; each index has `.mem1` and `.png`. The second-down
and 7–0 probes use `position-alt-*`. The six original states use
`original-<savestate-id>.mem1`. The temporary main capture script is
`position-capture.py` on W. No game image, savestate, raw asset, or RAM image
was added to the repository.

The ball-height field's physical interpretation and the authoritative
source of ball motion remain open. [player_position.md](player_position.md)
documents the separate Thomas actor coordinates.
