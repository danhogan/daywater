# Actor channels: the gameplay actors' animation update

`actor_channels.c` reconstructs the per-update animation step the game runs
for every gameplay actor, `8003A24C` (called from `8002E24C` once per game
update with the elapsed updates as `f1`, 1.0 per 60 Hz update in the shipped
loop `80005284`). It replaces the host clip choice of `live_pose.c`: the
channels at gameplay `+4E4` (ten `0x7C`-byte records, count at `+4DC`)
advance and build the pose as the original decides. `anim_stream.c` serves
the streamed banks (`animdata` files) in place of ARAM.

## What the update does (verified)

For all actors (table `r13-0x7C30`, stride `0x14D4`), in three passes:

1. **`80178290` manager tick** (the four 100-byte clip-set records at `+9BC`):
   delay `+C`; weight blend `+50` through the curve table `r13-0x6714`
   (`80178E78` none, `80178E7C` linear, `80178F04` delay, `80178F28`
   rise/hold/fall); retirement `80177A64` (group callback mode 3, `801740BC`
   stops the group's channels) or finished groups (callback modes 2 and 1,
   `8017428C`); compaction `80177950`. The callbacks go through `801778C0`,
   which first lists the group's resources at `8032C34C` (`80175AF8`,
   `801758E0`, `80179650`, `80174860`, `801748A0`).
2. **`80173D60` channel step**: delay `+48`, weight curve `+30`, and the clip
   program `80174374` → `80177374`/`80176E28`/`80176DD0` (frame pair
   `+5C/+5E`, time `+54`, remaining `+58`, fraction `+60`, loop stack,
   commands `0x8000`–`0x8006`, `0xA000`, events `0xC000..` through
   `80173A24` to the event callback `r13-0x6734`).
3. **Per actor**: the visual's `+1C/+20` from `+304/+308`; the pose list
   `80176650` at `8032BF28` (clip weights `80178404`, frames `801749E8`,
   descriptors `80176610`); the blend `800D4CBC`/`8015E9C8`; **the channel
   roots `80176904`**; the root travel `80176B3C`; `800D4CE4`/`8015EC30`.

**The channel-root producer is `80176904`** (the inference in `live_pose.c`
is retired by it). After the blend wrote each list entry's roots (`+C..+14`),
a channel with two entries gets `8017650C(fraction, +10, first, second)`:
the shortest 24-bit turn per word (`8018879C`) when the three differences
sum to at most a quarter turn, otherwise a quaternion slerp (`801A3794`,
`801A383C` with `acosf 80215844` and `sinf 80216B94`, `801A3580` with
`80188D98`). A single entry copies its roots. Entries with flag 2 then move
the heading (`+14`, zeroed) into the heading travel (`+1C/+20` → actor
`+224`, flag `+220`). The saved channels the old inference missed are the
slerp cases and the flag-2 zeroed headings.

Float operations follow the module exactly (`gekko_float.h`: single results
rounded from double, `frC` rounded to 25 bits for `fmuls`/`fmadds`, fused
tie correction).

## The stream service (`anim_stream.c`)

Streamed banks (header flag `0x200`; entries 1–3 and 7–17 in the saved
matches) keep frame table words that are ARAM addresses or
`0xF0000000 | slot` of the stream cache (`80B40BE0`, `80B4D260`). The
service matches each such bank table entry to its file (first six bytes)
and derives the bank's ARAM base from the saved words (ARAM = base + file
frame offset; cached frames through their slot record). Two paths:

- **Cache path** (default, RAM-identical): `80179B60` hit (`80179830`,
  LRU `8017972C`) or eviction (`801798CC`/`80179900`, the evicted frame's
  word restored) and `801519A8`: the `0xC0`-byte ARAM read `8017BB78` is
  served from the files into `8030B8E0` (toggle `r13-0x5968`), then
  `8015E938` decodes into the slot.
