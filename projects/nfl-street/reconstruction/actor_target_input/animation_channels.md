# Animation channel allocation and state updates

## Result — 2026-10-08

Windows C now predicts the observed channel and manager slot choices, starts
the selected clip, prepares a new animation group, and updates blend state.
Two fresh saved-match runs, using forward and right input, pass **26 complete
block comparisons**. Expected slot choices and output blocks are comparison
targets, never inputs to the native functions.

This is a bounded animation component. It does not yet replace the complete
contact animation service or run a match independently. Game-specific
callback metadata, transition matching, and bank resolution remain original
providers. The existing contact selector still derives the paired clip keys;
this component consumes upstream clip keys and resolved source programs.

The subsequent [callback and transition reconstruction](callback_metadata_transitions.md)
now replaces those tested callback/transition effects and joins both actors
in a native paired contact application. Source bank/asset resolution remains
a provider. The limits below describe this allocation component itself.

## Reference and confidence

All observations below refer to **NFL Street, GameCube GNNE69, revision 0**:

- ISO SHA1: `813c4511ba3135059a13de424555647e8252254b`.
- Original DOL SHA1: `46e3de854998a0f66c9be8c72a1164d22cd29eb4`.
- Dolphin build: **2603a**.
- Saved match SHA256: `e2faeb8f9481096ff87ae261c8c2943452fbc023cd27510f641808e307603d93`.
- All integers, pointers and floats in captured objects are **big-endian**.
  Floats are IEEE binary32. Pointers remain source identities, not host addresses.

`verified` applies to the observed allocation, initialization, preparation
and blend branches reproduced across both fresh emulator processes.
`probable` applies to channel fallback selection and manager compaction:
their native code follows the original instructions, but these paths were
not exercised by these live captures. Full manager eviction is unsupported.

## Observed layouts

Test procedure: restore the identified match, hold A until the event
constructor confirms play has begun, apply forward or right input, and stop
on execution breakpoints after the first successful contact selection.
Capture full actors before/after each allocation and state helper. Compare
the entire affected pool, manager span, or blend block with Windows output.
The values below were reproduced in both runs. Readers/writers listed in
the following section establish their interpretation.

Actors are Stewart at `0x803C04EC` and Thomas at `0x803C19C0`, each size
`0x14D4`. Offsets are relative to the actor or selected record/channel.

| Field / pointer chain | Type / size | Observed values and interpretation | Confidence / limitations |
| --- | --- | --- | --- |
| actor `+0x318` → manager | u32 pointer / 4 | Stewart `0x803C0EA8`, Thomas `0x803C237C`; both point to actor `+0x9BC` | verified for these actors |
| actor `+0x31C` → context | u32 pointer / 4 | Both point to actor `+0x4D8`; context `+4` contains channel count 10 | verified; count is u16 / 2 |
| actor `+0x320` → channels | u32 pointer / 4 | Both point to actor `+0x4E4`; ten records of size `0x7C` | verified; actor `+0x560` is channel **1**, not channel 0 |
| channel `+1` | u8 / 1 | Allocator first searches state 0, then state 1; initialized playback becomes state 2 | verified for the observed searches and writes; other producers not inferred |
| channel `+2`, `+4`, `+8`, `+0xA` | four u16 / 2 each | Clip keys `0x249` / `0x24A`, bank identity 0, group 30, group-table row `0x35` | verified; bank/group table remains supplied data |
| channel `+6`, `+7` | two u8 / 1 each | Bank key 1; program header byte 2 copied as `0x27` / `0x17` | verified copies; no meaning inferred for the header byte |
| channel `+0xC` | u32 / 4 | Copies context `+8`, including the run's varying heading value | verified copy semantics |
| channel `+0x30`, `+0x34`, `+0x38` | f32, f32, u32 / 4 each | Weights 1 / 1, blend mode 0 | verified initialization |
| channel `+0x1C`, `+0x20` | two u32 / 4 each | 0 and `0x01000000` | verified initialization; further semantics not inferred |
| channel `+0x24`, `+0x28`, `+0x2C`, `+0x48` | four f32 / 4 each | 0, 0, 1, 0 | verified initialization; later update roles not claimed |
| channel `+0x4C` → interpreter | mixed layout / first 48 bytes within channel | Program identities `0x80800012` / `0x80800050`; tag pairs `0x3E3D/0x3E3E` and `0x3E56/0x3E57`; period 2 | verified complete pool output; interpreter fields documented in command timing evidence |
| manager record `+4`, `+6`, `+8` | three u16 / 2 each | Group key, state, flags. Previous state 3 becomes 2 before callback. New group 30 starts state 3. Optional group `0xFFFF` starts state 1 | verified preparation; record stride `0x64`, four records |
| manager record `+0xC` | f32 / 4 | Zero for the tested active records; nonzero retirement path is rejected | verified bound for this reconstruction |
| manager record `+0x50` blend | f32 current/target, u32 mode, f32 initial, u32 duration / 20 | New group 0→1, mode 1, initial 0, duration 10. Retiring groups preserve current weight and target 0, with duration 10 | verified helper effects; advancing the blend over later frames is not implemented here |

The complete pool comparison covers 1,240 bytes, including preserved
channels. Manager comparison covers 404 bytes: four 100-byte records plus
the following four bytes touched by the original clear/copy routines.
The original clears **100 bytes starting at record +4**, preserving that
record's first word and reaching the next record's first word. This is not
a 64-byte clear.

