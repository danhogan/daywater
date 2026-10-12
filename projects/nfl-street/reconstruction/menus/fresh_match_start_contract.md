# Fresh-match startup contract from original `801EA480`

This note records the static original call contract after the field picker.
It does not claim that a match was created or that the underlying query and
resource services have been reproduced.

## Reconstructed native orchestration — 2026-10-12

`match_start.c` now owns the `801EA480` caller sequence in ordinary C. It
copies the supplied two fourteen-word formations, requests registration and
the initial mode, follows the four setup operations and two side queries,
applies both side assignments, resolves all28 player words in order and
requests the session recipient. The final update is requested only on the
original accepted branches. Nonzero initial mode bypasses setup and player
conversion. Initial, setup, side-query, side-assignment and session errors
return immediately. Per-player query statuses are deliberately ignored,
matching the original caller. The input selection is preserved; owned output
arrays retain partial conversion if a later service fails.

**Verified controlled caller behavior:** `verify_match_start.py` builds a
strict Windows differential and runs17 cases twice in each saved context14,
15 and16 (102 cases in six fresh processes). The unchanged original module
executes `801EA480` and its registration/side-assignment wrappers. Ordered
normalized service requests, all28 output words, result status and session/
update request counts match. Success has40 service boundaries and28 player
queries; modes1 and255 bypass with three boundaries. An actual initial-query
error returns after two boundaries. Other checks cover each setup/side error,
session and update errors, and ignored player errors at opposite array ends.
The original session recipient `801E577C` is captured without execution.

The real-data case consumes the JSON produced by the actual Win32 menu loop.
`match_player_query.c` resolves the literal packed PLAY predicate
`DIGT==supplied context && DIOP==input word`, returning `DIGP` from one unique
row. Both14-word arrays resolve in the private team1 block with context1
explicitly supplied. This checks caller/data integration; the original query
engine and ownership of its side-context query are not reconstructed here.
Missing or ambiguous packed rows reject without changing the output and
without inventing an original database error code.

A separate read-only packed-table audit covered402 PLAY rows across32 team
blocks: DIOP equals DIGP in those records, and both team/key predicates are
unique. Existing team1,3 and31 formation words have matching records. These
are facts about those private base tables; they do not establish custom
career/save behavior or universal identity equivalence.

Native orchestration has no Dolphin/ModernGekko dependency. Its caller must
provide real query/resource/session owners. The current differential supplies
those service responses; status0 is not evidence of a created playable match.
Private report:
`W:\Daywater\research-temp\nflstreet-match-start-20261012\verification.json`.
Original DOL, DLL and runner source retain their protected baseline hashes.

## Caller and copied inputs

Original `8011F170` runs this path only when `8010C390` returns `1`. It calls
`801211D4`, reads the selected UI slot from `r13-0x6E04`, indexes the
DOL-backed eight-byte order at `r13-0x6E10`, and passes that selected byte to
`80027270`. It then calls `8003F7F0(0, local_side0)` and
`8003F7F0(1, local_side1)`. Each call copies exactly fourteen 32-bit words,
from `0x802A4430 + side*56` to its 56-byte local array, preserving order.
Those two array pointers are passed to `801EA480`.

The native [quick-match selection builder](quick_match_selection.c) retains
the selected field slot/key and both fourteen-word formation arrays. The
[original commit checker](field_commit_original_check.c) executes the original
`80027270` and `8003F7F0` portions and confirms the transfer at the
`801EA480` call boundary for one supplied selection on contexts 14, 15, and
16. Its `801EA480` handler is a capture boundary, not a match-start execution.

## Original query strings and loop shape

The following NUL-terminated strings were read from the mapped original DOL;
the high-byte placeholders are preserved as raw query-format bytes:

| DOL address | Original query text |
|---|---|
| `0x80268B74` | `select 'PNIG' into 0x83 from 'FNIG'\n` |
| `0x80268B98` | `select 'DIGT' into 0x82 from 'MTTS' where ('UNLS' = 0x82)\n` |
| `0x80268BD0` | `select 'DIGP' into 0x85 from 'YALP' where 'DIOP' = 0x85 and 'DIGT' = 0x85\n` |
| `0x80268B00` | `update 'NIOM' set 'ASCM' = 0x82\n` |

`801EA480` first initializes the callback table from `PTR_FUN_80295240`, then
issues the `PNIG` query. A query error returns immediately. If its output byte
is zero, it performs the resource/setup calls `801E3828(0x07000000)`,
`801E3720(0x04000000)`, `801E05A4(0x07000000, 0x03000000)`, and `801E5ECC`,
stopping on a nonzero result. The two `DIGT`/`MTTS` query calls then run with
selector arguments 0 and 1; each nonzero result also returns immediately.
`801E4EF4` and `801E4F28` follow for the two returned values.

It next iterates fourteen words from the first input array, then fourteen
words from the second, advancing each pointer by four bytes per call. The
PowerPC call frame passes the current word in `r6`, its address in `r5`, and
that side's query context in `r7` to the `DIGP`/`YALP` query format. Thus the
source evidence establishes two ordered 14-entry passes and their ABI inputs;
it does not establish additional meaning for the four-byte tags or the
placeholder fields.

After both loops it calls `801E577C` with the initial query output and both
input-array pointers. A nonzero result is returned. If the current result is
zero, it issues the final `NIOM`/`ASCM` update with argument `2` and returns
that query result. The resource helper `801E3828` and `801E3720` accept the
inner status values `0`, `0x17`, `0x15`, and `0x14` after releasing a nonzero
handle; other statuses are propagated. These are observed control-flow
conditions, not labels for error meanings.

## Completed comparison boundary and next recipient

The completed differential executes the original `801EA480` body while
supplying only its external service boundaries: the query dispatcher
`801B4510`, the resource/setup operations above, and the terminal
`801E577C` service. Feed the exact mapped query strings and the already
verified selected key and 14+14 input words, record every query argument and
output write in order, and compare immediate-return cases for each query and
setup failure. The success branch should separately verify the fourteen
calls per side and the final update. The checker must leave the query results
and match/session service explicit; it should not describe this as match
creation until the original recipient path is exercised.

**Evidence confidence:** the caller order, array lengths/copy behavior, query
string bytes, loop counts, and listed status branches are supported by the
cached original decompilation and mapped DOL. The completed commit comparison
is verified only through the `801EA480` entry boundary; all later service and
match-creation behavior remains unverified. The later controlled differential
above also verifies caller behavior through its captured session request.
The next recipient is `801E577C`, which resolves side contexts, clears source
tables, constructs session records and invokes further initialization
services. Its resource/database/callback ownership requires separate work;
these callees are not supplied as successful gameplay by the native runtime.

The ordinary-mode recipient is now reconstructed and joined to this caller
and the owned configuration resolver: see [session and pipeline](match_session.md)
and [settings](match_settings.md). The joined differential executes all three
original bodies while supplying query/table/resource/downstream responses.
It does not execute the original query engine or create a fresh match. The
next concrete object foundation is [base model initialization](../native_assets/actor_model_init.md),
which owns model/channel writes but leaves complete player appearance and
asset binding separate.

Reference provenance: NFL Street GameCube GNNE69 revision0; DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`; source RAM captures were made
with Dolphin2603a. All new comparisons are offline original-module checks,
not new live Dolphin gameplay or a full match startup comparison.
