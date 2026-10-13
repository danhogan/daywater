# HUD text, name tags and the screen camera

`hud_draw.c` reconstructs the in-match text renderer and the player name
tags. `matrix_stack.c` reconstructs the part of the matrix library they and
the HUD camera need. Like the scenery draw (`scenery_draw.md`), the code
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

## Verification

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus \
  python3 projects/nfl-street/reconstruction/native_assets/verify_hud_draw.py
```

Every original routine executes unhooked in the module. Only GX SDK entry
points are recorded (`gx_oracle.h`), and immediate vertex data is captured
at the GX pipe. Per saved context (14, 15 and 16, each in two fresh
processes) there are 1,001 original runs, with 931,763 calls and pipe writes
in context 14:

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

Eighteen deliberate faults were each rejected:

| Area | Faults |
| --- | --- |
| Text layout | kerning switch, center rounding (`x>>1`), wrap boundary (`<` for `<=`), layout error code, the 0x162 length limit, line height |
| Text drawing | color rounding, quad corner order, palette flag |
| Name tag | profile stamp, the mode-4 gate, black-text alpha, font gate |
| Matrix library | sine without fused adds, rotation sign, cosine phase, translation without fused adds, rotation order |

Three of these needed stronger cases before they were caught:
- the wrap boundary needed whole-number limits a pen position can equal;
- the error code needed a nonzero prior value;
- center rounding needed negative line widths.

Private report:
`W:\Daywater\research-temp\nflstreet-hud-draw-20261013\verification.json`.

## Use

`player_render_check.c` draws the name tags last. Before them it rebuilds
the HUD camera's screen view (`806A1DB0`) natively. The tag then lands on
the selected player's ring, as in the game's screenshot.

## Not modeled

- **The uis screens** drawn after the name tags in the HUD camera:
  scoreboard, GameBreaker meters, "1ST DOWN", turbo bars. They go through
  `8012AB5C` → `8012C528` → `801D7DAC`/`801D7DCC`, text through
  `801D9D84` → `80186B1C`, and quads through `801D6690` → `801D6A54`.
- **Other camera types** (the field camera is type 2), and the camera
  node's projection and viewport setup (`801DFAB0`).
- **The rest of the matrix library:** scale, concatenation and the
  projection products.