- **Direct path** (`direct = 1`): frames are decoded from the file into the
  context's host memory (`0xA0000000..`); the cache, frame tables, read
  buffers and toggle are untouched (list entries' `+28` point to host).

Saved matches hold some stale cache words (a table word naming a slot that
now holds another frame, or a slot its frame's word no longer names); the
mapping ignores them as evidence and the frame check counts them.

## Original execution gate

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_actor_channels.py
```

Private evidence: `W:\Daywater\research-temp\nflstreet-actor-channels-20261014`
(logs, mutants, `verification.json`). Per saved context (14, 15, 16), run
twice:

- **frames**: every saved cache slot its frame word names equals the
  service's frame from the file, decoded by the native `8015E938` (276
  slots in context 14).
- **update**: 301 cases (case 0 saved as is, 300 edited), the native update
  then the original `8003A24C` from the same RAM; all RAM outside the oracle
  stack, the 14 per-actor `8015E9C8` frames, and every event and group
  callback call (arguments, order, the event's returned r3) are compared.
  In the original run `80039990`/`80039A18` return at entry (not
  reconstructed), the event callback and the `801778C0` group callbacks
  return at entry (the event with the r3 the native hook returns, so the
  `0x1000` operand skip is exercised), and `8017BB78` is served from the
  files. Edits: manager records (states, keys, flags, delays, curves 0–3
  with phase bytes, including the bytes past the fourth record), channels
  (state, delay, curve, key, bank including empty entries, flags, variant,
  roots, heading scale, travel fields, frame pair past the bank's frames,
  fraction, program state) with synthetic programs of frames, `0x8000`–
  `0x8006`, `0xA000`, unknown `0x8xxx/0x9xxx` and events with and without
  `0x1000`, the actor's `+C` flag `0x200`, `+304/+308`, and dt 1, 2, 0.5 or
  random. Cases where the original enters `rem_pio2f`'s large path
  (`80217548`, wild fractions from time edits) are counted: the native stops
  there.
- **direct**: the same kind of cases through the direct path actor by
  actor against the cache path: all list frames and all RAM except the
  cache state match.
- **soak** (plausibility, native only): 600 updates of all 14 actors from
  the saved state without providers.

Results (contexts 14/15/16): 276/266/276 saved slots equal the files'
frames (10 slots in context 15 that its tables no longer name are
skipped); 255/251/259 update cases match fully and 46/50/42 stop where the
original enters `80217548`; 820/862/935 event and 4958/5115/5154 group
callback calls match; the direct path matches the cache path in 263/259/254
cases (13,039/12,940/11,915 frames; 5 context-15 cases where the cache
serves a stale saved slot are skipped). The soak keeps 9/7/7 actors with
active channels and changing poses after 600 updates; the others' programs
end or their groups retire, and without the group callbacks nothing starts
their next clip.

Mutation test (`mutants.log`, `mutants-2.log`; scratch copies, context 14):
14 planted faults, all caught: the fraction, the `0x8003` loop count, the
linear-curve clamp, the list normalization, the slerp threshold, the LRU
tail relink, the eviction's frame word restore, the read-buffer toggle, the
travel rotation sign, the compaction's delay reset, the retire threshold
(after adding edits exactly at the 1e-7 threshold), the event operand
skip, the heading travel scale and the curve phase state.

## Live integration

At startup load every `animdata/*.bin` (by number), then
`nflstreet_anim_aram_map(&aram, span, ram, files, sizes, count)` (14 banks in
the saved matches) and fill an `nflstreet_actor_channels` once
(`span`, `memory = ram`, `aram = &aram`, `direct` 0 or 1, hooks NULL or the
host's). Each 60 Hz update, after the movement write-back, call
`nflstreet_actor_channels_update(&channels, 1.0f)` instead of
`nflstreet_live_pose_step`, then for each of the 14 gameplay actors
`nflstreet_actor_pose_twist(&channels.pose[i], actor)` (the `80053404` part
of `80021A60`; its `8005345C` target step is not modeled) and
`nflstreet_actor_skeleton_frame(palettes, visual)`. The per-actor pieces
(`_manager`, `_step`, `_pose`) let a host keep `live_pose.c` for the
controlled player and run the others natively (same pass order). Until clip
requests (`8017413C` and the group callbacks) are reconstructed, the
controlled player does not respond to the stick through this path.

## Not modeled

- `80039990` (visual flag `0x10` sub-animation at `+C14`) and `80039A18`
  (the two attachment blocks at `+FA4`/`+FBC`/`+11AC`).
- The event callback `8001532C` (game effects of program events: actor
  flags, sounds, effects, its operand reads through `801777A0`); the hook may
  read operands with `nflstreet_actor_channels_operand`. Without a hook the
  event returns 1 and consumes no operands.
- The group callbacks (`r13-0x6718` tables, e.g. `80068E8C`): they start
  transitions and new groups. Clip starts requested by the movement state
  (`8017413C`, `80173FF4`, `80177EF4`, `80178780`, called from the actor
  state handlers `80244D28` table via `800150E8`, e.g. `8006475C`,
  `80066490`) are not reconstructed here.
- `rem_pio2f`'s large-argument path `80217548` (|x| > 201; the native
  returns 0). Streamed banks with flag 4 (none shipped). The ARQ completion
  counter `r13-0x5788` that `8017BB78` waits on. ARAM bytes outside the
  mapped files read as 0. The pose list's 24-entry cap is unreachable with
  ten channels.
- The twist targets (`8005345C`) and the skeleton (`800D51B4`) run later in
  the frame (`80039F84` from `8002E198`), as before.
