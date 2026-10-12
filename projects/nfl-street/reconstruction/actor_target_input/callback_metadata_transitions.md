# Callback metadata and transition selection

## Result — 2026-10-08

The tested contact animation update is now reconstructed in Windows C,
including callback metadata and transition decisions. A single native paired
application carries both actors through links, requests, channel/manager
allocation, clip initialization, metadata, retirement and event completion.
It matches **both complete 5,332-byte actor objects and the complete
256-byte selection manager** in two fresh saved-match runs.

The earlier contact replay left animation-service byte differences outside
its ownership. This combined replay has **zero actor-byte differences** for
the tested successful contact. No original allocation index, metadata row,
transition outcome or output actor block is supplied to the native update.
This is a complete bounded contact update, not a full independent match loop.

## Reference and verification

Game: **NFL Street, GameCube GNNE69 revision 0**, Dolphin **2603a**.
Checked reference ISO SHA1:
`813c4511ba3135059a13de424555647e8252254b`.
Original DOL SHA1: `46e3de854998a0f66c9be8c72a1164d22cd29eb4`.
Saved match SHA256:
`e2faeb8f9481096ff87ae261c8c2943452fbc023cd27510f641808e307603d93`.

Procedure: restore the private first-down match in a fresh emulator process;
release held A at the event constructor; request forward or right movement;
capture execution-breakpoint entry/return pairs after the first successful
contact selection. Capture raw source assets, bank/program tables, transition
rows and descriptors, registry entries, profile data and actor objects.
Run the compiled Windows replay against those initial blocks and source data.
Reject incomplete captures and mismatched original-module identities.

Both runs reach the successful update at frame 51. Although requested stick
directions differ, the contact geometry is the same in these captures. The
evidence does not establish broad input, pose or game-mode coverage.

Each run passes:

- Six complete actor comparisons: callback, transition and complete group
  update for each actor.
- Two complete actor comparisons plus the whole selection manager for one
  paired native application. Its requests come from the existing **native**
  contact selector, with its candidate/RNG/parameter comparisons rerun first.
- Four direct native transition-row searches against original return values.

The right capture also checks seven shared-motion-global checkpoints.
The forward capture predates those extra global snapshots; it verifies actor
and manager outputs, but does not verify the shared global blocks. Counts
include overlapping checkpoints and are not game-coverage percentages.

## Reconstruction

### Callback — `0x80068E8C`, group 30, mode 0

Native code marks the matching manager record, decodes clip/bank keys from
the actor request, looks up the source program in the supplied bank table,
and calls the verified channel initializer. It records the resulting channel
index and group, sets actor flag `0x200`, and builds metadata.

`0x80068310` scans all nine supplied metadata assets in source order for a
type-0 row whose clip key matches the request. Stewart's type-1 request has
no matching primary type-0 row; the group-30 fallback leaves position data
alone. Thomas matches the type-0 row for clip `0x24A`. The native code derives
its partner from the selected record's handle and the two input actor
headers. It copies the row's local position, rotates it using the partner's
heading and the previously verified sine polynomial, translates by the
partner position, and wraps the combined heading to 24 bits.

For the observed group-30 fallback, `0x80068808` performs no stores: its
special groups are outside this branch. The native code omits its temporary
stack geometry calculation while preserving every output actor byte.

`0x800618FC` / `0x8004F0D4` use the source profile category and raw speed table
to initialize bookkeeping. `0x80103E64` sets the side byte from the raw
source byte: absent source returns 0; source byte 1 returns 1; other source
bytes return 0. These captures use source byte 0. Mirror paths are rejected.

### Transition — `0x80178780`

`0x801786D0` chooses the first playback channel with the largest weight
strictly greater than zero among state-2 channels in a requested group.
It does not receive a chosen channel index. Thomas's old group chooses
channel 2 and the new group chooses channel 5. Stewart's transition table
is empty; his old channel has been reused for the new group.

`0x80178D8C` scans supplied transition blobs and returns the **last** matching
row target. Exact bank/clip queries and wildcard group queries have different
destination rules. Source clip and progress-tag range are checked; wildcard
source clip skips the range check. The orchestration tries, in order:

1. Exact source and exact destination clip/bank.
2. Exact source and destination group.
3. Wildcard source and exact destination clip/bank.
4. Wildcard source and destination group.

The incoming source table contains one blob with 32 rows and all 32 raw
target descriptors. None matches the new group/clip in these runs. All four
original searches return zero, and native code independently reaches the
same result. It changes the retired manager record to state 1 and starts
the default blend to zero over duration 10, preserving current weight.

