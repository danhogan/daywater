# Native readiness-command timing

The Windows C replay now advances a bounded original command stream from
its initial state and elapsed-time inputs. It decides which updates emit
`0xC004` or `0xC006`; those emission times are no longer supplied as replay
actions. The commands feed the previously reconstructed readiness flag
effects. This is a simulation component, not an independently playable game.

## Reference and procedure

- NFL Street, GameCube US `GNNE69`, revision 0.
- ISO SHA1 `813c4511ba3135059a13de424555647e8252254b`.
- DOL SHA1 `46e3de854998a0f66c9be8c72a1164d22cd29eb4`.
- Dolphin `2603a`, first-down state SHA256
  `e2faeb8f9481096ff87ae261c8c2943452fbc023cd27510f641808e307603d93`.

`capture_selection_handle_calls.py --trace-event-flags --trace-command-timing`
starts a fresh reference session, snaps with A, and supplies right or forward
stick input. After the first watched readiness callback, it captures the
actor timing calls at `0x80173E34` / `0x8017425C`, the return at `0x80177484`,
and command dispatch/return at `0x80177078` / `0x80177090`. Each pair includes
64 original state bytes, 256 program bytes, and the input double from FPR1.
Register 33 maps to FPR1 in Dolphin's `GDBStub.cpp`; the captures show
`3ff0000000000000` (1.0). These are abstract update units; conversion from
host time into this delta remains outside the reconstruction.

The verifier runs the native executable through Windows `cmd.exe`, compares
all 64 state bytes, return value, emitted command sequence and original flag
stores, and carries its previous output into consecutive unchanged streams.
Program replacement or external state changes start a new bounded segment.
It reports unsupported updates separately and fails on a command-free trace.
The early broad trace and idle-state trace are retained as diagnostics; they
do not establish the full first-down transition.

## Original operations

The timing driver is `0x80177374`, calling the stream interpreter
`0x80176E28`. The observed stream is at actor `+0x5AC`: Stewart
`0x803C0A98`, Urlacher `0x803C72BC`. These addresses apply to the documented
saved state. The live flag callbacks also identify that same stream through
the original global at `r13-0x4F58`; they return into the driver at
`0x801773DC`.

All scalar fields below use big-endian storage. Function meanings are
limited to the listed operations; animation/gameplay meanings of raw tags
and control flags have not been established.

| Stream offset | Primitive / size | Operation and observations | Original readers/writers |
| --- | --- | --- | --- |
| `+0` | pointer / 4 | Points to command data, including `0x807F8A50` and `0x807F3CFE`; words start at data `+4`. Captured bytes are bounded, not a recovered file format. | `0x80176E80` |
| `+4` | float32 / 4 | Regular interval, including 2.0 and 1.3125. Driver adds it after advancing the stream. | `0x801773FC`, `0x80177400` |
| `+8` | float32 / 4 | Accumulator updated by the driver/interpreter; observed 3 → 4 → 5. Entire output word matches the reference. | `0x801770CC`–`0x801770D8`, `0x80177428`–`0x80177444` |
| `+0xC` | float32 / 4 | Countdown: subtract input delta; advance only if **strictly negative**; add interval after each advancement. Captures include 1 → 0 → -1 → 1 for interval 2. | `0x80177390`–`0x80177408` |
| `+0x10`, `+0x12` | uint16 / 2 each | Previous/next raw tags returned from ordinary words, including `0x13A5` / `0x13A5` and `0x0456` / `0x0457`. Interpretation as animation frames remains candidate. | `0x801773C8`–`0x801773F4` |
| `+0x14` | float32 / 4 | Driver stores `(interval-countdown)/interval`, or 1 on completion. Live fractional results include 0.5 and `3f4f3cf4`. | `0x80177450`–`0x8017747C` |
| `+0x18` | uint16 / 2 | Cursor in 16-bit words, incremented before dispatch. `0x8001` resets it to zero. | `0x80176E7C`–`0x80176E94`, `0x80176F30` |
| `+0x1C` | uint16 / 2 | Counter incremented for ordinary words, observed 3 → 4 → 5 and preserved across the cursor loop. | `0x801770B0`–`0x801770C0` |

