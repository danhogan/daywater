# Actor animation: pose, root and compact skeleton

## The pose layout (+5C)

The visual actor's pose (+5C) holds signed 16-bit x/y/z angles per bone in
**compact SKL1 order** (the 29-bone `animdata/00095.bin` skeleton at +64).
The visual's +30 halfword (30) times three is the number of angles the
blend touches (90). Evidence (verified):

- `801EAE1C` reads bone *i*'s angles at +6*i* for *i* = 0..28.
- `8015E9C8` writes a streamed frame's 26 bones (78 components in stream
  order, which is compact order) straight into +5C. The mirrored form
  permutes bones through the table at `8028CB30` and negates y and z.
- `8015EC30` then sets bone 0 to (x, 0, z) from the channels and zeroes
  bones 26-28 (`headend`, `eye_knuckle`, `reye`; only 26 with flag 0x80).
  Bone 29 is never written by the animation.
- `80162760` indexes the same buffer by TMdl body bone. In RAM the body
  skeletons are stored with their first 29 bones in compact order (the
  translation rows match `00095.bin` for all 14 actors in all three saved
  contexts, except the ball bones 19/25), and their Clst rows reference
  bones 0..24 only. The 225 values it could index past +5C+180 are
  unrelated memory, never weighted.

The streamed banks (flag 0x200 in the bank header, all compressed banks in
the saved matches) reach `8015E9C8` already decoded: the ARAM callback
`801519A8` runs `8015E938`, which copies the frame header and decodes
the components non-scattered (`8015D104`) and dequantized (`801667CC`).
The list entries then carry no compression descriptor. The scattered
`8015D104` branch runs only for mirrored entries of in-RAM compressed
banks (none in the saved matches); it is reconstructed and verified on
the resident bank 7 descriptor.

## Verified scope

`actor_pose.c` (on saved RAM through a span):

- `8015E938` streamed-frame decode (plain and scattered).
- `8015E9C8` pose blend: per list entry `8015DBFC` / mirrored `8015D584`
  (header parsing `8015CFA0`, the 18-bit root offset form, the height
  threshold, ball vector and angles via `8015DB28`/`8015D49C`, defaults at
  `8028CBA8`), `8015D104`, `801667CC`, the blends `80166730`/`80166770`,
  `801DE858`, `8018879C`/`80188834`/`80188758`, and the ball bone +32.
  Its stack frame (the blend descriptor and the 384-byte decode buffer) is
  partly stale between entries: the native context carries it as an input.
- `8015EC30` channel root rotation and extra-bone zeroing (`80178440`).
- `80053404` torso and neck twist (`800531CC`).

`actor_skeleton.c`: `8015F08C` with `801EAE1C` (the verified
`compact_pose.c` kernel) through the locked cache and its DMA, and the
`800D51B4` prelude (`801893C8(1)`, `801893EC`).

## Original execution gate

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_actor_animation.py
```

Private evidence: `W:\Daywater\research-temp\nflstreet-actor-animation-20261014`.
Per saved context (14, 15, 16), each case runs the native routine for all
14 actors and then the original module from the same RAM, comparing all
RAM outside the oracle stack:

- 401 skeleton cases (5,614 runs of `8015F08C`, 4,018 after the stack 1
  select and reset): pose angles, flag 0x2000, +34/+4C, skeleton
  translations and bone count, moved destinations, the locked cache and
  its enable flag, edited viewport tops. The locked cache is compared.
- 401 blend cases (5,614 runs of `8015E9C8`, 15,618 entries): the saved
  list, then random lists of streamed records, compressed bank 7 frames
  (descriptor `808177E0`, resident) and synthetic records, with random
  flags (7,853 mirrored, 7,712 flag 4), modes 0-3, +1C, +30, root fields
  and stale stack frames. The stack frame is compared too.
- 201 root cases (2,814 runs of `8015EC30`), 201 stream cases (804 runs of
  `8015E938`), 201 twist cases (2,814 runs of `80053404`).

The original runs everything except the locked-cache enable, flush and
store (`801F0C84`, `801F0BA0`, `801F0BD0`: no RAM effect) and the DMA
`801F0DD4`, served from the oracle's locked cache. Scratch lists and
frames at `80F10000..80F37000` (zero in the saved contexts) are harness
allocations. 14 planted faults were caught: the 0x2000 flag, the DMA
length, the cache flag, the 4095 weight scale, the mirrored heading
sign, the mirrored ball blocks, the last entry's ball bone flags, the
dequantization count, the extra-bone limit (after adding random bones
past 26 to the root cases), the header length, the twist shift, a
non-stale flag-4 heading, an unfused height blend and the root y write.

## Live pose source (`live_pose.c`)

Per 60 Hz update it steps `live_locomotion.c` with the controlled
player's movement blocks (heading state[6], state[8], speed state[7]),
builds the pose list from its four channels with the verified 80176650
weight expansion, decodes each frame from `animdata/00111.bin` with the
native `8015E938`, copies +300/+304/+308 to the visual (as `80020FDC`),
and runs the native `8015E9C8`, `8015EC30` and `80053404`.

**Host choices (not reconstructed):** clip selection and timing
(`live_locomotion.c` and its NSC1 set catalog) instead of the gameplay
actor's channels and animation services; entry flags 0 and no clip-set
weighting; and the channel root rotations, whose producer is not
identified. They are inferred as the 24-bit shortest-turn interpolation
by the channel fraction between its two frames' entry roots (heading 0
under flag 4). The gate counts this against the saved active bank 7
channels: it reproduces 20 of 22 (context 14), 19 of 22 (15) and 18 of 20
(16) exactly, including the controlled player's. The others hold roots
that no frame pair of theirs explains.

`live_pose_demo.c` (`build_windows_live_pose_demo.sh`) walks the
controlled player forward for 150 updates and stops for 60, running per
update: movement, `nflstreet_live_pose_step`, `nflstreet_actor_place`,
`nflstreet_actor_skeleton_frame`, `nflstreet_actor_palettes`. The legs
alternate eight times (ankle stride -0.85..0.90 in model space), the right
knee spans 6,950..21,860, the ankle palettes change every update, and the
player settles into the stance when stopped. It writes `walk-<context>.csv`
beside the evidence. This is a plausibility check, not an original
comparison.

## Not modeled

- `80176650` on the gameplay actor's channels (its frame lookup goes
  through the ARAM stream cache `80179B60`), and the channel root
  producer.
- `800D51B4`'s +35C countdown (`801EE24C` random draw).
- Native rejections where the original would compute: non-finite
  skeleton matrices, invalid SKL1 parents, field widths over 16 bits or
  five-field groups over 64 bits (no shipped descriptor has these).
