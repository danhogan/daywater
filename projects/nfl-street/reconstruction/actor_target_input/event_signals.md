# Verified readiness commands and native handoff polling

## Result

The flags that advance events and permit handoffs now have live writer
evidence. Original command parser `0x8001532C` handles command `0xC004`
by setting actor flag `0x4`, and command `0xC006` by setting flag `0x1`.
Both arrive through callback dispatcher `0x80173A24`, called at
`0x80177084`. Their gameplay names and command-stream timing remain open.

The bounded native Windows gate now **decides whether to wait or hand off**
from the stage-three event and readiness bit. It clears that bit, finds and
updates the actor's record, and reproduces the observed conditional old-actor
flag cleanup. It is not given an expected HANDOFF decision. Across two
fresh right/forward runs, ten command effects and eight gate decisions match:
four waiting cases and four handoffs.

This extends [descriptor_event.md](descriptor_event.md), which constructs
incoming actors but replays explicit handoff actions. The new checks are
separate bounded replays; they do not yet form a complete native scheduler.

## Reference and capture procedure

- NFL Street GameCube GNNE69 revision 0; Dolphin 2603a.
- ISO SHA-1 `813c4511ba3135059a13de424555647e8252254b`.
- DOL SHA-1 `46e3de854998a0f66c9be8c72a1164d22cd29eb4`.
- First-down preplay state SHA-256
  `e2faeb8f9481096ff87ae261c8c2943452fbc023cd27510f641808e307603d93`.

Use [capture_selection_handle_calls.py](../../tools/capture_selection_handle_calls.py)
with `--trace-event-build --trace-handoff --trace-event-flags
--trace-event-gate --captures 3`. Arm breakpoints before pressing A, then
hold right or forward. The tool checks reference identities, records original
stores at `0x80015804` and `0x800157F0`, steps each store, and reads its result.
It captures the first waiting stage-three gate and the later ready gate for
Urlacher and Stewart. Gate entries are at `0x8007C758`; waiting exits are
`0x8007C808`, and handoff exits are `0x8007C7BC`, before a later optional
service call. It also captures the actor-transfer call and return inside
the handoff. The tool never writes game RAM.

The command stores preserve register `r30` as the incoming command from
`r4` and `r31` as the actor from `r3`. Saved return addresses identify
dispatcher return `0x80173A64` and upstream return `0x80177088` in every
checked command. The dispatcher's callback pointer is `0x8001532C`, and its
selector byte is zero. Captured context values vary; they are not assigned
animation or gameplay meanings.

## Fields and observed behavior

| Field | Address/pointer chain | Primitive type/size/endianness | Values and related code |
|---|---|---|---|
| Actor flags | Actor `+0xC`; Urlacher `0x803C6D1C`, Stewart `0x803C04F8` | unsigned 32-bit, 4 bytes, big-endian | `C004` ORs `4` at `0x80015804`; `C006` ORs `1` at `0x800157F0` |
| Stage word | Actor `+0x150+4` | unsigned 16-bit, 2 bytes, big-endian | `3` at all measured gate entries; dispatch at `0x8007C2FC–0x8007C300` |
| Callback pointer | `r13-0x6734`, `r13=0x803A9A40` | unsigned 32-bit pointer, 4 bytes, big-endian | `0x8001532C`; dispatcher reader `0x80173A54` |
| Callback selector | `r13-0x579C` | signed 8-bit in original dispatcher; 1 byte | `0`; reader `0x80173A30` |
| Dispatch-value pointer | `r13-0x7CF8` | unsigned 32-bit pointer, 4 bytes, big-endian | `0x803CD9A0`; accessor `0x8002E4F8` |
| Dispatch value | Dereference of that pointer | unsigned 32-bit, 4 bytes, big-endian | `3` at all gate entries; exact gameplay enum meaning unestablished |
| Incoming actor flags | Resolved event actor `+0xC` | unsigned 32-bit, 4 bytes, big-endian | Bit `0x400` participates in conditional transfer at `0x8003D838–0x8003D848` |

Each fresh run captures five command stores: three `C004` and two `C006`.
The first two `C004` stores on Stewart precede his later measured event;
these flag commands are not assumed to serve only this event path. The
later `C004` occurs with Stewart's stage word `1`. Each ready gate's
complete flag word equals the preceding `C006` store result for that actor.

