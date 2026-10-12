# NFL Street (GameCube): controller 0 selected actor

## Finding

**Confidence:** `verified` for the tested game image, controller slot, states,
and Dolphin build.  
**Date:** 2026-10-02  
**Game:** NFL STREET, `GNNE69`, North America, revision 0  
**ISO SHA-1:** `813c4511ba3135059a13de424555647e8252254b`  
**DOL SHA-1:** `46e3de854998a0f66c9be8c72a1164d22cd29eb4`  
**Dolphin:** Official Linux 2603a

The controller mapping for slot 0 holds a team byte and an actor index byte.
Following those bytes through the roster manager resolves the actor named by
the on-screen selected-player marker in the tested plays. A value of `0xFF`
for either byte means there is no mapped actor; it appears during play
selection and some transitions. The mapping is a better runtime locator than
the first visual object's actor reference described in
[selected_actor_candidate.md](selected_actor_candidate.md).

```python
mapping = read_be_u32(0x803A1E30)
entries = read_be_u32(mapping)
team = read_u8(entries + 4 * controller_slot)
index = read_u8(entries + 4 * controller_slot + 1)
if team == 0xFF or index == 0xFF:
    selected_actor = None
else:
    manager = read_be_u32(0x803A1E10)
    actor_base = read_be_u32(manager)
    actors_per_team = read_be_u16(manager + 8)
    selected_actor = actor_base + (team * actors_per_team + index) * 0x14D4
```

The verified controller slot is `0`. The static root `0x803A1E30` is a
4-byte big-endian pointer. Its first word is a 4-byte big-endian pointer to
the entry array. The team and index fields are each one byte; endianness does
not apply to those bytes. The roster base is a 4-byte big-endian pointer, the
per-team actor count is a 2-byte big-endian unsigned integer, and the actor
stride is `0x14D4` bytes. The observed mapping object was `0x803CD920`, its
entry array `0x803CD960`, and the entry-0 index byte `0x803CD961`; these are
runtime targets, not constants for other allocation histories or revisions.

## Controlled verification

The capture tools are
`projects/nfl-street/tools/selection_memory_pair.py` and
`projects/nfl-street/tools/clean_match_selection_probe.py`. Both use the
official Dolphin 2603a build. Paired runs restore a savestate, wait for a
stable frame, capture full 24 MiB MEM1 and a completed frame, apply one
controller input, and capture another pair. A no-input run from the same
state controls for ordinary passage of time.

| State and procedure | Entry 0 before | Entry 0 after | Resolved and visible actor |
|---|---|---|---|
| Supplied 7–0 state, `X` run A | team 1, index 0 | team 1, index 2 | Kreutz → Stewart |
| Supplied 7–0 state, `X` run B | team 1, index 0 | team 1, index 2 | Kreutz → Stewart |
| Supplied 7–0 state, no-input control | team 1, index 0 | team 1, index 0 | Kreutz throughout |
| Supplied 0–0 offense, `A` snap and 8 s wait | team 1, index 0 | team 1, index 1 | Stewart → Thomas |
| Clean Giants–Dolphins Quick Game | team 1, index `0xFF` during intro | team 1, index 0 before snap; index 1 after snap | None → Collins → Barber |

The two `X` runs were separate fresh Dolphin processes. Their before and
after frames label Kreutz and Stewart, respectively. The no-input process
kept both the mapping index and marker on Kreutz. The offense snap's later
frame labels Thomas. In the clean match, a stable pre-snap frame labels
Collins and a later stable frame labels Barber, agreeing with the mapping's
actor names. An earlier frame near the Collins-to-Barber transition still
showed Collins after the RAM index changed, so frames must not be treated as
exactly synchronous with memory samples.

All six supplied original MEM1 captures and six separately started
Bears–Lions score-study captures were checked. The active supplied states
have the expected index (`0` or `1`), while play-selection captures have
index `0xFF`. No actor is decoded from the sentinel. The clean Quick Game
started without a savestate, providing a separate match and allocation
history. The tested roster count was seven actors per team; the locator reads
that count from memory rather than assuming seven.

## Static code evidence

Ghidra disassembly of this DOL sets `r13` to `0x803A9A40` at startup.
`FUN_8003D600` reads the mapping pointer from `r13 - 0x7C10`
(`0x803A1E30`), reads the two entry bytes, rejects `0xFF`, and calls
`FUN_8003A4A0(team, index)`. That function reads the roster manager from
`r13 - 0x7C30` (`0x803A1E10`) and calculates
`actor_base + (team * actors_per_team + index) * 0x14D4`.
`FUN_8003D788` writes an actor's index byte into the controller entry when
assigning control; `FUN_8003D704` clears an entry to `0xFF`. These function
roles are supported by the listed instructions and the controlled memory
transitions. No GDB writer PC is claimed from the earlier unreliable
watchpoint experiment.

## Scope and retained evidence

This verifies the mapping for controller slot 0 in the tested game image and
Dolphin build. Additional controller slots, game modes, regions, revisions,
and edge states need separate tests. The mapping identifies the actor
assigned to controller slot 0; whether every visual effect or possession
state uses that actor is a separate question.

Raw MEM1 images, paired frames, and metadata are under
`W:\Daywater\research-temp\selection-memory-defender-x-a`,
`selection-memory-defender-x-b`, `selection-memory-defender-control-a`,
`selection-memory-offense-snap-a`, and `clean-match-selection-probe`.
No game image, proprietary asset, savestate, or RAM dump is in the repository.
