# Native Thomas actor-update replay

The guarded [Windows C replay](actor_update_replay.c) begins at the observed
`0x800F705C` movement-preparation entry and runs three native stages:

```text
actor input fields and kinematics block
  -> acceleration preparation (0x800F705C branch)
  -> result heading and command stores (0x800F6FA4, 0x80036B18)
  -> position/velocity update (0x80036B28 branch)
```

It reads the actor header byte, the actor's signed rating at `+0xBB4`, the
`+0x200..+0x22F` input block, and the `+0x1A8..+0x1FB` movement block. The
rating, motion fields, and constant locations come from the original GNNE69
revision 0 DOL (SHA-1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`) and the live saved match.
The preparation formula uses the original constants at `0x8025446C..80`
and `r13-0x70E8..70D4`, checked in the supplied second-down MEM1 image.
It uses the native reconstruction of the game's sine polynomial for the
observed partial-turn branch. The command heading is calculated from the
actor and movement fields; the acceleration value is calculated from the
prior acceleration, turn angle, speed, and actor rating.

Three fresh Dolphin 2603a runs from supplied state `8287f395` held right,
forward, and backward input. The [capture tool](../../tools/capture_thomas_kinematics.py)
stopped at preparation entry, command entry and return, and movement entry
and return for Thomas. The actual Windows x64 replay matched all 21 words
of the state block at each of the three post-entry boundaries in every run.
The tested angular differences at preparation entry were `0`, `0x4C71B`,
and `0x1CDEC`, respectively. These validate the zero-turn and partial-turn
branches. The traces do not exercise the saturated-angle branch, the other
actor-type branch, or a fresh game state with different global parameters.

| Input | Private W: trace SHA-256 |
|---|---|
| Right | `feb1564347024f3024efa926f96f2c439b0a71ae0edd04fdc5a6f0c4ab2329c9` |
| Forward | `365b34d0a900b1cab636baae668ae093ea222ae9b74a71f5c9e9de484a35ccb1` |
| Backward | `b6675434b8d258086d6acbb8173bd5d3890c39a2b5a92e07b0b786efbc690997` |

Build with
`bash projects/nfl-street/reconstruction/build_windows_actor_update_replay.sh`.
The checked executable at
`W:\Daywater\research-temp\nflstreet-actor-update-replay.exe` has SHA-256
`bf1b2caa6fbb9e46f0aed65d76cefcadc55af30ae32c3c5e301df71107ba5fa1`.
To verify a private trace:

```bash
python projects/nfl-street/reconstruction/command_to_movement/verify_actor_update.py \
  --trace /mnt/w/Daywater/research-temp/thomas-preparation-right.jsonl \
  --executable 'W:\Daywater\research-temp\nflstreet-actor-update-replay.exe' \
  --windows
```

The raw traces, original MEM1 image, and Windows executable stay on W:.
Temporary debugger and session changes used for live capture were removed;
the Daywater container's original source hashes were restored. The normal
Windows gameplay DLL SHA-1 remains
`dd521cc38b0db6bca98a0bb261570a2be5e6e0ed`.

**Confidence:** `verified` for the three fresh observed input/output
transitions and their three internal boundaries. The source is a bounded
reconstruction of those branches, not the full actor function. It requires
already-populated actor input fields and a valid movement block. It does not
schedule repeated updates, manage the rest of the match, load assets, or
render a native game.

The [consecutive update replay](update_sequence.md) now checks two separate
three-call sequences with an explicitly modeled intercall acceleration reset.
