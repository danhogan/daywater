# Native stream setup and early event stages

This slice reconstructs stream table lookup, initialization, and the observed
stage-zero/one decisions and pre-contact stage-two steering. It runs as Windows
C without Dolphin or ModernGekko. It still receives private source tables,
programs, descriptors, geometry and service inputs from the reference.
The contact asset matcher and complete actor transfer are outside this slice.
The subsequent [contact component](contact_selection.md) now reconstructs
selection and bounded completion writes; animation-channel effects remain open.

## Reference and procedure

- Game: NFL Street, GameCube GNNE69 revision 0.
- ISO SHA1: `813c4511ba3135059a13de424555647e8252254b`.
- DOL SHA1: `46e3de854998a0f66c9be8c72a1164d22cd29eb4`.
- Dolphin: **2603a**, with the private first-down state SHA256
  `e2faeb8f9481096ff87ae261c8c2943452fbc023cd27510f641808e307603d93`.

Restore the state independently for right and forward input. Stop at stream
lookup calls `0x80173F8C`/`0x80174200`, initializer `0x8017724C` and return
`0x80177338`. Capture table prefixes, program bytes, all 64 interpreter bytes
and actor flags. Native lookup produces the identity passed to native setup;
compare its complete output with the original return.

Stop after event preflight at `0x8007C2D8` and return `0x8007C930` for watched
roster slots 7/12. Capture descriptors, geometry, event, flags, request,
control, record handles and target. Additional stops at `0x8007B9C0` and
`0x8007BA20` supply the tracking service result and field parameters.
Native code independently chooses the supported transition and steering
output. Compare all event, flags, request and control bytes. Missing inputs
and unported contact calls are counted separately, never as matches.

## Observed layouts

All multibyte fields below are **big endian**. Absolute roster base is
`0x803B7320`, stride `0x14D4`; the watched actors are slot 7
(`0x803C04EC`) and slot 12 (`0x803C6D10`). These offsets are representation
boundaries, not a semantic API for gameplay policy.

| Location | Type / size | Observed operation | Readers/writers | Confidence |
| --- | --- | --- | --- | --- |
| Actor `+0x5AC`, stream `+0` | uint32 / 4 | Program identity selected from the supplied table | `0x8017484C`, `0x8017724C` | verified |
| Bank `+0xC` → table `+4+4*index` | pointer, uint32 / 4 each | Lookup indices 309, 85 and 585 select three programs | `0x80173F8C`, `0x80174200`, `0x8017484C` | verified |
| Program `+0` | uint8 / 1 | Interval byte times 1/16; observed 32→2 and 21→1.3125 | `0x8017724C` | verified for those values; zero fallback probable |
| Stream `+4`, `+8`, `+0xC`, `+0x14` | float32 / 4 each | Interval, accumulator, countdown and fraction initialized to interval, zero, interval, zero | `0x8017724C` | verified |
| Stream `+0x10`, `+0x12` | uint16 / 2 each | First two interpreted tags; observed `13A5/13A5`, `0451/0452`, `3E3D/3E3E` | `0x8017724C`, `0x80176E2C` | verified; tag gameplay meaning candidate |
| Stream `+0x18`, `+0x1A`, `+0x1C` | uint16 / 2 each | Cursor, marker and tag counter initialized then advanced by two interpreter calls | same initializer/interpreter | verified for captured programs |
| Stream `+0x1E`, `+0x1F` | uint8 / 1 each | Interpreter control and stack-depth bytes reset | same initializer | verified |
| Actor `+0x150`, event `+4` | uint16 / 2 | Stage 0→1 for code 4; 0→3 for code 14; stage 1 waits or advances to 2 | `0x8007C180` after preflight | verified for captured branches |
| Event `+6`, `+7`, `+8` | uint8 / 1, uint8 / 1, uint32 / 4 | Observed mode 2, request selector written 4, codes 4/14 | `0x8007C180`, `0x8007B4F0` | verified operations; broad code meanings candidate |
| Actor `+0xC` | uint32 / 4 | Stage entry clears bit 4 while preserving other bits | `0x8007C180` | verified |
| Actor `+0x3F0` | 3 bytes | Stage-one `04 01 01`, stage-two `03 02 01`; code 14 writes only first byte 1 | `0x8007B4F0` | verified in mode `+0xB5D=0`; other mode branches probable |
| Actor `+0x200`, `+4`, `+8`, `+0xE` | float32 / 4, uint32 / 4 each, uint8 / 1 | Speed 1, duplicated 24-bit heading, control byte 4 | `0x8007B928`, stores `0x8007BEA4`–`0x8007BEB0` | verified for captured steering calls |

Steering consumes geometry at actor `+0x1A8`, incoming geometry at the same
relative offset, and the manager record target at record `+0xC`. Existing
angle and sine reconstruction supplies the game's arithmetic. Captured
tracking service result is 1 and the 12-byte field input is
`00000000c2040000c0200000`; wider service behavior remains unported.
Near-threshold floating-point behavior, zero vectors and the alternative
tracking branch have no live coverage in this slice.