| Gate | Before flags | After flags | Record result |
|---|---|---|---|
| Urlacher waiting | `0x10040000` | `0x10040000` | Urlacher remains |
| Urlacher ready | `0x10040001` | `0x10040000` | Stewart selected |
| Stewart waiting | `0x10044600` | `0x10044600` | Stewart remains |
| Stewart ready | `0x10044601` | `0x10040200` | Thomas selected |

The first native comparison correctly found a mismatch on the final row:
clearing only readiness bit `1` produced `0x10044600`. Original helper
`0x8003D81C`, called at `0x8007BFE8`, additionally clears old flags
`0x4400` when the old actor has `0x400` and the incoming actor does not.
Calls through `0x8003D704` implement those clears. Fresh traces verify the
helper inputs and outputs. The native poll now calculates this condition
from the incoming flags, rather than accepting the expected final flag word.

That helper also changes old/new actor byte `+8`, incoming actor flags,
and service/table state. Those additional effects are outside this API.
For example, the measured Stewart-to-Thomas helper changes Stewart's
byte `+8` from `0` to `0xFF` and changes Thomas's flags to `0x14040600`.
These observations are recorded as an implementation lead, not as a complete
native transfer or a proven controller mapping.

## Native implementation and checks

[event_signals.c](event_signals.c) contains:

- `nflstreet_event_flag_command`: effects of already accepted `C004/C006`
  commands. Context routing, command acceptance filters, and emission timing
  are not implemented.
- `nflstreet_stage3_record_poll`: accepts the observed event codes `4/14`
  at stage `3` and dispatch value `3`. It waits with records unchanged if
  readiness is absent. Otherwise it derives the incoming handle from the
  event, performs native record lookup/reassignment, and computes the old
  actor flag clears. Other states are rejected without partial mutation.

[event_signal_replay.c](event_signal_replay.c) runs these operations in
Windows without Dolphin, ModernGekko, an ISO, or a RAM image. The verifier
supplies captured operation inputs, including stage, flag words, dispatch
value and incoming flags; this is not independent timing or native boot.
Build with `reconstruction/build_windows_event_signal_replay.sh` and run
[verify_event_signals.py](verify_event_signals.py) with the original DOL,
private trace, executable and `--windows`. It checks exact parser dispatch,
stores, record lookup and conditional-transfer instructions, callback chain,
complete capture boundaries, and all native output lines.

Guard checks cover waiting without mutation, successful cleanup, unknown
commands, unsupported stages/dispatch values, and missing matching records.
Rejected calls preserve flags, records and both result outputs. The native
path does not implement earlier event stages or whole actor headers.

## Private artifacts and confidence

All artifacts remain in `W:\Daywater\research-temp`.

| Artifact | SHA-256 |
|---|---|
| `nflstreet-event-signal-transfer-right-20261007.jsonl` | `e1f93293dd1c5053205fa1e6b16ae018a99c03729b7efd732e34fb2b58eb63a1` |
| `nflstreet-event-signal-transfer-forward-20261007.jsonl` | `b4bc67473f9b8826c5bdb248889e054f62290f0639faa9df04e7a3bcdaaa727f` |
| `nflstreet-event-signal-transfer-replay-20261007.exe` | `ea49a3bb73f9237e74517bfba54ee834eb37d8a3910f8f3bf22980821328b809` |

**Confidence:** `verified` for the command callback/store chain, stage-three
waiting/ready decisions, record changes and measured old-flag cleanup in
these two fresh runs. `candidate` for command-stream timing, context and
gameplay meanings, and general transfer behavior. Original accessor cache
writes, other transfer effects, earlier stages, descriptor production,
animation/service behavior, and scheduling remain unimplemented. All
container source baselines were restored; the playable Windows DLL is unchanged.

## Accepted idle callback addition — 2026-10-09

The accepted-command handler now also supports C00C: original `0x8001532C`
sets actor +0x0C bit 0x40000 and returns 1. The bit's gameplay meaning is
unknown. Twelve controlled direct callback tests and three idle stream runs
match complete original actor blocks. The native viewer now runs selected idle
cycle 18 through original timing rather than skipping this command. Context
routing and clip selection remain outside this accepted-effect handler.
See [native idle stream evidence](../native_assets/native_idle_stream.md).
