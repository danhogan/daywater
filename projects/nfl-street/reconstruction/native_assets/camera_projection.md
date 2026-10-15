# Camera screen mapping and perspective slots

## Verified scope

`camera_projection.c` reconstructs the two writes made before the camera-slot
tail in `801DFAB0`: `801DF8C4` writes slot 1 at `80331F00`, and
`801DF938`/`801FFF64` writes slot 2 at `80331F40`. The screen mapping reads
camera-node offsets `+04`, `+08`, `+14`, `+18`; the projection reads
`+D8`, `+DC`, `+E4`, `+E0`, `+E8`, `+EC`. Flag 4 selects the `801DF970`
asymmetric projection path; otherwise the builder uses `801DF938`/`801FFF64`.
Float operations follow the observed single-precision instruction sequence.
No host trigonometry is used.

Evidence is `probable` until the native/original gate is run on Windows. The
check executes each original builder directly on the same prepared RAM and
compares all RAM. It covers the saved case and 400 randomized node edits for
each of three nodes in each saved context.

## Gate

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_camera_projection.py
```

Private output: `W:\Daywater\research-temp\nflstreet-camera-projection-20261014`.

Pass any third argument to `camera-projection-original-check.exe` to print up
to four differing words from each of slots 1 and 2 when the RAM comparison
fails.

## API and integration

Call `nflstreet_camera_projection(&view, camera_node, viewport_scale)` after
the viewport stack is selected and before `nflstreet_camera_slots`. It writes
slots 1 and 2 at `view.slots + 0x40` and `view.slots + 0x80`. `viewport_scale`
is the fifth argument to `801DF8C4`; in this build, `801878DC` returns zero,
so `801DFAB0` passes `0.0f`.

## Not modeled

The remaining `801DFAB0` work that updates the viewport stack and loads
camera view parameters is outside this module. This routine only rebuilds
slots 1 and 2.

## Suggested one-line mutants

Apply one at a time in a scratch copy; each should cause the original check to
fail:

- In `screen_mapping`: `const float sx=width*k,sy=height*k,sz=depth*k;` → `const float sx=width*k*2.0f,sy=height*k,sz=depth*k;`
- In the same declaration: `...sy=height*k...` → `...sy=height*k*2.0f...`
- In the screen matrix initializer: `zero,zero,-sz,sz` → `zero,zero,-sz,zero`
- In `perspective`: `const float dx=right-left,dy=top-bottom,dz=far_z-near_z;` → `const float dx=left-right,dy=top-bottom,dz=far_z-near_z;`
- In `perspective`: `const float near_term=c8*near_z;` → `const float near_term=c8+near_z;`
- In `asymmetric`: `m[11]=-(nf/dn);` → `m[11]=nf/dn;`
