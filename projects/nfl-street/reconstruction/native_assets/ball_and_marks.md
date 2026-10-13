# Ground marks, ball shadows, the ball and the field camera

`marks_draw.c`, `ball_draw.c` and the field-camera path in `hud_draw.c`
reconstruct the last pieces of the in-match 3D scene: the turf marks left by
players, the drop shadows of field objects and the ball, the ball model, and
the field camera that the scene draws under. Like the other draws they
produce the original GX calls, GX pipe data and RAM side effects.

## Source trace

- **Ground marks (`800D75EC`).**
  - **Gate:** the `r13-7280` byte.
  - **Lists and handlers:** three mark lists (pointers `802AA7C4`, counts
    `80271268`). Each list runs between its type's begin and end handlers
    (`8028CCB4`, 0x20 bytes per type: begin, end, draw).
  - **List 2 (type 2), 130 slots:** it starts at the oldest mark
    (`r13-5B60`) and wraps. Each slot's alpha `+34` is set from a ramp:
    0.1, rising by 0.1 every eight slots (`802529C8`), capped at 1
    (`802529CC`). Active marks (`+70`) draw through their type's handler.
  - **Type 2 begin (`80160104`):**
    - the GX reset;
    - fog on, with its previous state kept in the `r13-58F0` byte;
    - no cull, source-alpha blending;
    - depth LEQUAL without update, at depth range [0.01, 0.01];
    - direct F32 position and texture coordinates, white material, unlit.
  - **Type 2 mark (`80160298`):**
    - material alpha `+34` × 255;
    - the mark's texture (`+60` record `+28/+2C`);
    - push, translate `+00`, rotate about z by `+30`, scale `+0C..+14`,
      load as matrix 0, pop;
    - a four-vertex fan of ±0.16 by ±0.27 (`80260730..`).
  - **Type 2 end (`80160250`):** depth range [0, 1], LEQUAL with update,
    and fog restored.
  - **Lists 1 and 0 (types 0 and 1):** their begin (`8015FD28`: blend
    preset 2, early depth, alpha always, depth LEQUAL with update, one
    channel) and their end (a bare return) run even when nothing is active.
    Active type 0/1 marks (`8015FD88`) are not modeled and are rejected.
  - **Call site:** the field passes `8015B670` call `800D75EC` before layer 4,
    while the stack top is translated to the passes object.
- **Ball shadows (`801652BC` → `80164E78`).**
  - **Gate and setup:**
    - the `r13-58D8` byte;
    - profile bucket 2;
    - the GX reset;
    - direct position, RGBA8 color and texture coordinates;
    - source-alpha blending without depth;
    - the texture object `80310A58`.
  - **Type 0 entries:** they use the stack top × light 0's matrix
    (`801ED450(0)` = `8034FD8C`). Their direction comes from
    `801ED4D4(0)` on stack 0: (0, 0, 1) minus its rotation by light 0's
    matrix (`801DEB54`), normalized (`801DE094`/`80188EAC`: 1/√, 0 at
    or below 1e-7). It is then rotated 45° in xy (`801DE2AC`, a fused
    multiply-subtract) and normalized in xy (`801DE0CC`).
  - **Type 1 entries:** they use the top with z flattened (scale 1, 1, 0)
    and direction (0.5, 0.5).
  - **Each active entry** (`+10`) is a four-vertex fan at its position
    (`+04`) ± size × direction, with the color bytes in the order `+0C`,
    `+0E`, `+0D`, `+0F`.
  - **Afterwards:** the profile stamp is restored, then depth LEQUAL with
    update and material grey.
- **The ball (`8015CB60`).**
  - **Lighting:** light set 4 (`801EDC60`, shared with the player draw as
    `nflstreet_draw_light_set`), scaled by the ball's light color
    (`+294 → +0C..+14`, times `+298..+2A0` for flag 0x20). Material white
    with alpha `+28C` × 128, and channel controls on the set's mask.
  - **Frustum tests (`801EC298`):** the box `8028CA20` through stack slot 3
    (the view) × light 0's matrix × the ball matrix `+20`, then through the
    view × the ball matrix with projected bounds (`801CB1C4`). Flag 8
    clears itself and skips both tests.
  - **Level of detail (`801EB68C`):** the projected area picks the first
    used entry of `803897D8` whose threshold it exceeds, stepping back over
    entries marked 0xFFFF. Its `+1C` is the model view. Materials are at
    `+284`.
  - **Ball pass:** the stack top × `+20` as position and normal matrix,
    blend preset 2, every chunk.
  - **Shadow pass**, unless flag 2 or culled: the top × light 0's matrix ×
    `+20`, depth LESS with update at a fixed depth (0.99995), and chunk
    3 in black. An alpha of 1 gives material alpha 0x68 in preset 4 (the
    code compares 1 with 4, so it always takes this branch). Other alphas
    give alpha × 48 in preset 2.
