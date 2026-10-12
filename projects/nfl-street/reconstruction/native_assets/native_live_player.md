# Live native player on the field

## What works

The Windows `live-player.exe` loads original extracted assets once, evaluates
compressed animation poses with time interpolation, rebuilds the posed player
mesh, and renders it over a cached native field and sky. The field cache holds
both color and depth, so actor draws still obey scene occlusion. No Dolphin,
ModernGekko DLL, ISO, saved RAM, or pre-rendered animation frames are loaded.
The static private draw profiles retain captured camera, light, appearance and
material selections; independence from those selection profiles is unfinished.

Private launcher: `W:\Daywater\Start-NFLStreet-LivePlayer.cmd`.
Click the window to give it keyboard focus:

- WASD or arrows: move and face the movement direction.
- R: reset placement and animation clock.
- Escape: close.

These are host inspection controls. Speed (5 field units/second), rectangular
movement limits, camera, clip choices, phase rates and instantaneous facing are
chosen preview behavior. They are not reconstructed NFL Street gameplay.

## Evidence and confidence

Target: GameCube NFL Street GNNE69 revision 0. Original DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`.
Private evidence:
`W:\Daywater\research-temp\nflstreet-live-player-20261008`.

- **Verified decoder:** all 18,752 common-bank frames were compared exactly
  with the unchanged original module in the previous
  [animation milestone](native_player_animation.md). This includes both clips
  used here, from direct bank `animdata/00103.bin` and compact skeleton
  `animdata/00095.bin`.
- **Candidate locomotion interpretation:** clip 575 repeats 16 frame references
  and terminates with original cyclic word `0x8001`. Independent decoding
  measures opposing hip-angle correlation about -0.90249 and pronounced knee
  motion. Native images show a run-like stride. Clip 18 is the chosen stationary
  cycle. Their original names, gameplay eligibility and selection policy have
  not been established against a live match.
- **Probable interpolation:** original cached function `0x80166730` wraps a
  signed-short angular difference and multiplies by a 12-bit fixed-point weight.
  The viewer applies that formula to adjacent decoded frame angles. The time
  source/rates and handling of clip changes are host choices. No independent
  original-module interpolation oracle was run for this milestone.
- **Probable posing math:** original weighted cluster construction was checked
  previously against 128 original cluster matrices; host trigonometry remains
  approximate (largest observed float difference 0.000241041183).
- **Verified own cache behavior:** rebuilding the initial posed mesh from cached
  model bytes matches every original initial mesh byte. Rendering the actor over
  cached field color/depth matches the same combined scene render exactly.
  Offline checks cover actor occlusion behind the field, visibility in front,
  and preserving the cache across overlays.
- **Verified own Windows behavior:** two fresh actual Windows window runs each
  pass 60 timer updates/paints. Keyboard messages go through the window's actual
  event handler: right, forward, diagonal, then focus loss. Both reach
  x=0.732232, y=-15.732241 after 45 moving updates and 7.499997 units of requested
  travel. Five rendered checkpoints repeat byte for byte. This is automated
  message-input coverage; a human physical-key trial was not performed here.

Model Y-up is mapped to field Z-up. This inspection placement raises the
hip-centered skeleton by 1.1593527793884277 units, the observed body skeleton's
`IDEAL_OUT` bind translation Y. That supplies a visible ground placement, not
original animated root travel or foot locking. Hair follows the native full
neck cluster rigidly. The template face and omitted secondary skin texture
retain the limitations of the [fixed player renderer](native_player_render.md).

## Build and checks

Run `bash projects/nfl-street/reconstruction/build_windows_live_player.sh`.
Builds and asset-dependent diagnostics go to W:. The executable accepts:

```text
live-player unpacked-root player-profile envrnmt.dat field-profile output-directory [--render-check]
```

The output directory must exist. `--render-check` exercises the window and saves
five native BMP checkpoints there, then exits with a failure status if cache
comparisons, rendering, input positions or focus clearing fail.

The existing offline `field_checks.c` and `pose_checks.c` also pass. Baseline
ModernGekko runner source and original module hashes remain unchanged; the
viewer does not load or patch either file.

## Remaining work

Connect the previously checked native movement slices and original clip
selection to this visible actor, with acceleration, turning and root travel.
Then join contacts, animation events, other actors and ball behavior. Native
match initialization, collision, rules, audio and menus remain unfinished.
Rendering still lacks personalized heads, a secondary skin plane, moving
camera/scene objects, exact original pixel matching and original animation
transition semantics. The current result is an interactive native inspection
viewer with one controllable player, not a playable football match.
