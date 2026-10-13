# HUD: text, name tags, screen camera and in-match screens

`hud_draw.c` reconstructs the in-match text renderer and the player name
tags. `uis_draw.c` reconstructs the in-match uis screens: the scoreboard,
GameBreaker bars, "1ST DOWN" and the turbo meters. `matrix_stack.c`
reconstructs the part of the matrix library they and the HUD camera need,
and `gx_texobj.c` reconstructs the game SDK's texture-object encoders. Like the scenery draw (`scenery_draw.md`), the code
produces the exact GX calls and GX pipe data of the original, along with its
RAM side effects.

## How the HUD was located

A probe ran the original frame's scene traversal (`80180A14` with
`801DEF50`) and logged every function entry and GX call after the last
player. The frame has three camera nodes:

| Node | Data position | Draws |
| --- | --- | --- |
| `806A1CA8` | — | the field and players |
| `806A1DB0` | (427, 240, −579.4) | the HUD: the 14 name tags, then the uis screens (scoreboard, GameBreaker and turbo meters) |
| `806A1BA0` | (320, 240, −579.4) | a further overlay |

Each camera's handler builds the viewport's matrix stack (`801DF458` →
`8017D16C`). The saved RAM therefore holds the last camera's view, not the
HUD camera's. In the saved match only "Thomas" (`8091C6AC`) has its name tag
shown. It sits at screen position (253.8, 352.1), under the selected
player's ring.

## Source trace

- **Text layout (`801863A4`).**
  - It runs only when the `r13-6650` byte is set; otherwise it sets error
    0x110002.
  - Text longer than 0x162 bytes gives nothing at all, not even an error
    code.
  - Glyphs: `80185A98` looks up the code by index (code − 32), then by
    binary search.
  - Kerning: `80185B18` searches the pair table linearly. Flag 4 turns it
    off.
  - Each glyph box is `{x + offset, y + offset, width, height, u16 u,
    u16 v}`. Boxes go to `8039C4BC`, 0x14 bytes apart.
  - Lines break at a newline. When the limit is positive they also break
    once the pen (as a float) passes the limit. A NaN limit would never
    leave the loop, so it is rejected natively.
  - Each line is centered by `x/2`, rounded toward zero (flag 1), or
    right-aligned (flag 2), and advances by the font's byte at `+13`.
  - The count is stored, then error 0.
- **Text draw (`80186B1C`).**
  - The viewport's stack is selected (`r13-4EE8`), pushed and translated to
    the position, then loaded as matrix 0.
  - **C4 fonts** (header flag 4, as in all three saved fonts) set state
    flag 0x40 and draw twice with the font texture: first palette
    `80331460` in the second color, then palette `80331480` in the main
    color. This gives the dark outline and the white text.
  - **Other fonts** draw a shadow pass first when state flag 0x10 is set
    (second color, offset `8039C4A4`), then the main pass.
  - **Glyph quads** (`80186924`/`8018675C`): each is a GXBegin(quads, 0,
    4), and each vertex is written to the pipe as:
    - position (x, y, 0);
    - the color bytes (× 255, truncated);
    - texture coordinates over the atlas size.
- **Name tag (`800DADC0`).**
  - **Gates:**
    - the `r13-7240` byte;
    - option word 0x12 (`8014E29C`, from `8030B2AC + 0x120`; the
      `8010B60C` re-read with `r13-6334` set and `r13-6FF0 == 8` is
      rejected);
    - the shown byte `+1C` and font `+14`;
    - the play-state hold (`8000C2D8`);
    - mode ≠ 4 and ≠ 7, read twice through `8002E4F8`, which stores
      `r13-7CF4` each time;
    - the count `80308560` not positive.
  - **Then:**
    - profile bucket 13 (`80187CB4`/`80187CC4`);
    - fog off;
    - the text defaults (`80186264`: white, a half-alpha black shadow,
      offset (3, 3, 1)), with black text when `r13-723F` is set;
    - the GX setup `80186CD8`: direct position, color and texture
      coordinates; vertex color with texture alpha; source-alpha blending;
      no depth; alpha > 0.
  - **Matrices:** the projection slot 1 copy gets the tag depth `+20` and
    is restored, so there is no net change. The stack is translated by
    `80252B68` (106.67), then the text is drawn centered at `+04`.
  - Finally fog is restored.
- **Screen camera (`8017D16C` → `8017D604`).**
  - The handler is chosen through `r13-5780` by the camera data's type
    byte. Only the screen type (`8017DB70`, with no pre-handler) is
    modeled.
  - Steps: select the stack, reset to depth 0 and identity (`801893EC`),
    rotate about x, y and z by the 24-bit angles `+14/+18/+1C`
    (0x800000 = π), then translate by minus the position `+04/+08/+0C`.
