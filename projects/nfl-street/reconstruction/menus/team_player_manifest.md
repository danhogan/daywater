# Selected player resource manifest — 2026-10-11

The independent C manifest reads one exact team/player PLAY row from a version8
team database. It retains the raw appearance fields, checked portrait selector,
body-pool key and normalized head/hair lookup keys. A supplied lookup provider
returns resource indices. These callback selectors are local API values, not
original game enums.

## Evidence and confidence

**Verified native contract:** the Windows fixture checker exercises exact-row
selection, raw fields, name normalization, injected lookup responses and atomic
failure. Its database and lookup responses are synthetic. This test does not
establish full original player loading or all real PLAY rows.

The subsequent [real-resource resolver gate](team_player_resolve.md) checks all
402 actual regular-team PLAY rows and 2,416 original lookup queries. That gate
also corrected the synthetic fixture's appearance tags against the original
getters: PHAR, PLHT, PJEN, PLEX and PHGT use their recorded raw field tags.
`tghp` and `phgt` denote the same raw PHGT field, not separate database fields.

The component rules come from the bounded original checks documented in
[the player loader](team_player_loader.md): original name lookup, portrait
selection and TBYP body-key routing. The head lookup miss fallback is `0x28a`.
TBYP7 routes to body key2; supported actual regular-player keys are0,1,2.
The body keys now join particular model files through the **verified bounded**
original descriptor/attachment check in `verify_team_player_body_binding.py`.
That check covers all nine key/variant combinations with actual TMdl records;
runtime menu variant selection and complete resource registration remain open.

Both normal and hat hair candidates are retained. The original derived preview
flag choosing between them is unresolved; PLHT is not used as a substitute.
For YTRP!=0, portrait selection is available but model keys remain unresolved.
The manifest does not assemble geometry, select material/palette overlays or
render a selected athlete. The earlier captured reference renderer cannot be
used as proof of that athlete's appearance.

## Reproduction

`build_windows_team_player_manifest_checks.sh` builds the Windows fixture checker
privately on W:. `verify_team_visible_input.py` runs it alongside repeatable
team-screen captures and records source/build/input hashes. No proprietary
assets or generated models are embedded in this code.

Game evidence: NFL Street GameCube GNNE69-r0, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. No new live Dolphin integration
or full menu-to-match startup is claimed.
