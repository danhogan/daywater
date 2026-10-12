# Native descriptor-to-event-to-actor handoff

## Result

The Windows replay now derives incoming actor handles from original actor
descriptors and roster headers. It constructs the event itself, keeps that
event in native actor state, and uses its calculated target for the later
record handoff. Expected incoming actors and captured event outputs are
not supplied to the executable.

Two fresh right/forward restores each match three complete 12-byte event
blocks, the constructor's flags/input-mode outputs, and two record changes.
This extends [record_handoff.md](record_handoff.md), which supplied incoming
handles from captured handoff arguments. The native executable still starts
with captured actor data and the initial Urlacher record, and its build and
handoff actions are driven by the reference sequence. It does not generate
the descriptor or decide when an event is ready for handoff.

## Original path and observations

`0x8007C000` takes the current actor in `r3`, with its event at actor
`+0x150`. Its pointer at `+0x310` supplies a descriptor. Original code:

1. Clears the 16-bit event field at `+4`.
2. Widens descriptor byte `+2` into the 32-bit event word at `+8`.
3. Copies descriptor byte `+3` into event byte `+6`.
4. Passes actor header byte `+2` and descriptor byte `+1` to roster lookup
   `0x8003A4A0`, then copies the resolved actor's handle into event `+0`
   through `0x80020988`.
5. Leaves event byte `+7` untouched. Later branches may call services.
6. At the common tail, clears actor flag bits `0x4` and `0x1`, and zeros
   actor input-mode byte `+0x20E`.

The capture stops at the constructor entry, after the roster lookup/copy
at `0x8007C054`, and after the tail writes at `0x8007C16C`. All measured
event bytes are identical at the second and third boundaries for these
descriptors. The same original handoff/writer chain is captured afterward.

| Actor | Descriptor bytes | Derived incoming actor | Event code |
|---|---|---|---|
| Stewart | `1a0104020000` | Thomas, slot 8 | `4` |
| Urlacher | `1a000e002100` | Stewart, slot 7 | `14` |
| Stewart, later build | `1a0104022100` | Thomas, slot 8 | `4` |

Both fresh runs produce the same rows. The event codes and descriptor
bytes are raw values; no gameplay enum meaning is assigned. The first
Stewart event is built before the initial Urlacher record write. The replay
seeds that record separately and preserves observed event-build order.

## Fields, reference, and procedure

- NFL Street GameCube GNNE69 revision 0; Dolphin 2603a.
- ISO SHA-1: `813c4511ba3135059a13de424555647e8252254b`.
- DOL SHA-1: `46e3de854998a0f66c9be8c72a1164d22cd29eb4`.
- First-down preplay state SHA-256:
  `e2faeb8f9481096ff87ae261c8c2943452fbc023cd27510f641808e307603d93`.

| Field | Address/pointer chain | Type/size/endianness | Observed role |
|---|---|---|---|
| Descriptor pointer | Current actor `+0x310` | unsigned 32-bit pointer, 4 bytes, big-endian | Supplies the captured six descriptor bytes |
| Incoming roster member | Descriptor `+1` | unsigned 8-bit, 1 byte | `0` or `1`, passed as roster lookup member |
| Event code source | Descriptor `+2` | unsigned 8-bit, 1 byte | `14` or `4`, widened to event `+8` |
| Event byte source | Descriptor `+3` | unsigned 8-bit, 1 byte | `0` or `2`, copied to event `+6` |
| Roster team argument | Actor header `+2` | unsigned 8-bit, 1 byte | `1`, passed as the other roster lookup argument |
| Constructed event | Current actor `+0x150` | 12-byte block; handle/code unsigned 32-bit, stage unsigned 16-bit, big-endian | Derived handle, stage zero, copied byte, preserved byte, widened code |
| Actor flags | Current actor `+0xC` | unsigned 32-bit, 4 bytes, big-endian | Common tail clears mask `0x5` |
| Input-mode byte | Current actor `+0x20E` | unsigned 8-bit, 1 byte | Common tail writes zero; one captured input was `3` |

Readers/writers for these fields are the instructions at
`0x8007C014–0x8007C050` and `0x8007C150–0x8007C168`.
Later readers include processor `0x8007C180` and handoff `0x8007C7B8`.
The names **stage** and **input mode** describe the observed code usage;
their full state-machine/gameplay meanings are not established.

Run [capture_selection_handle_calls.py](../../tools/capture_selection_handle_calls.py)
with `--trace-event-build --trace-handoff --captures 3`. It hash-checks
the reference, captures all 14 roster handle words, arms breakpoints before
pressing A, and holds right or forward. Both complete runs advance from
12 to 56 dumped frames. Dump numbers do not establish a fixed update rate.

An earlier `--trace-event-process` capture also observed the event stage
word taking values `0,1,2,3` on Stewart and `0,3` on Urlacher before the
handoffs. That observation is a lead for reconstructing timing; it is not
a complete verified transition rule. Repeated processor-entry breakpoints
can greatly slow the game, so they are optional.

## Native code and verification

[event_descriptor.c](event_descriptor.c) implements the bounded constructor.
It accepts the observed descriptor code/byte combinations, calculates the roster
slot, checks that its header handle resolves consistently, constructs all
12 event bytes, and calculates the measured tail flags/mode outputs.
Unsupported forms, inconsistent roster entries, and invalid bounds reject
without changing outputs. It does not reproduce code-14 service effects
on other fields, or the rest of the constructor's branches.

[event_handoff_replay.c](event_handoff_replay.c) stores native events by
actor slot. Its `B` actions receive header/descriptor/prior-event bytes;
its `H` actions receive only the current actor handle. The incoming handle
is read from the native event, then the existing record lookup/copy updates
the record array. Neither expected incoming handles nor expected output
events are in the replay arguments. Build with
`reconstruction/build_windows_event_handoff_replay.sh`.

[verify_event_handoff.py](verify_event_handoff.py) checks exact constructor
and record-lookup instructions against the original DOL, validates capture
identity and complete boundaries, and compares every Windows output line
with the captured event/flags/mode and record outputs. Both fresh runs
match three event builds and two handoffs each. The executable runs without
Dolphin, ModernGekko, an ISO, or a RAM image; the verifier uses the original
DOL and trace for comparison.

Guard checks cover an unknown event code, an out-of-range descriptor member,
and an unresearched descriptor variant. They preserve event bytes, flags,
and mode on rejection. A synthetic prior flag word checks the common-tail
mask; a nonzero event byte `+7` checks that it stays untouched.

## Private artifacts

Under `W:\Daywater\research-temp`:

| Artifact | SHA-256 |
|---|---|
| `nflstreet-descriptor-event-handoff-right-complete-20261007.jsonl` | `8be44ac6265aa5e85167061a7e02ccf2fef6f5ab16c201885fc177bb01dd9364` |
| `nflstreet-descriptor-event-handoff-forward-complete-20261007.jsonl` | `3d7c2cdd29af57530a6b6a0dcd25867828539320156e0eba8a09263a999efc63` |
| `nflstreet-event-handoff-replay-20261007.exe` | `1611019b7fce8f63a74ed9447b7bf51cdd398c07082f26c470efe60d944bb6b5` |

**Confidence:** `verified` for the listed descriptor reads, event outputs,
tail flags/mode, resolved incoming actors, and native handoffs in these two
fresh runs. `candidate` for gameplay meanings, descriptor production,
event-stage timing, and general scheduling. This is a native data-to-event
continuation, not native match startup or a standalone playable game.
Container sources were restored after each capture; the baseline Windows
DLL remains unchanged.
