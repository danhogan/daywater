# Native Windows on-field input check

**Confidence:** `verified` for automated controller slot 0 input in the tested
saved match state. Manual X and arrow-key input in the staged Windows launcher
was observed by the user; other players, modes, and hardware controllers have
not been tested.

## Identity and runtime

- Game: NFL STREET, GameCube `GNNE69`, North America revision 0.
- Local ISO SHA-1: `813c4511ba3135059a13de424555647e8252254b`.
- `main.dol` SHA-1: `46e3de854998a0f66c9be8c72a1164d22cd29eb4`.
- Native runner: ModernGekko commit `edd460adad0e82b56909af1d793642c83b6d31c8`, with vendored Dolphin commit `10f7ac4fe48f74485fbc161ebf4d67242e11e671` and the local boot-image and MSVC lambda patches documented in [recomp.md](recomp.md).
- Starting savestate: `W:\Daywater\research-temp\nflstreet-moderngekko-win-vulkan-automation\artifacts\early-play.sav`, SHA-256 `fc82b85bfc29c9d91d45e89d26e18273c9a62807f6912631a1a2f37bf6565023`.

## Controlled procedure

`probe_windows_controls.py` started a fresh Windows runner process for each
trial and loaded the same pre-snap state. It sent three frames of GameCube A
input, then held the main stick left (`main_x=-1`), neutral, or right
(`main_x=1`) for exactly 180 emulated frames through ModernGekko automation.
The game can auto-snap, so this procedure does not assign a snap effect to A.
Immediately after the 180-frame input, the script captured a screenshot and
read 2 MiB of guest memory starting at `0x803A0000`. All six runs stopped
cleanly with zero interpreter fallback steps and no automation error.

The existing [selected-player mapping](../../../research/nfl_street/selected_player.md)
was followed in each memory image: the big-endian pointer at `0x803A1E30`
led to the controller-entry array, whose slot 0 contained team `1`, index
`0`. The roster-manager pointer at `0x803A1E10` resolved that entry to actor
`0x803C04EC`, named `Green` at actor `+0xB80`. The ground X and Y values are
big-endian IEEE-754 float32 at actor `+0x1A8` and `+0x1AC`, respectively.
The addresses of the pointer slots and offsets match the earlier
[position research](../../../research/nfl_street/player_position.md); the actor
address is specific to this allocation history. The static reader and lookup
functions are described there; no native Windows writer PC was captured.

| Input | Trial 1 X | Trial 2 X | Trial 1 Y | Trial 2 Y |
|---|---:|---:|---:|---:|
| Neutral | 1.587 | 0.555 | -33.718 | -34.040 |
| Left | -12.609 | -13.753 | -35.809 | -36.033 |
| Right | 8.694 | 6.430 | -36.568 | -39.690 |

The screenshot marker names Green in the captured play. He moves left in
both left trials and right in both right trials, while neutral remains near
the center. The actor identity and X separation agree across independent
processes. Captures and raw memory images are under
`W:\Daywater\research-temp\nflstreet-moderngekko-win-control-{neutral,left,right}`
and the corresponding `-repeat` directories. They remain outside Git.

This verifies that the native Windows runner's automated GameCube main-stick
input affects the selected player on the field. The user also confirmed that
X and the arrow keys work in the staged Windows launcher using the copied
Dolphin keyboard profile. This manual report does not establish a particular
frame-level effect for X, input latency, or full-match stability; the user
reported that play is very slow.

The three conditions were repeated once using the isolated
`StaticRecompIdlePC=0x801F471C` profile described in [recomp.md](recomp.md).
Green remained the slot-0 actor; X was -0.368 neutral, -12.609 left, and
+8.916 right. The separate `-idle` W: capture directories retain those
screenshots and memory images. This confirms that the faster idle-loop test
did not erase the observed directional input effect in this play.

The private local Windows bundle was tested separately with the same
`early-play.sav` and `probe_windows_controls.py`, using its copied runner,
module, ISO, game tree, and profile. Fresh neutral, left, and right trials
ended at frame 17,201 with zero interpreter fallback steps and clean exits.
Green's X was +0.0301, -8.6527, and +8.4676 respectively after 180 input
frames. The `-bundle` W: capture directories contain the frames and memory
reads. This confirms the copied launcher inputs still reach the game after
the runner's `Sys` resources are included.
