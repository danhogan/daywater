# Per-frame light update (`801ED3A0`)

`light_rig.c` reconstructs the routine that, each frame, turns the active
light rig's world-space lights into the view-space GX light objects of the
light sets at `8034FE9C`. The player draw and the ball load those sets
through `801EDC60` (`nflstreet_draw_light_set`). With this update the live
app no longer needs to re-express the saved frame's lights for a moved
camera.

## Where it runs in the frame (verified against the original)

- `801ED408(rig)` (from `8015B928` at match entry, `8015BA30` in the front
  end) stores the venue's rig in `8034FD88` and runs the update once.
- `80004870` (match state entry) calls `801ED47C(r13-5CE4)`. That registers
  `801ED3A0` through `801DF39C` as the **phase-1 callback of the field
  camera's view node**. `r13-5CE4` is `806A1CA8`, which is `hud_cameras[0]`. The
  saved RAM holds the record `{0, 1, 801ED3A0, 806A1CA8}` at `806A1F70`,
  next to the phase-2 callback `80161D84`.
- `801DEF50(view)` processes each camera node in this order:
  1. phase-0 callbacks;
  2. the handler `801DF458`, which calls `8017D16C`, the field view on the
     viewport's stack;
  3. the viewport setup `801DFAB0`, which copies the top into slot 3 and
     leaves the top equal to slot 3;
  4. **phase-1 callbacks, where the light update runs**;
  5. the scene draw `801DF4BC`, which draws the venue, players and ball;
  6. phase-2 callbacks.

  The update therefore runs after the field camera view is computed and
  before the venue is drawn.
- `80180260` returns `r13-4F44`, which no code writes (zero in the saved
  RAM). It is the unused second argument of `801EC8E4`, not a light input.
  The scenery draw only selects lights by mask. It does not read
  `8034FE9C`.

## What it does

`801ED3A0`: if `8034FD88` holds a rig, it selects the viewport's stack
(`801DF594` and `801893C8`), then:

- **`801ED168`, the lights (0x30 each).**
  - The callback at `+20` runs first. `8015B858` and `8015B8A4` copy the
    sun direction `802A3030` (`80027764`). `8015B8C4` stores the negated
    sun x and y with z set to `80260590` (0), then normalizes.
  - The world direction at `+04` is normalized in place (`801DE118`).
  - The view direction at `+24` depends on the type. Types 2 and 4 rotate
    the world direction by the stack top (`801DEB54`, rotation only). Type 3
    copies the world direction, so the light is fixed to the camera.
- **`801ED324` and `801ED998`, the light sets (0x108 each).**
  - The count and the ambient bytes are cleared.
  - Each of five entries `{light, scale[4]}` computes a color: the light's
    color times the entry's scale times the rig's scale (`+94`), each
    channel converted to an unsigned integer and masked to a byte.
  - Type 1 sets the ambient color.
  - Types 2 and 3 add a light object, up to four. Its position is the view
    direction × 10000 and its direction is the view direction. Spot and
    distance attenuation are off.
  - Type 4 adds a specular light: `GXInitSpecularDir` of the negated view
    direction (Gekko `frsqrte` estimate and three Newton steps), with
    attenuation (0, 0, 1, 25, 0, −24).
- **`801ED230`, the shadows (record 0x20, projection 0x40 at `8034FD8C`,
  color at `8034FE8C`).**
  - Type 1 (`801ECEA4`) shears x/y by the light's world direction (−x/z,
    −y/z), capped at length `r13-6228`.
  - Type 2 (`801ECF80`) uses the cosine and sine of an angle, times a
    length.
  - The color (`801ED024`) is each channel × 128, with the words OR'ed
    together unmasked.
- **`801EDCCC`** clears `r13-6220`, the render state's last light set.

The GX SDK light initializers it calls (`80204DB4`–`80205158`) only write
memory. It issues no GX commands.

## API

```c
nflstreet_light_rig_context c = light_rig_context(ram, x); /* light_rig_fixture.h */
nflstreet_light_rig_update(&c, s);
```

The context holds the viewport's stacks (`venue_stacks`) and the addresses
`8034FD88`, `8034FD8C`, `8034FE8C`, `8034FE9C`, `r13-6228` and `802A3030`.
The function clears `s->light_set`.

## Integration into the scene and the live app

- In `nflstreet_scene_venue` (`scene_frame.c`), call the update right after
  the field view is copied into slot 3 and before `nflstreet_sky_draw`.
  This is the original phase-1 position.
- The scene needs a `nflstreet_light_rig_context *lights` field, filled in
  `scene_fixture.h` with `light_rig_context(ram, x)`.
- The live app can then drop `view_delta`, `drawn_field_view` and
  `nflstreet_gx_aurora_set_light_delta`. `gx_aurora.c` loads the light
  objects as given.

## Verification

```
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_light_rig.py
```

The checks run in contexts 14, 15 and 16, twice each. Private evidence is
in `W:/Daywater/research-temp/nflstreet-light-rig-20261014`.

- **401 update cases per context.** Case 0 is the saved frame. The edited
  cases change:
  - the view;
  - the stack index and a stale stack selection;
  - the rig (any of the nine venue rigs, or none);
  - light types 0–5, directions, colors and callbacks;
  - the sun direction;
  - set counts, entries and scales, including colors above 255, negative
    and at or above 2^31;
  - the rig scale;
  - shadow counts, types, lights, angles, scales and colors;
  - the shadow limit.

  Native and the original `801ED3A0` must leave all RAM outside the oracle
  stack equal. Every GX entry other than the light initializers fails a
  case.
- **201 frame-order cases per context** with edited camera positions and
  angles. The original runs `801DF458`, `801DFAB0` (its GX calls return
  without effect) and then `801ED3A0`. The native side runs
  `nflstreet_camera_view`, copies the top into slot 3 and runs the update.
  The checks compare the rig's lights, `8034FD8C`–`803504CC`, slot 3 and
  the stack top. In context 15, the saved camera is the camera the frame
  was drawn with (`ball_and_marks.md`). There, the native field view plus
  the update reproduces the saved light sets byte for byte. In contexts 14
  and 16 the camera has moved since the frame was drawn, so the light sets
  differ slightly from the saved ones, as expected.
- **Mutation testing.** 15 deliberate faults were each caught in all three
  contexts:
  - no Newton steps;
  - type 3 rotated;
  - color byte order;
  - a fifth light object;
  - the shadow cap;
  - shadow color channels masked to bytes;
  - the shadow color scale;
  - the single normalize in `8015B8C4`;
  - the 2^31 conversion branch;
  - the ambient bytes not cleared;
  - the specular attenuation;
  - `r13-6220` kept;
  - no stack select;
  - sine and cosine swapped;
  - the far position.

  Two faults were not caught, and both are expected:
  - One Newton step instead of two, before the final step. After two steps
    the estimate is already accurate to about 2^-47, so this fault is
    practically equivalent to the original.
  - An extra mask on the shadow color's shifted words. The mask was
    redundant, and it has been removed from the code.

## Not modeled

- Light callbacks other than `8015B858`, `8015B8A4` and `8015B8C4` are
  rejected before any write. These are the only callbacks in the nine
  venue rigs.
- The checks avoid NaN inputs, so host NaN payloads may differ.
- The rest of `801DFAB0` is not reconstructed: projection, viewport,
  scissor, and slots 1, 2 and 4–6. The frame-order check only shows that
  it leaves the field view on top.
- This is evidence about the order within the frame, not a frame
  scheduler. The native scene must call the update itself (see
  Integration).