- **Matrix library.**
  - **Push and pop:** push (`8018941C`) adds 1 to the depth and copies the
    slot below; pop (`8018949C`) subtracts 1.
  - **Translate (`80189BB0`):** `m3 + (z·m2 + (x·m0 + y·m1))`, computed
    with fused multiply-adds.
  - **Rotations (`80189854`/`944`/`A34`):** each rotation mixes two
    columns per row as `fmadds(m_i, c, m_j·a)` and `fmadds(m_i, b,
    m_j·c)`.
  - **Sine (`80188A70`/`80188AEC`):** the sign-extended 24-bit angle ×
    2⁻²³ gives a turn fraction t. The result is `sin(πt)` from the
    paired-single polynomial at `8039E100`: eight coefficients in two
    lanes, `ps_madd` steps and a final `ps_sum0`. Cosine adds 0x400000 to
    the angle. The recompiler computes `ps_madd` on single operands as a
    correctly rounded single-precision fused multiply-add
    (`DolRecomp/src/backend/llvm/paired_fma.cpp`), which is what the native
    code uses.

## In-match screens

- **`8012AB5C(camera, ticks)`.** It sets:
  - text and draw modes 0;
  - profile bucket 13;
  - pending loads (`8030834C`, none here);
  - depth test off (`80187980`), then the text defaults;
  - the refresh check (`80004668` → `801D5F60`, inactive here);
  - fog off.

  When the camera is `r13-6CD8`, it draws screens `r13-6C84`, `-6C94`,
  `-6C88` and `-6C8C`, enabled by `r13-6C98` bits 8, 1, 2 and 4.
  `r13-4F80` is cleared after the second. Then fog and depth are restored.
- **`801D0BC4(screen, 0)`.** The tint is reset (`8034CDB8`: scale 1, add
  0). Each root (`+3C`, 0x14 apart) is walked by `801CEFDC` with the draw
  message `0xFFFFFFFE`. The ticks update `801CF610`, which the frame runs
  once, is not modeled: the native draw shows the saved state.
- **`801CEFDC`.**
  - The root's pre-entries are messaged first.
  - A visible node (`*node != 0`) multiplies and adds the tint.
  - The screen's transform callback (`801D8398` → `801D8268`) does
    select, push, translate, then the pivot. Rotations (degrees ×
    `80263B80`, truncated) are applied only when one is nonzero. Then come
    scale and minus the pivot.
  - The color stack is pushed (`801D7E74`: multiplied entries, alpha
    clamped to [0, 1], eight bytes clamped from × 255).
  - Each enabled group's entries are walked: subnodes (class `0xFFFF`), or
    objects whose draw flag is set, through the class table `+4C`.
  - Then the tint is restored and the transform and colors popped.
- **Classes.** On the draw message:
  - images (`801D60DC`) draw through `801D6690`;
  - text objects (`801D9244`) draw through `801D97D4`;
  - the game classes `8012CB94` and `8012C5D8` do nothing.
- **Images (`801D6690`, `801D6A54`).**
  - **Resource lookups** (manager `r13-63DC`, archives 0x24 apart, entries
    8 apart):
    - A pending image change with the same ids is cleared after an
      archive state query (`801A8464`).
    - The record is `801D5C34`. An in-use entry notifies only archives with
      flag 4, which does nothing.
  - **GX state:** the GX reset, then `801D7AD8`:
    - s16 positions with 4 fraction bits, RGBA8 colors, s16 texture
      coordinates with 8 fraction bits;
    - source-alpha blending, no depth, alpha > 0.
  - **Colors:** the color-stack bytes become the material color and the
    color of light 7 (`801D7A58`). Lighting uses the vertex color as
    ambient.
  - **Cull and matrix:** cull back for flag 4, and the stack top as
    matrix 0.
  - **Textured images:**
    - the texture matrix 30 from the record's `+30/+34`;
    - `GXInitTexObj`, or `GXInitTexObjCI` with TLUT 15 (wrap where a
      coordinate passes 0x100);
    - `GXInitTexObjLOD`;
    - `GXInitTlutObj` (16 or 256 entries).
  - **Vertices:** four of them (`801D73E0`/`801D6C00`), with position
    `+34`, color `+14` (palette-mapped through the cached `801D7C1C`
    entry), and texture coordinates `+24`.
- **Text (`801D97D4`).**
  - **Font:** from the archive `+3`/`+2` record.
  - **Text state flags:** set from the object's high nibble.
  - **Colors:** main and shadow colors are `(byte + add) × 1/255 ×
    scale` of the current color-stack entry, with the shadow offset
    `+C..+E` for flags 0x50. The text setup runs once per mode
    (`r13-63C4`).
  - **Position:** `+6/+8/+A`, shifted by `+14` (halved when centered) for
    flag 0x100.
  - **Lines:** `+10` to `+F`, each temporarily terminated in its buffer and
    drawn through `80186B1C`. With flag 0x200 they are clipped at
    `+16` + y.
- **SDK encoders.** `GXInitTexObj` without mipmaps, with tile classes and
  shifts from the format table at `80298A7C`. `GXInitTexObjLOD` clamps the
  bias to [−4, 3.99] (× 32) and the LODs to [0, 10] (× 16).
  `GXInitTlutObj` packs the format, address and entry count.

