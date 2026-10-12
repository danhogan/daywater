# Visible native football inspection — 2026-10-09

## What now works

Subsequent milestone: [native ownership inspection](native_ball_ownership.md)
uses the checked assignment record to attach this mesh to either player and
release/reassign it. The fixed mode described below remains available.

The independent Windows viewer can draw the real GameBall mesh and texture on
the first reference player's animated left wrist. It follows decoded native
locomotion, direction changes, braking, set transitions and contact inspection
poses. Field, both players and football share retained depth, allowing normal
occlusion. This runtime loads local unpacked assets and supplied profiles; it
uses no Dolphin, ModernGekko module or ISO while running.

This is **fixed attachment inspection**, not original possession selection or a
playable match. The host chooses actor0, left compact wrist24 and ordinary render
mode. Original compact root/extra-bone routing, model profile selection,
ownership events, release/pass/catch and fresh roster/match startup remain open.
The two wrist diagnostic records do not represent two possessed footballs;
only one selected wrist is rendered. Lighting/camera state remains supplied
inspection data, and no original-frame pixel-equivalence claim is made.

## Existing knowledge reused and verified

The prior FIFO geometry matches already identified 105 shared GameBall draw
commands. `export_player.py --ball` now uses those exact frame1 draws 4224–4328,
rather than guessing a model from appearance. Both original reference/repeat
captures produce the same private draw profile:

- 105 primitives, 513 triangles and 2,892 referenced attribute comparisons per
 capture; exact original command/position/normal/UV matches.
- Shared source aliases `statmod/00011.bin` GameBall.ea3 and00012 GameBallLow.ea3
 are retained in the report; the deterministic selected source is 11. This is
 not a complete original model-selection policy.
- Full original CMPR texture planes match `statmod/00013.bin`, image 0, including
 recorded resource coverage. Texture decoding reuses the existing native CMPR
 implementation; no invented substitute texture is used.
- Captured draw settings: one TEV stage, no culling, lit RGB (`51E`), unlit alpha
 (`51C`), source material alpha80, blend54B9, depth17, generator280.

The football uses rigid display lists without per-vertex PN indices. The offline
FIFO inspector's new optional `--rigid-state` includes its complete XF normal,
lighting and palette state; existing `--actor-state` behavior is preserved.
The exporter remaps the selected fixed position/normal matrix into profile slot 0.
The native primitive decoder accepts this measured rigid form and adds statmod
as category 6. Indexed player forms retain their previous behavior. The renderer
allows the observed unlit-alpha form and reads its lighting controls from the
correct profile fields.

The independent geometry check compares the native Windows mesh with source
attribute math: all 513 triangles (1,539 triangle vertices) have exact UVs and
maximum position error 9.072122164610619e-7, below the 4e-6 checked bound.
Confidence: **verified resource identity and bounded native geometry**. Native
lighting/sampling/pixel fidelity remains **candidate**, independent of geometry.

## Original render transform

`nflstreet_ball_render_matrix` reconstructs ordinary-mode `800C05E0`, multiplying
the corrected ball quaternion by the original positive-quarter-turn X/Z
quaternion, then `801A3008` quaternion-to-matrix math. It preserves scalar float
operation order and does not add normalization that the original omits.
The original full-update oracle checks 576 render bases (5,184 basis float words)
exactly across position-neutral14/15/16, in addition to the earlier complete
ball/model checks. Translation is the supplied ball position already checked
by the attachment service; the original basis function itself leaves translation
unchanged. Alternate render mode and original camera scheduling remain open.

Game identity: NFL Street GNNE69r0, DOL SHA1
46e3de854998a0f66c9be8c72a1164d22cd29eb4, captured MEM1 from Dolphin2603a.
These are controlled calls into the untouched original module, separate from
independent Windows integration checks. Confidence: **verified bounded ordinary
render transform**, not verified full original gameplay.

## Windows integration and regression checks

Actual independent Windows runs:

| Check | Result |
|---|---|
| Contact inspection | 200 ticks, 201 football layers, 6,789 shaded football samples; two contacts complete |
| Eight-direction movement/set changes | 528 ticks, 529 layers, 14,950 shaded football samples; all starts/brakes/stops pass, four set changes |
| Ball disabled |All 14 contact and 12 direction/set BMPs remain byte identical to the preceding build |
| Exporter compatibility |Both default 557-draw player profiles remain byte identical |

Ball-on/off capture differences are localized to the attachment region (up to 43
pixels per saved image at the current distant camera). Some checkpoints have
zero differences because the ball is occluded; no claim of visibility in every
frame is made. A native close-up confirms the football's original textured mesh
is drawn. The initial shared-lighting offset regression was caught by image
comparisons and corrected before all final baseline checks passed.

Original DLL SHA1 remains dd521cc38b0db6bca98a0bb261570a2be5e6e0ed; baseline
runner SHA256 remains 3f9e3a3eb4b5728d9036e9552c01e8c9dc084a1f53805fef95dcf3b1b86f73ea.
No original code, module or existing user launcher was modified.

## Try the inspection

Private Windows launcher:
`W:\Daywater\research-temp\nflstreet-visible-ball-20261009\play-football-inspection.cmd`.
WASD/arrows move player1, IJKL player2, C plays fixed contact inspection, V changes
supported player1 animation sets, R resets and Esc closes. There is no pass,
snap or tackle-control claim. All game assets, exported profiles, binaries,
captures, previews, diagnostics and verification hashes remain private on W:.

The optional viewer flag is `--ball-profile <private-NAP1-draw-profile>` alongside
`--attachment-profile <private-NAP2-or-NAP3>`. Ball draw profiles are the existing
16-byte header plus 3876-byte draw records; attachment profiles are separate
24/36/88-byte files. Do not interchange these files despite their shared legacy
magic names. Existing user launchers remain unchanged.

Build with `build_windows_live_player.sh`. To reproduce resource evidence,
build the offline inspector via `prepare_fifo_capture.py --inspector`, decode
the two existing DFF files with `--rigid-state`, then use
`export_player.py <capture-directory> --ball --output <private-W-output>`;
place decoded reference/repeat-ball-state.json in that output directory.
Geometry verification uses `verify_player_geometry.py <private-output> --ball`.

The next gameplay connection is semantic ownership and attachment/release
scheduling, followed by snap/pass/catch and natural contacts. A visible ball
following a chosen wrist establishes rendering, not those event rules.
