# Animation asset loading and transition clip spawning

## Result — 2026-10-08

Windows C can now load the tested animation bank data from the original
privately extracted `animdata.dat`, derive metadata/program/transition sources
from the initial registry, and execute a successful transition that starts an
extra clip. No resolved candidate assets, selected transition, chosen channel,
clip duration, or expected actor output is supplied to that native update.

Two fresh original-game experiments exercised different real descriptors:

| Source descriptor | Clip key | Factor | Original duration | Ramp ticks | Hold ticks | Group/channel hold |
| --- | --- | --- | --- | --- | --- | --- |
| `0x80803360` | `0x75` | 0.5 | 52 | 13 | 26 | 39 |
| `0x808033C0` | `0x84` | approximately 0.8 | 124 | 12 | 100 | 112 |

Each run matches **six complete 5,332-byte actor checkpoints**, all six motion
global checkpoints, and the original program-duration return. This includes
the callback, transition, and enclosing group update for both actors. The
successful transition allocates channel 6, uses bank registry key 1 and the
registry-derived anonymous key `0x4051`, and starts a clip in group `0xFFFF`.
All actor byte differences are zero. Twelve resident context-one bank source
blocks are loaded and relocated natively in each disc replay.

These are **controlled experiments**: an explicit capture option made one
existing source row eligible by changing its destination/source keys and
progress range in an ephemeral Dolphin process. Original instructions and
transition descriptors remained unchanged. This verifies mechanics of the
successful branch, **not naturally occurring eligibility during gameplay**.
The earlier unmodified contact capture still takes the default retirement
path; its paired Windows replay also passes after these changes.

## Identity and reproduction

- Game: NFL Street, GameCube GNNE69 revision 0.
- ISO SHA-1: `813c4511ba3135059a13de424555647e8252254b`.
- DOL SHA-1: `46e3de854998a0f66c9be8c72a1164d22cd29eb4`.
- Dolphin: 2603a.
- First-down state SHA-256:
  `e2faeb8f9481096ff87ae261c8c2943452fbc023cd27510f641808e307603d93`.
- `animdata.dat`: 9,232,384 bytes; SHA-256
  `a0856f4241086f4568fdfce7821decc3710b57ef4bc1c4e8e3055886cf1f242a`.
- Final Windows executable SHA-256:
  `a6d41c545dd2033d6c6eae9ec8a668c28895565bc859f38563f426786754f80b`.

All assets, captures, source disassembly, replay inputs and reports stay in
`W:\Daywater\research-temp`. Relevant private files:

- `nflstreet-assets-spawn-confirm-20261008.jsonl`: 453 events, 98 paired
  animation calls; SHA-256
  `5a1e082ccd1da9d5d8f0c7fbbf3d3fc60463f13d88680efe84e513315d308f6c`.
- `nflstreet-assets-spawn-second-20261008.jsonl`: 453 events, 98 paired
  calls; SHA-256
  `867b18ba34090a2883d738e60f875be7a942e09cedef072fdb82094e352d60e8`.
- `windows-disc-assets-spawn-first-20261008/asset-verification.json`.
- `windows-disc-assets-spawn-second-20261008/asset-verification.json`.
- `windows-assets-baseline-regression-20261008/`: original unmodified
  contact regression, eight actor comparisons, four selector comparisons,
  complete paired manager comparison and seven motion-global comparisons.

The initial diagnostic experiment (`nflstreet-assets-spawn-experiment-20261008`)
patched the wrong old group's source clip. It reached no successful transition
and is excluded from positive spawning claims.

Reproduce using `extract_animation_data.py` (hash-checked ISO, FST lookup by
filename; output restricted to private W:), the explicit source-row experiment
in `capture_selection_handle_calls.py`, the existing capture wrapper, and
`verify_asset_loading.py --animdata <private-file> --windows`. The second run
selects source row 6; the first selects row 0. Capture flags are
`--trace-animation-details --trace-loaded-assets --experiment-transition-clip`.

The verifier requires complete paired calls and the documented DOL/state/build.
In disc mode it starts with a **zero-filled source arena**, finds each unique
raw bank by its source identity header, and passes original offset-form bytes
to C. C relocates them and resolves all group/metadata/transition/program data
before applying the update. Only the initial registry and actor/environment
state remain reference providers. Controlled source edits are applied to the
raw row's first 12 bytes; descriptor pointers are relocated by C.

## Reconstructed source paths

- `0x80175F48`: bounds-checked registry bank selection; a context value of
  `0x8000` marks an unavailable bank.
- `0x80175D18`: first registry entry of kind 1 with the requested context and
  bank tag; original missing result `0xFFFFFF`.
- `0x801758E0` / `0x80174860`: registry-order group lookup, bounded to 32 bank
  matches. Native lookup uses the uncached path; cache population is omitted.
- `0x80175C18` / `0x801749B4`: metadata asset pointer from matching group rows;
  last matching bank wins. Callback candidate keys mask bit `0x8000`, as in
  the original callback. The nine group-30 assets are derived natively.