## Original functions and native scope

| Original routine | Reconstructed behavior | Live checks across two runs |
| --- | --- | --- |
| `0x80173C24` | Search state 0 then 1, in index order | 4 full pool comparisons; Stewart channel 1, Thomas channel 5 |
| `0x80177950` / `0x80177ACC` | Compact holes and return first free manager record; reject full-manager eviction | 6 complete manager comparisons; no holes in these captures |
| `0x80173E78` / `0x80174444` | Allocate, initialize channel fields and blend, look up the group row in a supplied table, initialize the existing native interpreter | 4 full pool comparisons, plus flags and return indices |
| `0x80177EF4`, entry through callback dispatch | Retire zero-weight active records; allocate optional placeholder and new group; initialize new group blend | 4 complete manager comparisons; Stewart new slot 2; Thomas placeholder slot 2 and new slot 3 |
| `0x80179288` / `0x801791A8` | Blend target/current/mode/initial/duration stores; preserve current for sentinel −1 | 8 complete 20-byte blend comparisons |

Thomas's supplied transition table contains one entry; Stewart's contains
none. That source-table count determines the placeholder branch. The
native code derives slot choices and writes from the initial state and this
table. It does not receive the expected allocation index or initialized
manager records.

Channel fallback follows the original search for group bit `0x8000` and
lowest weight no greater than 1, choosing the later slot on a tie, then
stopping the selected channel. Nonfinite eligible weights are unsupported.
Offline guards exercise search priority, tie selection, bounds, full-manager
rejection, partial preparation rollback and invalid-program rollback.
Assertions remain enabled in the Windows checks, even for release builds.

## Reproduction and private artifacts

`capture_selection_handle_calls.py --trace-animation` requires
`--trace-contact-service --trace-event-stages`; it observes the animation
calls only after contact succeeds. It captures function arguments, f1/f2,
full actor objects, source bank/group data and clip programs. It uses
execution breakpoints; it never writes game RAM. Entry/return pairs must
be complete for verification. The verifier checks the DOL and saved-state
identities and supplies only initial blocks and source data to the replay.

Private files under `W:\Daywater\research-temp`:

| Artifact | SHA256 |
| --- | --- |
| `nflstreet-animation-confirm-forward-20261008.jsonl` — frame 46, 212.171 seconds | `2d8453ebe6a1ea345edd1cb0fa73110a4258cd084620ab707bd9799f374f52de` |
| `nflstreet-animation-confirm-right-20261008.jsonl` — frame 50, 212.811 seconds | `7f3546107439c25e62ff74333352c148eab296c3d9435cc284276a61c817774e` |
| `nflstreet-animation-replay-20261008.exe` | `4bbdf595f8c720d96fae36fbbbf0a67f5500c6ce6b55e5af7377613d1dd136ea` |

Each verification report contains 13 comparisons and 7,060 matched bytes.
Some comparisons overlap the same source memory; these counts are not a
measure of game coverage. Reports and binary replay inputs are private in
`animation-verification-forward-20261008` and
`animation-verification-right-20261008`.

The exploratory `nflstreet-animation-forward-20261008.jsonl` was stopped
before contact to correct the link-register tracing. It is incomplete and
excluded from all claims.

Build and verify from the repository root:

```sh
bash projects/nfl-street/reconstruction/build_windows_animation_replay.sh \
  /mnt/w/Daywater/research-temp/nflstreet-animation-replay-20261008.exe
python projects/nfl-street/reconstruction/actor_target_input/verify_animation_channels.py \
  --dol cache/extracted/813c4511ba3135059a13de424555647e8252254b/boot.dol \
  --trace /mnt/w/Daywater/research-temp/nflstreet-animation-confirm-right-20261008.jsonl \
  --executable /mnt/w/Daywater/research-temp/nflstreet-animation-replay-20261008.exe \
  --windows --work-dir /mnt/w/Daywater/research-temp/animation-verification-right-20261008
```

## Remaining work

- `0x80068E8C` callback metadata, including `0x80068310` positioning and
  `0x800618FC` motion bookkeeping, still changes manager data after preparation.
- `0x80178780` transition matching and anonymous-group bank selection remain
  original. Its observed blend helper writes are verified individually,
  but the native component does not yet decide or orchestrate all these calls.
- Bank resolution, program resolution and transition-table construction are
  source providers. Group renaming, nonzero-weight retirement, full-manager
  eviction and other callback families are unsupported.
- These components need joining with native contact selection, setup,
  timing, event processing and movement into one continuously owned segment.

No production DLL was replaced. Its baseline SHA1 remains
`dd521cc38b0db6bca98a0bb261570a2be5e6e0ed`. The scratch transport sources were
restored after each run: debugger SHA256
`25e959cb97dc6048dbcdd0ba606355b4d886041083d7f57885fe8c4fff040281`, session SHA256
`85d2063e8e33e241dc6983a66df1f06ae3630cd5d4839e1797605fca8a828c65`.

## Successor: native asset loading and successful transitions

The [asset loading and transition spawning reconstruction](asset_loading_transition_spawning.md)
now derives tested source data from disc bank bytes and verifies mode-three
clip spawning in two controlled original-game experiments. Earlier unsupported
source-loading/spawning notes above describe this component before that extension.
