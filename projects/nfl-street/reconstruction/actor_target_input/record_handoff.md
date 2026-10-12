# Verified record writes and native actor handoffs

## Result

Fresh Dolphin restores now identify the live writer behind the first-down
record changes. Call `0x800BE2F8` copies the word at an actor's base into
record `+0xB4`: empty → Urlacher → Stewart → Thomas. Index zero stays fixed.
Execution breakpoints at the call and its return establish each copy;
earlier no-hit write watchpoints did not establish this writer.

A bounded Windows C replay reproduces the two later handoffs. It starts
with the captured Urlacher record, finds the record for the current actor,
and copies the incoming actor's handle. It receives no expected record
index. Both fresh right and forward runs match, giving four native handoffs.

## Reference and procedure

- NFL Street GameCube GNNE69 revision 0; Dolphin 2603a.
- ISO SHA-1: `813c4511ba3135059a13de424555647e8252254b`.
- DOL SHA-1: `46e3de854998a0f66c9be8c72a1164d22cd29eb4`.
- First-down state SHA-256:
  `e2faeb8f9481096ff87ae261c8c2943452fbc023cd27510f641808e307603d93`.

Run [capture_selection_handle_calls.py](../../tools/capture_selection_handle_calls.py)
with `--captures 3 --trace-handoff`. It hash-checks the reference, restores
preplay, arms execution breakpoints, presses A, and holds right or forward.
It records source/destination words before the copy and the destination at
its original return. Handoff arguments are captured at `0x8007C7B8`.
No game RAM is written by the tool.

Right advances from 15 to 59 dumped frames, with writes on frames 16/23/59;
forward advances from 13 to 57, with writes on frames 14/21/57. These are
dump observations, not a verified simulation cadence.

| Field | Address/type/size | Values in both fresh runs |
|---|---|---|
| Selection manager | Pointer at `r13-0x73D0`, `r13=0x803A9A40`; unsigned 32-bit, big-endian | `0x803B6F00` |
| Record count | Manager `+0`; unsigned 8-bit, 1 byte | `1` |
| Record index | Manager `+1`; unsigned 8-bit, 1 byte | `0` |
| Record | Manager `+8+index*0xF4` | `0x803B6F08` |
| Record handle | Record `+0xB4` = `0x803B6FBC`; unsigned 32-bit, 4 bytes, big-endian | `0 → 0x00050101 → 0x00000101 → 0x00010101` |
| Urlacher source handle | Actor base `0x803C6D10`; unsigned 32-bit, 4 bytes, big-endian | `0x00050101`, slot 12 |
| Stewart source handle | Actor base `0x803C04EC`; unsigned 32-bit, 4 bytes, big-endian | `0x00000101`, slot 7 |
| Thomas source handle | Actor base `0x803C19C0`; unsigned 32-bit, 4 bytes, big-endian | `0x00010101`, slot 8 |

Writers: wrapper `0x800BE2B4`, helper call `0x800BE2F8`, helper store
`0x80020994`. Readers: record scan `0x800BE4F8`, actor lookup
`0x80020944 → 0x8003A4A0`, and the previously verified target choice chain.
The clear call `0x800BE29C` remains statically established. A longer forward
run reached its entry at the time limit without a return capture; that
incomplete call is not verified clear-write evidence.

## Upstream boundary

The wrapper's saved return address identifies the same two paths in fresh
right/forward three-write runs:

- Initial Urlacher assignment: call `0x800F91E8`, return `0x800F91EC`.
  Original instructions obtain an actor through `0x8003A670`. The decision
  selecting Urlacher remains unimplemented.
- Stewart and Thomas: call `0x8007BF10`, return `0x8007BF14`, within the
  function beginning at `0x8007BEE8`.

The DOL has one direct caller of `0x8007BEE8`, at `0x8007C7B8`. Just
before it, `0x800BE4F8` scans records, resolves their handles to actors,
and returns the first matching record or null. Both live handoff traces
confirm its result feeds record `0x803B6F08` into the subsequent writer.

| Current actor | Incoming actor | Raw event argument `+4` / `+8` |
|---|---|---|
| Urlacher | Stewart | unsigned 16-bit `3` / unsigned 32-bit `14`, big-endian |
| Stewart | Thomas | unsigned 16-bit `3` / unsigned 32-bit `4`, big-endian |

These values do not establish gameplay enum meanings. The original path
checks actor flags and event codes before the handoff; generating the
events and selecting the incoming actor are still open.

## Native reconstruction and checks

[target_selection.c](target_selection.c) implements
`nflstreet_record_handle_handoff`: resolve the current actor, scan record
handles, return the matching index, and copy the incoming handle. Only
the documented tag-1 actor branch and empty records are supported. Missing
actors, unsupported handles, or invalid bounds are rejected without state
changes. The assignment wrapper's other field writes/services are absent.

[record_handoff_replay.c](record_handoff_replay.c) carries its record array
through both transitions in one Windows executable. Build with
`reconstruction/build_windows_record_handoff_replay.sh`; run
[verify_record_handoff.py](verify_record_handoff.py) with the private trace,
original DOL, executable, and `--windows`. The verifier checks original
scan/argument instructions and complete writer chains, then compares native
indices and handles. The executable uses no Dolphin, ISO, or RAM image.
The verifier uses the DOL and captures as comparison evidence.

Native guard checks cover another record index, empty records, missing
actors, and unsupported tags. Rejected calls preserve handles and output
index. A deliberately corrupted captured return word is rejected.

## Private artifacts

Under private `W:\Daywater\research-temp`:

| Artifact | SHA-256 |
|---|---|
| `nflstreet-firstdown-handle-calls-right-three-20261007.jsonl` | `ff39a7339059cf42206bad39bae06712be26205dbe6151edaadd03fda99e0b4d` |
| `nflstreet-firstdown-handle-calls-forward-three-20261007.jsonl` | `839980e9f71f9865ada608f20fa3bcea69741014ce4e203aea7fa55226601049` |
| `nflstreet-firstdown-selection-handoffs-right-20261007.jsonl` | `32f92ffa8e19b098ec3fa19d86ca04a568526a97a579469f84e0fd8db7112807` |
| `nflstreet-firstdown-selection-handoffs-forward-20261007.jsonl` | `4ab8764adeda4d81abbc48399365530cdbb291ca60491e20567eb5bbb57ceaa4` |
| `nflstreet-record-handoff-replay-20261007.exe` | `feb8adcaa80de579126e8e2f5f3c97e37fd95a936cb6a147ee75f69678860317` |

**Confidence:** `verified` for live record writes and actor sources in
repeated fresh right/forward restores, and for the two later handoff
arguments and bounded native lookup/copy in two fresh runs. `candidate`
for gameplay event meanings and the rules selecting incoming actors.
This replay accepts captured events; it does not generate them, run a
match, or replace the whole assignment wrapper. All container source
baselines were restored after captures; the playable DLL is unchanged.