For the anonymous placeholder record, `0x80175E00` scans 91 source registry
entries. Twelve have the relevant kind/context type. Native code reproduces
the flag/key search and stores key `0x4051` in the record. The key is selected
from raw registry/bank/group data, not supplied as an expected result.

### Paired application

`nflstreet_contact_animation_complete` operates transactionally on both
actors, the selection manager and the motion globals. It sets reciprocal
links, applies the current request and full animation update, then applies
the incoming request and full update, then performs the owned contact/event
completion. The second callback uses the state produced by the first update.
No reference actor snapshot is reloaded between these calls.

The selector's candidate assets, parameters, RNG state, generated requests
and owned completion are still checked by `verify_contact_selection.py`.
The paired application consumes its generated request bytes and the original
initial actor/manager blocks. It does not consume the original paired requests
or the selector replay's partially completed output actors.

## New field evidence

All memory values below use the checked title/revision/DOL/Dolphin/state and
procedure above. Integers and IEEE binary32 floats are **big-endian**.
Actor layout and allocation fields are documented in
[animation_channels.md](animation_channels.md). Manager records have stride
`0x64`; callback data begins at record `+0x10`.

Stewart's new record is actor `+0xA84` (slot 2); Thomas's is `+0xAE8` (slot 3).
Their actor identities are `0x803C04EC` / `0x803C19C0`.

| Field / chain | Primitive / size | Observed values | Readers/writers / confidence / notes |
| --- | --- | --- | --- |
| callback data `+0`, `+0xC`, `+0x10` | three f32 / 4 each | 0 | `0x80068310`; verified bounded primary initialization / preserved current zeros |
| callback data `+4`, `+8` | two f32 / 4 each | Stewart 0 / 0; Thomas approximately −0.347445756 / −37.0967102, bits `BEB1E469` / `C2146308` | `0x80060764`; verified rotated/translated metadata for this row and pose |
| callback data `+0x14` | u32 / 4 | Stewart 0; Thomas `0x0050B4C7` | `0x80060764`; verified wrapped heading sum |
| callback data `+0x18` | u32 / 4 | 30 | `0x80068F3C`; verified group copy |
| callback data `+0x1C`, `+0x1D` | two u8 / 1 each | Stewart 0 / 1; Thomas 1 / 5 | `0x80068310`, `0x80068F38`; verified primary-row status / allocated channel |
| callback data `+0x1E`, `+0x1F` | two u8 / 1 each | 0 / 0 | `0x80103E64`, callback; verified observed unmirrored/side path; alternate producers unsupported |
| actor `+0x30C` → profile `+8` → table `0x8026E028 + 4*category` | pointer u32 / 4; category u32 / 4; table f32 / 4 | Category 2; speed 0 | `0x8004F4C0`, `0x8004F0D4`; verified observed table lookup; wider categories not live tested |
| callback data `+0x30` | f32 / 4 | 0 | `0x800618FC`; verified source speed / lower-bound branch for these profiles |
| callback data `+0x34`, `+0x36`, `+0x37` | three u8 / 1 each | 0, 0, channel 1 / 5 | `0x800618FC`; verified bookkeeping stores |
| callback data `+0x3E`, `+0x3F` | two u8 / 1 each | `FF` / `FF` | `0x80068310`; verified initialization |
| retired record `+6`, blend `+0x50` | state u16 / 2; blend mixed / 20 | State 2→1; target 0, mode 1, duration 10 | `0x80178780`, `0x80179288`; verified default transition; current weight preserved |
| anonymous record `+0xA` | u16 / 2 | `0x4051` | `0x80175E00`, `0x80178780`; verified registry-derived group/bank key; no clip meaning inferred |
| `r13 − 0x4F70`; `r13 − 0x4F6C` → registry records | count u32 / 4; pointer u32 / 4 | Count 91; pointer supplies the captured 91-entry array, stride `0x1C` | `0x80175E00`; verified bounded scan; absolute array pointer is not recorded as a fixed address |
| registry record `+0x12`, `+0x14`, `+0xC` → bank `+6`, `+8` → group table | kind u8 / 1; context u16 / 2; pointers u32 / 4; flags u16 / 2 | Twelve relevant entries have kind 1 / context 1; key `0x4051` has bit `0x4000` and selected bank flags include bit 8 | `0x80175E00`; verified observed lookup criteria; other registry kinds are skipped |
| `r13 − 0x7078` → source `+0x39` | pointer u32 / 4; source u8 / 1 | Non-null source with byte 0, callback side result 0 | `0x80103E64`; verified observed path; alternate byte values only statically reconstructed |
| `r13 − 0x5BB8`, `r13 − 0x5BBC`, with `r13=0x803A9A40` | two f32 / 4 each | 0 / 0; source writes speed×35 and speed×50 | `0x800618FC`; probable: exact snapshots match seven checkpoints in one fresh run; cross-run global snapshots remain untested |

