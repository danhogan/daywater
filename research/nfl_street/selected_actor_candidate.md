# NFL Street (GameCube): actor reference in a nine-object array

## Finding

**Confidence:** `verified` for the pointer chain and original-match correlations;
`candidate` as a general selected-player locator  
**Date:** 2026-10-01  
**Game:** NFL STREET, `GNNE69`, North America, revision 0  
**ISO SHA-1:** `813c4511ba3135059a13de424555647e8252254b`  
**DOL SHA-1:** `46e3de854998a0f66c9be8c72a1164d22cd29eb4`  
**Dolphin build used for captures:** Official Linux 2603a

A big-endian 32-bit field (4 bytes) at `object + 0xFC` holds an actor pointer.
The first object is reached through the static DOL global at `0x8027BE8C`.
In the supplied Bears–Lions match, the object was at `0x80DDFCD0`, making
the field `0x80DDFDCC`. It followed the on-screen selection marker in the
tested transitions there. The object moves in other matches, and a fresh
Steelers–Cowboys match gives a probable counterexample to interpreting this
field as the selected player in general. Do not use it as a generic locator.
The [controller 0 selection mapping](selected_player.md) is the verified
selected-actor locator for the tested game image and states.

## Procedure and observations

For each retained 24 MiB MEM1 image, read the pointer at `0x80DDFDCC`, then
read the roster manager through `0x803A1E10`, its actor array pointer, and
the actor stride `0x14D4` documented in [player_position.md](player_position.md).
Decode the referenced actor's name at `actor + 0xB80`. Compare that name with
the selection marker in the matching frame image.

| Capture | Pointer value | Roster actor | Visible marker |
|---|---:|---|---|
| `position-longleft-00` | `0x803C04EC` | Stewart, slot 7 | Stewart |
| `position-longleft-05` | `0x803C04EC` | Stewart, slot 7 | Stewart |
| `position-longleft-06` | `0x803C19C0` | Thomas, slot 8 | Stewart |
| `position-longleft-10` | `0x803C19C0` | Thomas, slot 8 | Thomas |
| `position-alt-second_left-00` | `0x803C19C0` | Thomas, slot 8 | Thomas in the supplied second-down play |
| `position-alt-seven_neutral-00` | `0x803C04EC` | Kreutz, slot 7 in that state | Kreutz |

The original MEM1 captures for savestates `19bb4a7c`, `8287f395`, and
`27bb155a` contain the corresponding Stewart, Thomas, and Kreutz actor
pointers. The three other supplied original states (`9cf343e0`, `a419c605`,
`ca793e43`) contain zero at this address. Actor ordering differs between
states: slot 7 is Stewart in the first-down play and Kreutz in the 7–0 play.

The frame-06 mismatch is a negative observation. This field could represent
a pending selection, another game-state role, or a pointer copied before the
rendered marker updates. The earlier capture cadence was one second.

## Fresh transition experiment (2026-10-01)

The reproducible capture script at
`projects/nfl-street/tools/selected_actor_transition.py` restored
`19bb4a7c` in four fresh Dolphin 2603a processes. It sampled the pointer
every 0.05 seconds and retained a nearby dumped frame every 0.25 seconds.
The no-snap control sent no `A` press; three other runs pressed and released
`A`, then held the main stick left or right. Each run began with Stewart at
`0x803C04EC`.

| Run | Input | First Thomas sample | Remaining pointer samples |
|---|---|---:|---|
| `control-nosnap` | Neutral, no `A`, 10 s | None | Stewart in all 200 samples |
| `snap-left-1` | `A`, then left | 5.60 s | Thomas in the remaining 128 samples |
| `snap-left-2` | `A`, then left | 5.55 s | Thomas in the remaining 129 samples |
| `snap-right-1` | `A`, then right | 5.65 s | Thomas in the remaining 127 samples |

In the completed-frame captures for `snap-left-2`, the 5.75-second memory
sample points to Thomas while its nearby frame still shows Stewart's marker.
Later frames show Thomas under the marker. The right run reproduces that
order. The selected frame is deliberately several frame files behind the
newest dump to avoid copying an incomplete PNG, so the images do not measure
the exact display lag. They do confirm the stable-state association.

An aligned big-endian pointer search over the saved 24 MiB MEM1 images found
only `0x80DDFDCC` holding the marker-named actor's address in all three
independent stable states: first-down Stewart, first-down Thomas, and 7–0
Kreutz. This supports the pointer association but does not establish its
owning object or lifetime.

## Explicit defensive player switch

The 7–0 savestate `27bb155a` begins with the marker on Kreutz and the pointer
at `0x803C04EC` (roster slot 7). In two fresh Dolphin 2603a processes, a
0.15-second `X` press/release switched the pointer to `0x803C2E94`, the actor
named Stewart in slot 9. The first Stewart sample occurred 0.10 and 0.15
seconds after input. It then remained Stewart for the other 98 and 97 samples
of the respective five-second runs. Completed frame captures showed the
marker move from Kreutz to Stewart. A separate no-button control from the
same savestate stayed on Kreutz for all 100 samples and retained his marker.

This directly verifies that `X` switches the selected defender in this tested
state and that the pointer follows the selection. The captured PNGs were
deliberately selected several frames behind the newest dump to avoid partial
files, so they cannot establish the exact interval between the RAM change and
the displayed marker change.

