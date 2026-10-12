# Selected PLAY body/head geometry comparison

For team 1 / `DIGP=355` (Kordell Stewart), the checked PLAY resource catalog
and original lookup oracle select `StewartKordell2700.ea3` from face archive
entry 12. The checked TBYP value is 2. The bounded original body descriptor
and attach check maps it to PLYRBODY key 2; variant 0 resolves to archive entry
7, `ThinPlayer2700.ea3`. Variant 0 is an explicit host inspection choice.
This does not establish which body variant the menu runtime chooses.

The new `team_player_selected_pair` API opens the head by the caller-supplied
verified face index and opens the body through the original TBYP/key/variant
binding. It returns independently decoded stored-pose meshes and offers both
a side-by-side geometry comparison and an optional combined candidate image.
The comparison panels are independently fitted to their own bounds. The
captured comparison has 770 head triangles and 15,524 body triangles.

## Confidence and limits

The head lookup-to-file join and the bounded TBYP-to-body archive join are
**verified** at their documented boundaries. The stored-pose geometry is a
**candidate** view. The two models each carry 75 bones. Their root
translation/rotation/scale bytes match exactly, and the body model has the
same parent-bone chain used by the face model. All face-cluster-used bone
transforms match byte-for-byte in the body model; the only stored transform
differences across all 75 bones are on bones 31 and 58, which the face cluster
does not reference. This supports a common stored coordinate space as a
candidate, but does not establish original actor attachment, animation, or
renderer matrix semantics.

The combined image applies no extra placement transform and keeps both meshes
in those stored coordinates. It is a candidate head/body geometry
composition, not a verified original athlete pose or original pixels.

The flat inspection does not load the model's external textures/material
state, resolve hair/hat selection, add equipment, or apply original animation
and actor transforms. The native menu's actual variant choice and complete
body/head assembly remain unresolved.

## Reproduction

Run `verify_team_player_selected_pair.py`. It rebuilds the private Windows
inspection executable, checks the prior original head-lookup and body-binding
reports, renders repeated side-by-side and combined-candidate captures, and
compares their hashes. All proprietary inputs, executables, images, and reports remain under
`W:\Daywater\research-temp\nflstreet-team-player-loader`.
