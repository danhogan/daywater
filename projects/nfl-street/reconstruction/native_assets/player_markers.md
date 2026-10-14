# Player-select rings and name tags following the controlled player

## Verified scope

Three native routines on saved RAM, each compared with the original module
(all RAM outside the oracle stack, a saved case plus 400 edited cases per
context, three contexts):

- `select_rings.c`: `800E4A24`, the per-frame ring update (called from the
  game-update step `8004EDFC`, between the player marks `800DA6C0` and
  `800E5520`), with its pool animation `800E44E4`, pool start `800E4F34`,
  position copy `800E41A8` and target lookup `800E3C48`. Per tracked slot
  (nine objects at table `8027BE8C`, mirrors at `8027BEB0`) the ring takes
  the followed gameplay actor's `+1A8/+1AC` (negated when the side record
  `r13-7078` has `+39 = 1`). The ring of the controlled actor (`800BE3F0`)
  also drives the ring at `r13-71E0` (`80DE02E8`); the pool at `802AC020`
  (`80DE01E4` is entry 0) animates by flags 4, 8, 0x20, 0x40, 0x10. The
  rings drawn by `nflstreet_ring_draw` are `80DDFCD0` (table entry 0),
  `80DE01E4` and `80DE02E8`. A ring is one update behind its player because
  the update runs before the next movement step.
- `camera_slots.c`: the tail of `801DFAB0`, which turns the viewport stack's
  view into slot 3 and builds slots 4 (slot 2 x slot 3), 5 and 6 (4x4
  `801895F4`). Oracle run: the original from `801DFBE8` to `801DFC70`.
  Slots 1 and 2 (`801DF8C4`, `801DF938`) are read as stored; they are the
  same for all three camera nodes in the saved frames.
- `name_tag_project.c`: `800DB0E4` (project the visual actor's position
  through slot 4, clip test, divide, then slot 1; screen x/y, shown byte,
  depth) and `800E20E4` (the per-player step in the draw callback `8012888C`;
  the oracle runs the original with its final player draw `80164000`
  returning at once). Needs `r13-4EEC` set to the camera node being drawn
  (the field node `806A1CA8`), as `801DEF50` does while it draws; it is zero
  in saved RAM.

Evidence: the saved frame cross-check re-projects the shown tag (Thomas)
within 3 pixels (0 in context 15) of the stored position, using the field
camera's view computed natively (`nflstreet_camera_view`) and
`nflstreet_camera_slots`.

## Gate

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_player_markers.py
```

Private evidence: `W:\Daywater\research-temp\nflstreet-player-markers-20261014`.
Mutation test (not part of the gate, ~30 deliberate bugs, each must fail the check):
`python3 projects/nfl-street/reconstruction/native_assets/mutate_player_markers.py`.

## Frame order (live app)

1. Gameplay/movement update writes the actor positions (`+1A8`...).
2. `nflstreet_rings_update` (the original runs it after the player update step).
3. Field camera: `nflstreet_camera_view(hud, field_camera)` then
   `nflstreet_camera_slots` (replaces the slot-3 copy in `scene_venue`).
4. `nflstreet_scene_venue` (draws the rings).
5. Per player, before its palettes and draw: `nflstreet_actor_place`, then
   `nflstreet_actor_name_tag` with `r13-4EEC` = field camera node.
6. `nflstreet_scene_hud` (draws the tags).

## Not modeled

- Slots 1 and 2 construction (`801DF8C4`, `801DF938`) from node parameters.
- Null ring/actor references where the original would read low memory: the
  native routines return 0 instead (edits avoided them).
- `lfs/stfs` quieting of signalling NaNs (words are copied); NaN edits not tested.
- The tag text and font (drawn by `800DADC0`, already native) and whatever sets a tag's shown byte.
- `8012888C`'s temporary slot 2/6 changes for the player draw.
