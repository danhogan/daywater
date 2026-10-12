# Native cardinal stick translation replay

`input_translation_replay.c` models the observed nonzero-cardinal branch of
GNNE69 revision 0 `0x8003DDEC`. It accepts the function's 0x20-byte input
vector and the actor's 0x30-byte input-field block at `+0x200`, then emits the
updated field block. It computes the 24-bit heading with the same native
`0x80188D98` reconstruction used by the kinematics replay, and writes the
observed magnitude, requested/command heading, and status fields. All other
bytes in the actor block are preserved.
The guarded function is also exported through [input_translation.h](input_translation.h)
for the [native actor runtime](../command_to_movement/input_vector_bridge.md).

The original DOL SHA-1 is
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. The [live trace](../input_to_movement.md)
records the tested function addresses, input vectors, and field writers.
The three settled right, forward, and left Dolphin 2603a captures match every
one of the 48 actor-field bytes after translation. In two fresh runs, a
controlled change from left heading to right or forward stepped over the
original store at `0x8003E0A4`; this replay computes the same new headings.
The latter two traces do not capture a complete post-function actor block,
so they validate only the heading for that transition. Raw traces remain
private under `W:\Daywater\research-temp\`.
Four newer [partial-strength sequences](../command_to_movement/partial_strength.md)
also matched complete 48-byte translator returns on their later calls,
including partial right and partial forward vectors.

Build the Windows x64 replay to private W: storage with
`bash projects/nfl-street/reconstruction/build_windows_input_translation_replay.sh`.
The checked executable has SHA-256
`043b3b69cda5835a540e564b075e2f47753bef5948240038e1aef50f68ec1d7b`.
Compare one settled trace with:

```bash
python projects/nfl-street/reconstruction/input_translation/verify_capture.py \
  --trace /mnt/w/Daywater/research-temp/thomas-kinematics-input-right.jsonl \
  --executable 'W:\Daywater\research-temp\nflstreet-input-translation-replay.exe' \
  --windows
```

**Confidence:** `verified` for the three original complete cardinal
transitions, the four newer partial-strength sequences, and the two
independently stepped heading changes. The replay rejects zero and diagonal
vectors. It omits the original function's game-service
calls, actor flags outside this block, zero-stick branch, and other control
states. Its exact agreement for these inputs does not establish a complete
replacement for `0x8003DDEC` or a continuous native update loop.
