# Selected PLAY head geometry inspection

## Checked resource join

For selected team 1 / PLAY `DIGP=355`, the original name key is
`HEAD_MODEL_STEWARTKORDELL_2700`. The controlled untouched-DOL lookup run in
`verify_team_player_resources.py` checked the uppercase and lowercase query
against `801A8088`; the unique head index is 12. The corresponding unpacked
face model has `Info=StewartKordell2700.ea3` and SHA-256
`b4f9e7712eeaea9a3746e400a90f363c361255930220cd0955e56a67c88df060`.
The original lookup catalog also marks the Info name as a match.

The source model contains 114 Geom rows, 770 triangles, 75 skeleton bones, 13
cluster rows, and 3 material records. Its Skel scale vectors contain
non-unit values. The native `team_player_head_open` API loads this exact face
archive index and holds the decoded TMdl bytes for the lifetime of the mesh.
`team_player_head_render` produces a flat stored-pose inspection image.

## Native support and confidence

The selected model includes empty Geom descriptors; the model reader now skips
only zero-byte rows with zero offsets. Its stored-pose reader now composes
per-bone scale with local rotation before parent-cluster concatenation. The
independent Python reader does the same. For this selected head, the native
loader returns 770 triangles and agrees with Python within `5.96e-8` maximum
absolute float error. Existing model regression cases still match their
independent reference bytes exactly (14 cases, 10,582 triangles).

The PLAY identity-to-file join is **verified** through the original row-key
lookup and matching TMdl Info name. The stored-scale pose composition remains a
**candidate**. The native C and independent Python readers agree, which checks
that the two implementations are consistent; it does not establish that the
original renderer uses this R*S composition. `801CA4E8` is an animation matrix
builder that reads caller interpolation arrays; it does not consume the TMdl
scale vectors. There is no original matrix oracle yet for this static head
pose.

The head-only inspection renderer does not load external texture references,
reproduce original materials or palettes, attach the head, apply live pose, or
choose hair. The separate selected body/head candidate and its limits are
documented in [team_player_selected_pair_preview.md](team_player_selected_pair_preview.md).
Neither inspection uses the fixed NAP1 reference actor.

## Private reproduction

Private inputs and outputs stay under
`W:\Daywater\research-temp\nflstreet-team-player-loader`. Build the adapter
with `build_windows_team_player_selected_head_preview.sh`, then run it against
`plaface.dat` using the original lookup index. The generated BMP and mesh dump
are checked by `verify_selected_player_head_geometry.py` against the private
catalog, prior original lookup report, DOL SHA-1, unpacked source file, and
independent Python geometry reader. The checker records SHA-256 values before
and after and rejects changed protected inputs.