One detail matters for byte equality: the first interpreter call uses the
**pre-subtraction countdown** still held in FPR1. Later catch-up calls use
the interval. Simply adding one interval for every advancement gives the
wrong accumulator. Ordinary words return a raw tag; their numeric value is
not used as the wait duration by this driver.

`0xC004` and `0xC006` dispatch to the original readiness callback, which
returns 1 without consuming additional words on this accepted-context
branch. The native implementation sets actor flag bits 4 and 1 respectively.
The callback selector/context must already be bound correctly by the caller.

## Native implementation and boundaries

- [command_timing.c](command_timing.c): countdown, cursor advancement,
  ordinary tags, accepted readiness callbacks, and bounded controls.
- [command_timing_replay.c](command_timing_replay.c): Windows replay harness.
- [verify_command_timing.py](verify_command_timing.py): original-module and
  live-call comparisons.
- [command_timing_checks.c](command_timing_checks.c): offline boundary checks
  for equality, multi-interval catch-up, rollback after an unsupported
  callback, and non-advancing control cycles.

Controls `0x8000`, `0x8001`, `0x8004`, `0x8005`, `0x8006` follow the inspected
DOL. Only branches exercised in the live comparisons receive live verified
confidence. Loop-stack controls `0x8002`/`0x8003`, other callback families,
zero intervals and context rejection are unsupported. Failure leaves state,
flags, command buffer and result outputs unchanged. Work and reads are
bounded to prevent malformed streams from hanging the host or reading
outside the supplied program.

Builds, reference command bytes and diagnostics remain private on W:.
The capture restores baseline container sources afterward and does not
replace the playable Windows DLL. Native replay needs its supplied state
and program bytes; it uses no Dolphin, ModernGekko, ISO or savestate loader.

## Results

Two fresh right/forward captures match **30 updates**: 26 command-free
updates, two `0xC004` emissions and two `0xC006` emissions. Each capture
contains six consecutive carried native updates. There are no unsupported
updates within these measured spans. Both bounded captures finish after
the Urlacher-to-Stewart handoff; they do not reach the later Thomas handoff.

**Confidence: verified** for the listed observed field operations and
readiness-command timing in these saved-state branches. Cursor looping
through `0x8001` is exercised. Completion and controls `0x8004`–`0x8006`
have static instruction evidence but no live coverage here; their general
behavior remains **probable**. Gameplay meanings of raw tags remain
**candidate**. Offline catch-up and rejection checks pass; multi-interval
catch-up has not yet been compared with a live delta greater than 1.

Private artifact:

| Artifact | SHA256 |
| --- | --- |
| `nflstreet-command-timing-fast-right-20261007.jsonl` | `40f64c7ae8c36c7eb026b7ff2c91af10c83d29e144e05c3d31e61471ae32b345` |
| `nflstreet-command-timing-fast-forward-20261007.jsonl` | `9adb151b63d4cbc3efca796563fe7b9f0f49801cdb3c04a685f68f9fca2d55cf` |
| `nflstreet-command-timing-replay-20261007.exe` | `af5dc1d90c84bfbb8a9cb073498a6646f0f6339c990f3a8f6bd99314673c2737` |

The newer [stream setup and early-event slice](stream_setup_early_events.md)
now reconstructs lookup/initialization and observed early decisions/steering.
Descriptor production, paired-asset selection, remaining callbacks and full
actor transfer services remain open. These components do not yet drive the
two-player movement replay or a whole match loop.

## Visible native idle/run integration — 2026-10-09

The original stream setup/timing reconstruction now drives the chosen native
viewer cycles 18 and 575. C00C's accepted actor flag effect is reconstructed;
three snapshots for each program match 1,200 full stream states and full actor
blocks. Callback routing is controlled in those offline comparisons. Original
locomotion clip selection, full context routing and transition blends remain
open. See [native idle stream evidence](../native_assets/native_idle_stream.md).
