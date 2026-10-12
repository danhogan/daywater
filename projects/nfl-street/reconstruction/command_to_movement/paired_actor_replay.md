# Two-actor native replay

In the GNNE69 revision 0 saved match, Stewart (observed roster slot 7)
updates before Thomas (slot 8) on three consecutive dumped frames. At the
next Stewart update, the game passes Thomas's position plus `(0, -2, 0)`
as Stewart's target. This is an observed dependency between two actors,
not a general actor-selection rule.

The [paired capture tool](../../tools/capture_paired_actor_updates.py)
recorded preparation, movement entry, and movement exit for both actors
in each frame. It also recorded the target passed into `0x8008D864` before
Stewart's second and third updates. The [offline verifier](verify_paired_actor_updates.py)
checks event order, actor pointers, consecutive frames, and these target
handoffs. It runs the existing Windows x64 actor loop for Thomas and
Stewart, using only their initial captured actor and movement states for
each three-update run. The two later Stewart targets are computed inside
the Windows actor loop by the [guarded C target composer](../actor_target_input/target_selection.c)
from the *native Thomas outputs* and the observed offset. The verifier
also compares that dependency with the game's target bytes. All six actor
output blocks and all six
21-word movement output blocks match the original in each run.

| Held input | Dumped frames | Matched updates | Native inter-actor targets | Private W: trace SHA-256 |
|---|---:|---:|---:|---|
| Right | 22–24 | 6 | 2 | `7ac46ded000394a54c271ec4773e01f7edbeeb69083fb9ed2aabedf45fc834df` |
| Forward | 30–32 | 6 | 2 | `91f1c6d808df066ebbdb110874707e176afbe19e5097e2f0ea929a441544b349` |

The traces are `nflstreet-stewart-thomas-paired-right-three-20261007.jsonl`
and `nflstreet-stewart-thomas-paired-forward-three-20261007.jsonl` on
private W: storage. The new Windows executable there is
`nflstreet-actor-update-follow-loop-20261007.exe`, SHA-256
`5683663651af8927a90b1f2f4c9c757c3ef0de90d99012b82c86f86a93e1ae2d`.
It accepts a `P:` sequence of other-actor positions for this bounded branch.
The existing `T:` mode still matched its prior three-update Stewart trace.
Replay requires neither Dolphin nor the game image once the initial states
and input sequence are supplied.

The [integrated paired loop](paired_actor_loop_replay.c) now runs both
actor runtimes and the target composer in **one Windows executable**. It
starts with each actor's first captured state, advances Stewart then
Thomas on each frame, and reads Thomas's resulting position for the next
Stewart target. The verifier's `--paired-executable` mode checks its
target bytes, actor blocks, and movement blocks against both private
three-frame traces. Both runs passed. The build is
`nflstreet-paired-actor-loop-20261007.exe` on W:, SHA-256
`a6aaa02bae60a57e47fdb09b04bdacb98a282aba1323998297ed89aac3a6166a`.
The [build script](../build_windows_paired_actor_loop_replay.sh) keeps this
game-specific runtime separate from generic Daywater.

The later `nflstreet-paired-actor-handle-loop-20261007.exe` build (SHA-256
`a7bab8921812efb98f1a2e519af5d899394ff3bd7f85c6838d6c9b5a34f1814f`)
also decodes the [observed selection handle](../actor_target_input/README.md#handle-to-roster-slot)
inside native C before following Thomas. Its inputs include the handle
from a live choice trace and the observed roster count. The verifier's
`--choice-trace` mode checked both right and forward three-frame runs;
every target, actor block, and movement block still matched. This checks
the recorded choice, while the source of the manager's changing index
remains outside the native loop.

The [selector verifier](../actor_target_input/verify_target_selection.py)
also checked three consecutive `0x8008D764` → `0x8008D864` handoffs for
each input. The hash-checked DOL calls the selector at `0x8008DA60`, and
the output reaches the input function at `0x8008DA6C`. All six handoffs
selected Thomas and returned his position plus `(0, -2, 0)`. The right
and forward private traces have SHA-256
`57f7daf97166dd2e055bdfffb30f3439dbf08f0b6bf87aff068bc675779af938`
and `628cace966732ab07a9fbe990a5a19e8f655e9b7cadd8490346f7bcfcfaf4f09`.

**Reference:** NFL Street GameCube GNNE69 revision 0, ISO SHA-1
`813c4511ba3135059a13de424555647e8252254b`, DOL SHA-1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`, Dolphin 2603a,
saved-match state SHA-256
`ea3d1fa29797d1521b2ab83aee866c190ed092865b7739ceb537d15aba5e7f32`.

**Confidence:** `verified` for these exact two three-frame native replays,
the six selector handoffs, and their observed update order. The traces
begin from game-populated actor states. Selection of Thomas, actor
eligibility in later frames, other selector branches, and the full game
loop remain unmodeled. This does not yet make the game independently
playable.

The [snapshot-start replay](snapshot_start.md) now loads both actor states
and the selected handle directly from one private MEM1 image captured at
Stewart's preparation entry. Its Windows output matches a fresh
three-frame live run without trace-supplied initial actor blocks.
