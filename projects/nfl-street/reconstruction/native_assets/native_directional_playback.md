# Original directional locomotion playback

## What changed

The experimental native Windows player now consumes the two carried heading
words when selecting clip pairs and calculating their blend/rate. It joins the
original curve choice, phase transfer and four-channel fades already present.
The default chosen idle/run viewer and NLC1 fixed-primary inspection path remain
unchanged. The paired-player launcher uses this NLC2 path after passing checks.

The original has **two different sign thresholds**. `0x80062748` negates a
nonzero speed when the shortest 24-bit heading separation is greater than
`0x3C71C7`. The tail of `0x800635B8`, starting at `0x80063DAC`, negates the
blend speed when separation is greater than `0x56C16C` (equivalently, at least
`0x56C16D`). A single shared signed speed would miss the interval between them.
Curve choice still consumes the raw speed, before either directional sign.

This makes the native animation decision follow original direction handling.
It does not establish complete backward-running gameplay, body orientation,
root travel or original wall-clock timing. The viewer still uses its existing
measured orientation adapter and host time mapping. Upstream variants remain
fixed; naturally occurring curve changes and complete callback eligibility are
not reconstructed by this milestone.

## Original evidence and confidence

Game: NFL Street, GameCube GNNE69 revision 0. ISO SHA1
`813c4511ba3135059a13de424555647e8252254b`; DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Reference snapshots are
position-neutral14/15/16 from the Dolphin 2603a saved-match captures.
Checks execute the untouched original module offline, not a new live Dolphin run.

| Source data | Primitive / size / endian | Verified numerical role |
| --- | --- | --- |
| Actor+1C0 and +1C8 | uint32 /4 each /BE | Arguments of the shortest 24-bit separation helper 80188F28; movement state 6/8 |
| Actor+1C4 | float32 /4 /BE | Raw speed for 62748; carried movement state 7 |
| DOL 8024F9CC | float32 /4 /BE | Exact zero test; all snapshots contain 00000000 |
| Descriptor+2C | float32 /4 /BE | Raw speed carried by 635B8 before blend sign selection |
| Descriptor+30 | float32 /4 /BE | Returned blend/rate written by 627B8 |
| Descriptor+28/+29 | uint8 /1 each | Optional stream slots rescaled by 627B8; bounded native controller has neither, uses FF |

Confidence is **verified within these source functions and the controlled
callback tail**. Broader gameplay meanings of the two heading fields are not
newly assigned. The captured adapter stores 21 movement words from actor+1A8;
this establishes the state 6/8/7 mapping. The reference actor is 0x803C19C0 in
these saved snapshots; it is not a universal runtime address.

Original 80188F28 computes `(first-second)&0xFFFFFF`, then takes the shorter
of that value and `0x1000000-value`. The native helper preserves unsigned
wrap and high-bit masking. Pair selection retains the sign of zero; the blend
branch can negate zero. Nonfinite host speeds are rejected transactionally.

## Native join

`nflstreet_locomotion_direction` produces separate pair and blend speeds.
`nflstreet_live_locomotion_step_direction` connects these to the existing
controller; the viewer supplies carried state 6/8. The curve decision continues
to read the raw input. During a fade the selected pair remains held, while
weights/rate follow the signed blend input. Raw speed and resulting rate are
stored in the original descriptor slots. The optional extra streams are absent
in this four-slot controller; full callback scheduling remains unfinished.

Runtime needs extracted private assets and small captured profiles. It loads
no RAM snapshot, original DLL, Dolphin, ModernGekko or ISO while playing.
Fresh native actor/match initialization, other players, contacts, ball gameplay,
menus and audio remain unfinished.

## Verification and reproduction

Build with `build_windows_locomotion_selection.sh` and
`build_windows_live_player.sh`. `directional-playback-original-check.exe`
accepts the original DLL, one MEM1 snapshot, bank 00111 and an NLC2 curve profile.
All original data, builds, images and logs stay private in
`W:\Daywater\research-temp\nflstreet-directional-playback-20261009`.

- Three contexts give 960 exact shortest-separation/pair/callback-tail boundary
  cases: both threshold neighbors, wrap, high bits, either ordering, signed zero
  and positive/negative speeds. Pair indices come from actual original 62748.
  Blend signs come from executing actual instructions at 63DAC until the 627B8
  call; this deliberately excludes earlier actor/game-service branches.
- Three joined sweeps give 1,440 updates, 84 starts, 603 fade updates, 78 completions,
  24 source-identity swaps and 174 gated pair requests. Original 627B8 executes
  after its actual sign branch. All source curve bytes, carried pair index,
  current identity, descriptor numeric data and channel+30..7B match exactly;
  other channel metadata remains outside the claim. Original numeric frame-list
  ordering and weights match 5,988 entries.
- Existing curve-choice and legacy fade regression oracles pass in all three
  contexts. The Windows viewer preserves eight complete recorded movement
  checkpoints and passes all eight direction/start/release/stop segments.
  The 528-update Windows direction run exercises 65 negative-pair and 29 negative-blend updates, completing 34 fades; these are adapter outputs, not a full original gameplay comparison.
  The default recorded run passes; five inspection BMPs are byte-identical to
  the preceding baseline. These are native diagnostic pixels, not original
  gameplay pixel equivalence.

The private manifest records source hashes and unchanged original baseline
hashes: DLL SHA1 `dd521cc38b0db6bca98a0bb261570a2be5e6e0ed`, runner SHA256
`3f9e3a3eb4b5728d9036e9552c01e8c9dc084a1f53805fef95dcf3b1b86f73ea`.
No original DLL/source changes or baseline restoration were required.