A GDB write watchpoint on `0x80DDFDCC` returned 20 stops from 15 PCs while
the pointer still held Stewart. As with the earlier position watchpoint, the
stop PCs cannot be accepted as exact writers. No Ghidra writer was renamed or
linked as an observed writer from this probe.

## Static owner and code evidence

In all six original 24 MiB MEM1 captures, the big-endian pointer at
`0x8027BE8C` is `0x80DDFCD0`, and `0x80DDFCD0 + 0xFC = 0x80DDFDCC`.
The field at the derived address is Stewart, Thomas, Kreutz, or zero according
to the state. `0x8027BE8C` is the first entry in an array of nine pointers,
each pointing to a separate `0x104`-byte object in these captures. This rules
out interpreting the neighboring `0x802AC030` pointer as the owner: it points
to the next adjacent object.

Ghidra analysis of DOL SHA-1 `46e3de854998a0f66c9be8c72a1164d22cd29eb4`
shows `FUN_800e46c8` registering object size `0x104`, `FUN_800e47fc`
populating the pointer array starting at `DAT_8027be8c`, and
`FUN_800e4a24` writing the result of `FUN_800e3c48` to `object + 0xFC`.
`FUN_800e3dc8` initializes that field to zero. These are static code
observations; the GDB watchpoint did not establish a runtime writer PC.
The code suggests this object drives a marker or visual effect, but the exact
object role is still a `candidate` interpretation.

Two additional fresh Dolphin 2603a runs from `27bb155a` sampled the literal
field and the static chain every 0.05 seconds. Both had
`read_u32(0x8027BE8C) = 0x80DDFCD0` and the chain matched the literal actor
pointer in all 100 samples per run. The first run's `X` input did not switch
the actor; Kreutz persisted for 100 samples. In the second run, `X` switched
Kreutz to Stewart at sample 1 (0.0501 seconds), and Stewart persisted for the
other 99 samples. The derived pointer followed the change in every sample.
The first run is retained as an input nonresponse, not as a failed chain
observation. Raw CSV, metadata, and frames are under
`W:\Daywater\research-temp\selected-actor-chain-defender-x` and
`selected-actor-chain-defender-x-2`.

## Different allocation histories and counterexample

Six separately started Bears–Lions match captures already retained for the
[score investigation](score.md) have `read_u32(0x8027BE8C) = 0x80DDFAF0`,
placing the first object's `+0xFC` field at `0x80DDFBEC`. The field is zero
at each captured play-selection screen. The old literal `0x80DDFDCC` instead
contains `0xBF5E3A00`. This independently verifies that the literal address
does not survive a new match allocation history.

I also booted the ISO without a savestate in a fresh Dolphin 2603a process,
selected a Steelers–Cowboys Quick Game on EA Field, and followed the global
while entering a match. After allocation, `read_u32(0x8027BE8C)` was
`0x80DDFB30`, putting the field at `0x80DDFC2C`; the former literal
`0x80DDFDCC` read zero. This is a third observed object location. During
the match, the first object's field held `0x803C04EC`, whose name bytes at
actor `+0xB80` read `Faneca`. A nearby pre-snap frame labeled the controlled
player `Maddox`. During a later PAT, the first object still referenced
Faneca, while array entry 8 referenced `Carter`; a frame displayed both
names. The frame copies lag memory by several completed frames, so the
Maddox/Faneca mismatch is a `probable` counterexample rather than a precisely
synchronized proof. It is enough to withhold a general selected-player
interpretation.

Selected clean-boot frames are retained under
`W:\Daywater\research-temp\clean-boot-selected-actor-probe\`. The full
temporary Dolphin frame dump is archived there as
`full-dolphin-frame-dump.tar`. A complete archive-to-source comparison passed
before the duplicate cache frames were removed.

## Next verification

Identify what each of the nine `0x104`-byte objects represents and how its
`+0xFC` actor reference is chosen. Find the authoritative selected-player
field separately, using synchronized frame and RAM captures across teams.
The exact transition timing relative to the marker remains open.

## Reproduction environment

The normal Daywater container has a newer Dolphin build that cannot restore
these 2603a savestates. The timed runs used the [official Dolphin 2603a Linux
Flatpak](https://dolphin-emu.org/download/release/2603a/) and its KDE 6.10
runtime inside the existing container, with a temporary launcher at
`/tmp/daywater-dolphin-2603a`. The downloaded bundle is retained at
`W:\Daywater\tools\dolphin-2603a-x86_64.flatpak` (SHA-256
`99923279bd8fe0a8f70fdc2ccb30a3bfd800d465ee025acea151349edb6d595b`).
Rebuilding the container removes the temporary launcher and runtime install;
the bundle and raw captures on W: remain available.

Raw MEM1 images and matching frames remain outside the repository under
`W:\Daywater\research-temp\`. The new timed runs are under
`selected-actor-control-nosnap`, `selected-actor-snap-left-1`,
`selected-actor-snap-left-2`, `selected-actor-snap-right-1`,
`selected-actor-defender-control`, `selected-actor-defender-x-1`, and
`selected-actor-defender-x-2` there. The
first left run retains CSV and metadata only because its initial frame copies
were incomplete. The raw watchpoint diagnostic is
`selected-actor-watch.json` in the same W: directory.