- `0x80175AF8` / `0x801748A0`: collect metadata references from group chunks.
- `0x80175C88` / `0x80174934`: build the transition source list from nonempty
  group transition chunks, preserving registry order and original capacity 8.
- `0x80174574`: core bank offset relocation, including group reference lists,
  transition descriptor pointers and program pointers. Offset-form banks with
  cross-reference flag 4 are rejected atomically. The optional renderer hook
  and its allocation side effects are **not** implemented by this loader.
- `0x80178780` mode 3: select the anonymous key, resolve descriptor bank tag,
  allocate/init its clip, derive duration, set blends/envelope and group hold.
- `0x8017749C` / `0x801770FC`: duration scan after the original two-tag setup.
- `0x80179310`: mode-three blend parameter layout.
- `0x80178608`: matching active manager records receive the hold duration.
- `0x80174330`: active channels in the new group receive that duration at +0x48.

## Fields and evidence

The tested small-data register `r13` is `0x803A9A40`.
All original multi-byte fields below are big-endian. Tests restore the documented
state, select a real descriptor through the controlled row mutation, capture
original entry/return objects, and compare the complete native actor object.
Actor objects are Stewart `0x803C04EC` and Thomas `0x803C19C0`, size `0x14D4`.
Manager/header/channels come from actor +0x318/+0x31C/+0x320 as documented in
[animation channels](animation_channels.md).

| Field / source | Type / size | Observation and functions | Confidence |
| --- | --- | --- | --- |
| Registry pointer at `r13-0x4F6C`; count at `r13-0x4F70` | u32 each; entries 28 bytes | 91 entries; entry +0x12 kind u8=1, +0x14 context u16=1, +0xC bank pointer; `0x80175D18`, `0x80175F48` | verified for these initial registry lookups |
| Bank +2/+6/+8/+0xC | tag/flags u16; group/program pointers u32 | source bank key 1, tag 0; group-30 lookup and native program lookup match both fresh original runs | verified tested resolution; broader bank installation remains open |
| Descriptor +4/+8/+0xC/+0xE | mode u16, factor f32, bank tag u16, clip u16; descriptor 16 bytes | mode 3; factor 0.5 / approximately 0.8; tag 0; clip 0x75 / 0x84; `0x80178780` | verified controlled mechanics |
| Spawned channel +2/+4/+6/+8/+0xA | clip/tag u16, registry u8, group/row u16 | channel 6; clip 0x75 / 0x84, tag 0, bank key 1, group 0xFFFF, row 15 | verified controlled mechanics |
| Retired record +0xA | u16 / 2 | 0x4051 from anonymous registry lookup; `0x80175E00`, `0x80178780` | verified |
| Retired record +0x50 and +0x118 | mixed blend / 20 bytes each | ramp durations 13 / 12, original current/targets preserved as specified by `0x80179288` | verified controlled mechanics |
| Retired record +0xB4 | mixed mode-three envelope / 24 bytes | mode u32 at +8=3; parameters at +12: initial f32, ramp f32, ramp u8, zero cursor u8, hold u16; `0x80179310` | verified controlled mechanics |
| Group-30 manager record +0xC; active group-30 channel +0x48 | f32 / 4 each | 39 / 112; `0x80178608`, `0x80174330` | verified controlled mechanics |

The live mode-size table at `[r13-0x6710]` has prefix `00 08 04 0C`,
consistent with the copied 12-byte mode-three parameter block. The installed
relocation hook at `[r13-0x5798]` is `0x80151928`. These were read from the
second experiment process and checked against original disassembly;
interpretation outside the tested envelope is **probable**.

## Limits and ownership

The native **tested animation source loading and successful transition** are
verified by independent original instructions and full actor comparisons.
The complete disc/container loader is **not finished**. Disc bank identification
and resident layout still use initial registry/resource identities from the
saved match; TERF directory/manifest traversal and fresh registry installation
remain open. The prototype loader relocates library metadata/program pointers;
it does not reproduce the full renderer relocation hook (`0x80151834`), frame
payload splitting/allocation (`0x80151620`), compressed frame decoding, or all
bank flags. The disc-replay frame-pointer identities are unused in this slice.
No full loaded-bank byte-equivalence claim is made.

Coverage remains group 30, descriptors 0x1A/0x1B, unmirrored metadata, the tested
actor pool, and the documented source rows. Unsupported cross-reference banks,
other stream controls, mirroring, pool eviction and other callback families
still require evidence. Negative/malformed source paths reject without changing
actor state. Offline Linux and Windows checks cover bounds, truncated relocation,
missing bank lookup, duration scan truncation, idempotent relocation and paired
rollback. Both Windows disc replays pass; the existing unmodified paired contact
replay also passes.

The playable DLL was never replaced and retains SHA-1
`dd521cc38b0db6bca98a0bb261570a2be5e6e0ed`. Each live run restores the container's
baseline debugger/session sources. Experiment processes and temporary user
directories are discarded; original ISO, DOL, extracted file and saved match
remain unchanged. Game-specific code stays outside generic Daywater.