The screens re-initialize the shared texture object `8039E740` and TLUT
object `8039E780` for every image, as GX copies them when they load. The
scene render therefore executes HUD calls as they are issued, not after
each screen.

## Verification

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus \
  python3 projects/nfl-street/reconstruction/native_assets/verify_hud_draw.py
```

Every original routine executes unhooked in the module. Only GX SDK entry
points are recorded (`gx_oracle.h`), and immediate vertex data is captured
at the GX pipe. Per saved context (14, 15 and 16, each in two fresh
processes) there are 1,202 original runs, with 1,747,351 calls and pipe
writes in context 14:

- **The 14 name tags as saved:** 472 calls, 12 glyph quads ("Thomas", two
  palette passes).
- **200 edited name-tag frames:**
  - tag visibility, position, depth, text and font (including none);
  - the tag, black-text and option switches, the option mode;
  - the play-state hold, mode words 4 and 7, and the count;
  - fog;
  - the text system flag;
  - stack choice, depth and top;
  - profile buckets;
  - the text state.
- **400 direct `80186B1C` texts:**
  - all three fonts, with the palette flag toggled and kerning removed;
  - edited glyph advances and offsets (negative line widths);
  - generated text with newlines, control and high codes, and lengths
    around 0x162;
  - all alignment flags;
  - zero, negative, fractional and whole-number wrap limits;
  - positions, the text state and shadow offset, and the last error code.
- **400 screen camera views:** the two saved screen cameras, then random
  and boundary angles (0, ±90°, 180°, 0x7FFFFF, 0xFFFFFF), positions, and
  the stack choice and depth.
- **201 `8012AB5C` screen frames:** the saved screens (8,090 calls, 36
  image quads and 148 glyph quads), then 200 edited frames. The edits
  cover:
  - node visibility (hidden nodes shown too), transforms, rotations, scales
    and tints;
  - group enables and entry draw flags;
  - image flags, colors, positions, texture coordinates, pending ids and
    the cached palette;
  - text flags, positions, offsets, colors, shadows and start lines;
  - multi-line copies of text objects with wrap clipping at line-height
    multiples;
  - the screen flags and camera, the text mode, pending loads, the
    resource flag and state bits;
  - the base color-stack entry, the LOD constant, the stack choice, fog and
    the error code.

  The original runs unhooked except for the resource lock `801AF944`/
  `801AF9CC` (an OS mutex around archive state queries), which is a
  supplied no-op on both sides. The memory-only SDK encoders execute in the
  oracle (`gx_oracle.h`).

Thirty-seven deliberate faults were each rejected. Eighteen are in the text,
name tags and matrix library:

| Area | Faults |
| --- | --- |
| Text layout | kerning switch, center rounding (`x>>1`), wrap boundary (`<` for `<=`), layout error code, the 0x162 length limit, line height |
| Text drawing | color rounding, quad corner order, palette flag |
| Name tag | profile stamp, the mode-4 gate, black-text alpha, font gate |
| Matrix library | sine without fused adds, rotation sign, cosine phase, translation without fused adds, rotation order |

Nineteen are in the screens:

| Area | Faults |
| --- | --- |
| Color stack | alpha clamp, byte clamp, multiplied add |
| Transforms and matrices | angle rounding, pivot, scale axis |
| Images | wrap threshold, TLUT entries, palette gate, cull mode, pending clear |
| Text | color order, center shift, clip boundary |
| Screen walk and entry | entry draw flag, the `r13-4F80` clear |
| Texture-object encoders | LOD bias clamp, tile class, bias-clamp bit |

Four first-try screen faults were equivalent:
- `<=` for `<` at the clamps, which store the same value either way, and the
  NaN-only clip, since positions are integers. All three were replaced by
  stronger forms.
- The tint add was dropped, since the tint is restored before anything
  reads it.

Three of the first eighteen needed stronger cases before they were caught:
- the wrap boundary needed whole-number limits a pen position can equal;
- the error code needed a nonzero prior value;
- center rounding needed negative line widths.

Private report:
`W:\Daywater\research-temp\nflstreet-hud-draw-20261013\verification.json`.

## Use

`player_render_check.c` draws the HUD last. It first rebuilds the HUD
camera's screen view (`806A1DB0`) natively, then draws the name tags and
the in-match screens (`8012AB5C`). The rendered frame then shows the
Bears and Lions logos, scores, GameBreaker bars, "1ST DOWN", both turbo
meters and the "Thomas" tag, as in the game's screenshot.

## Not modeled

- **The screens' update tick** (`801CF610`, once per frame) and the uis
  script VM. The native draw shows the saved state.
- **Rejected image and text paths:**
  - resource loads and acquisitions (`801A6B78`, `801D54A4`, palette cache
    misses);
  - image exchanges (`801D84D8`);
  - `801DD5E8` refreshes;
  - nine-slice strips (flag 8);
  - display-list (flag 0x8000) and indirect (`+10 == 6`) images;
  - dirty fonts (`801861C4`) and layouts (`801D8E54`);
  - root callbacks and other classes.
- **Other camera types** (the field camera is type 2), and the camera
  node's projection and viewport setup (`801DFAB0`).