## Contact boundary and pair requests

Stage two can enter `0x8007B73C`. The observed incoming descriptor begins
`1B`. Its contact gate uses distance below 4 and wrapped angle difference
below `0x2AAAAA`, or a handle-tag/actor `+0x408` exception. Native code
rejects this service branch transactionally. A heading output value
`0x155555` in a different parameter field must not be mistaken for that
angle limit. The callback pointer was observed as `0x8005E5E0`; other
descriptor and eligibility branches remain unimplemented.

The matcher `0x8005EEAC` selects a paired-animation asset. Given that
**already selected** asset, `pair_requests.c` reconstructs `0x8005FA04`:
asset `+4` is a big-endian uint16 row count, rows begin at `+8` with stride
20, row byte `+2` distinguishes two types, and each type's first row begins
with a uint16 stream key. Parameters `+0x24..+0x2B` and `+0x30` supply
the common request fields; choice supplies the uint16 request `+0xC`.
The observed rows yield current key `0x249` and incoming key `0x24A`.
Their broader gameplay names remain **candidate**.

Two fresh execution-probe runs match both 16-byte requests and independently
derive key 585 (`0x249`), which matches the subsequent lookup. **Confidence:
verified** for this selected-asset row layout and request construction in
these runs. Asset matching, choice production, the contact gate's broader
behavior and unobserved row variants remain **probable** from static code
or unimplemented; they are not native live-verified contact completion.

The current request's key can drive native table lookup and setup. This
does not select the asset, allocate a channel, apply all paired geometry
effects, or advance a complete stage-two event natively. Contact capture
results and confidence are recorded below.

## Implementation and verification

- [command_timing.c](command_timing.c): shared interpreter, lookup and setup.
- [early_event.c](early_event.c): guarded early-stage decisions and steering.
- [pair_requests.c](pair_requests.c): bounded selected-asset parser.
- [verify_setup_stages.py](verify_setup_stages.py): Windows output comparison,
  source identity checks and explicit unsupported/missing counts.
- [setup_stage_checks.c](setup_stage_checks.c): offline rollback, table bounds,
  zero-header interval fallback, preserved tail and missing paired-row checks.

Build using `reconstruction/build_windows_stream_setup_replay.sh` and
`reconstruction/build_windows_early_event_replay.sh`. Invoke the verifier
with `--trace`, `--dol`, `--setup-executable`, `--event-executable` and
`--windows`. Run the offline checks by linking `setup_stage_checks.c` with
the three libraries above, `event_signals.c`, `target_selection.c` and
`kinematics/kinematics_replay.c`, defining `NFLSTREET_KINEMATICS_NO_MAIN`.

Write-watchpoint contact experiments produced invalid actor-memory reads
and are excluded from verification. Repeated execution-breakpoint captures
are required for accepted contact evidence. All captures restore baseline
container sources afterward; the playable DLL is unchanged. Private traces,
programs, assets and Windows executables stay on W:.

## Results

Two clean fresh right/forward runs match **six complete initializations and
six table lookups**, **12 early event updates**, including **six steering
outputs**, and **two pairs of 16-byte requests**. Each pair supplies the next
native lookup key. All 64 interpreter bytes are compared, including preserved
tail bytes. No tracking inputs are missing. **Ten contact-service updates
are explicitly unsupported** by the native event processor; these include
the original successful stage-two completions and are not counted as matches.

**Confidence: verified** for the observed initializations, supported event
decisions and steering outputs across these fresh restores. This proves
bounded output equivalence, not ownership of the whole event lifecycle.
The captures end at the 120-second observation bound with complete setup
and event call boundaries. Both contain two record-copy captures; they do
not establish all record writer paths. The previous 30-update command
timing comparison still passes after sharing its interpreter with setup.
Offline rejection/bounds checks pass on Linux and Windows.

Private W: artifacts:

| Artifact | SHA256 |
| --- | --- |
| `nflstreet-setup-stages-clean-contact-right-20261008.jsonl` | `bed9463a7abec4fcacb2b0f93955e081d841c0a2524a69623dff2102f714ad2e` |
| `nflstreet-setup-stages-clean-contact-forward-20261008.jsonl` | `722ed579ad81ae37844bbceac26013b96e37eb71f1d414cf1f9dfd8988041947` |
| `nflstreet-stream-setup-replay-20261008.exe` | `90e7f97610cce56618b0bddf35f3000d751bfac51e5f2598ec9602628f977f98` |
| `nflstreet-early-event-replay-20261008.exe` | `2813965d0db59f69c32d37b4f85c840e6331465864635720d56cb70cd2e3f621` |

Baseline playable DLL SHA1 remains
`dd521cc38b0db6bca98a0bb261570a2be5e6e0ed`. Container debugger/session sources
were restored after each capture; game-specific runtime and tracing changes
remain isolated under this project.