- **Matrix library additions:**
  - `PSMTXConcat` (`801FFDF0`): paired-single lanes as single fused
    multiply-adds, with the constant pair (0, 1) at `803A3958` for the
    translation column;
  - matrix load (`8018962C`);
  - vector rotation (`801DEB54`);
  - reciprocal length (`80188EAC`).
- **Field camera.** Node `806A1CA8`'s data has type 2, whose handler
  `800C3508` dispatches on `+98`. Modes 0 (`8017DB70`), 1 (`8017D938`) and
  5 all reach `8017D604`, the view the screen cameras use. Other modes leave
  the stack unchanged. Its view at data `+04..+1C` reproduces the drawn
  frame's field view exactly in context 15. In contexts 14 and 16 the
  camera has already moved, by about 0.03 units.

## Verification

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus \
  python3 projects/nfl-street/reconstruction/native_assets/verify_scenery_draw.py
PYTHONPATH=projects/nfl-street/reconstruction/menus \
  python3 projects/nfl-street/reconstruction/native_assets/verify_hud_draw.py
```

Every routine runs unhooked in the original module, and calls, pipe writes,
render state and all RAM outside the oracle stack are compared. In each
saved context (14, 15 and 16, each in two fresh processes):

- **Ground marks (scenery gate):** the saved marks (78–80, 1,979 calls in
  context 14), then 200 edited frames. The edits cover:
  - activity, positions, scales and angles;
  - the start index;
  - the gate and fog bytes;
  - the stack choice and top.
- **Ball shadows (HUD gate):** the saved three shadows, then 200 edited
  frames. The edits cover:
  - entry types, sizes, colors, activity and positions;
  - the entry count;
  - light 0's matrix (including identity);
  - the gate;
  - stacks and profile buckets.
- **Ball (HUD gate):** the saved ball, then 200 edited frames. The edits
  cover:
  - flags 2, 0x20 and 8;
  - alpha, the light color and its scales;
  - the ball matrix;
  - light 0's matrix;
  - LOD thresholds, used flags and 0xFFFF markers;
  - the refresh hook.

  In three of four cases the view slot and stack top hold the drawn field
  view, so the ball passes its frustum tests: about 100 of 200 cases draw
  it. The saved ball itself is culled, because the saved view slot holds
  the last camera's (screen) view.
- **Field camera (HUD gate):** the camera views now include the field
  camera, with edited angles, positions and modes (0, 1, 5 and others).

Twenty deliberate faults were each rejected:

| Area | Faults |
| --- | --- |
| Marks | ramp period, ramp cap, corner order, mark color, start wrap, fog save, rotation axis |
| Ball shadows | shadow angle, color byte order, shadow corner, flattened direction, light direction |
| Ball | shadow alpha, LOD walk-back, the shadow frustum's light matrix, alpha scale, the alpha channel's light mask |
| Matrix concat | unit lanes, accumulation order |
| Field camera | mode 5 |

The LOD walk-back needed edits that mark entries 0xFFFF before it was
caught.

## Use

`player_render_check.c` now builds the scene in the original order:
- the field camera's native view (then `801DFAB0`'s copy into slot 3);
- the sky;
- the field passes, with the ground marks under the passes translation and
  the ball shadows after them (hooks at the original call points);
- actor shadows, lines, rings, the main field;
- the ball;
- the players;
- the HUD.

The previous workaround, which derived the view from the stored main-field
matrix, is gone.

## Not modeled

- Type 0 and 1 marks (`8015FD88`) and flag 8 with the area needed: the
  original would read an unset stack buffer.
- The camera node's projection and viewport setup (`801DFAB0`) beyond the
  slot-3 copy. The render uses the saved GX projection.
- Ten further models (`80168234`) and the crowd.