The paired comparison also includes all previously verified link, request,
flags, stage and manager-reset fields. The complete actor comparison catches
unlisted mutations; it does not assign meanings to all unchanged bytes.

## Confidence and limits

- **verified:** complete group-30 callback/transition/paired application for
  the two identified descriptors and source profile/pose; program-table
  lookup; primary-row selection; default retirement; anonymous registry key;
  the four negative transition queries, reproduced in both fresh runs.
- **probable:** transition query behavior for positive matches, descriptor
  modes 0/1/2 and other profile/side values. Implemented from the original
  instructions, with offline query priority/range checks, but not live tested.
- **unsupported:** transition clip spawning (mode 3), mirroring, other callback
  groups/descriptors, full-manager eviction, nonzero-weight preparation
  retirement and optional contact positioning solver branches. Failure leaves
  the full paired input state unchanged.

Raw source banks, group/program tables, resolved metadata assets, transition
blobs/targets, registry entries and profile tables remain explicit providers.
Their complete bank/asset resolver and loader are not reconstructed here.
Native match initialization, longer scheduling/event progression, rendering,
audio and Windows host services remain open.

## Artifacts and reproduction

All raw data and executables are private under `W:\Daywater\research-temp`.

| Artifact | SHA256 |
| --- | --- |
| `nflstreet-callback-transition-confirm-forward-20261008.jsonl` — 241.763 seconds | `ff30f1a2c17644e7b6068ee12420661cd90b03f49b37717de151aa161c497a6b` |
| `nflstreet-callback-transition-confirm-right-20261008.jsonl` — 241.567 seconds | `518a766c3b6d0be936157891c91ec2fcfa78b81b933095cc15f2b4c7f65872df` |
| `nflstreet-callbacks-replay-20261008.exe` | `398d395bb0dfd15261efea0b7444cdf13c953b3c9061e4b8fbac7d1e16300b8f` |

Reports and replay inputs are in `callbacks-verification-forward-20261008`
and `callbacks-verification-right-20261008`. The exploratory
`nflstreet-callback-transition-forward-20261008.jsonl` failed at the initial
64-entry registry capture bound; the actual registry has 91 entries. It is
incomplete and excluded from verification claims. Capture bounds now allow
up to 4,096 entries and read bank data only for relevant entry types.

```sh
bash projects/nfl-street/reconstruction/build_windows_callbacks_replay.sh \
  /mnt/w/Daywater/research-temp/nflstreet-callbacks-replay-20261008.exe
python projects/nfl-street/reconstruction/actor_target_input/verify_animation_callbacks.py \
  --trace /mnt/w/Daywater/research-temp/nflstreet-callback-transition-confirm-right-20261008.jsonl \
  --dol cache/extracted/813c4511ba3135059a13de424555647e8252254b/boot.dol \
  --executable /mnt/w/Daywater/research-temp/nflstreet-callbacks-replay-20261008.exe \
  --contact-executable /mnt/w/Daywater/research-temp/nflstreet-contact-replay-20261008.exe \
  --windows --work-dir /mnt/w/Daywater/research-temp/callbacks-verification-right-20261008
```

Linux and Windows offline guards pass query priority, inclusive tag bounds,
malformed-blob rejection, unsupported mode-3 rollback, invalid-program
rollback and whole-pair rollback on second-actor failure. Windows assertions
are enabled even in release builds.

The playable baseline DLL was not replaced; SHA1 remains
`dd521cc38b0db6bca98a0bb261570a2be5e6e0ed`. Scratch debugger/session sources
were restored after every capture. Their baseline SHA256 values are recorded
in [animation_channels.md](animation_channels.md).

## Successor: native asset loading and successful transitions

The [asset loading and transition spawning reconstruction](asset_loading_transition_spawning.md)
now derives tested source data from disc bank bytes and verifies mode-three
clip spawning in two controlled original-game experiments. Earlier unsupported
source-loading/spawning notes above describe this component before that extension.
