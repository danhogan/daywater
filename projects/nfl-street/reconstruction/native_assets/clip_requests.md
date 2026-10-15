# Animation clip requests

**Confidence: candidate, source/disassembly traced and Windows compiled only.**
The owner reports that context 14's saved case 0 passed, then the initial
checker rejected replace case 1, actor 0. The generator fix below is compiled;
its oracle comparison and the remaining gate/mutation results are pending.
The implementing agent has not run Windows executables or the gate.

Game: NFL Street, GNNE69 revision 0, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Sources are the corresponding
lowercase-address `.txt` files under the shared cache's `decompiled/` and
`disassembly/`. Dolphin build was not supplied; this check uses the saved
MEM1 and the original recompiled module, not a live Dolphin integration.
Private build/check evidence directory:
`W:\Daywater\research-temp\nflstreet-clips-20261015`.

## API and scope

`clip_requests.c/.h` operate on big-endian RAM through a span. They require
only their own C file, existing headers and the C math library. Compile with
`-ffp-contract=off`; exact scalar operations use `gekko_float.h`.

| Native API | Original | Purpose |
| --- | --- | --- |
| `nflstreet_clip_request_replace` | `8017413C` | Replace one state-2 channel's clip, retaining elapsed phase and total duration by rescaling the new program's rate. |
| `nflstreet_clip_request_group` | `80177EF4` | Demote the current state-3 manager record, rekey conflicting groups/channels, allocate/retire records, initialize the new weight curve, call group mode 0, then transition the old group. |
| `nflstreet_clip_request_transition` | `80178780` | Select transition rules, initialize old/bridge/new curves, start a transition clip and delay the new group when a type-3 rule succeeds. |
| `nflstreet_clip_request_start` | `80173E78` / `80173FCC` | Allocate a channel, bind its bank/clip/group, initialize the program's first frame pair. Returns the original slot through `int32_t *index`, including -1 for allocation failure. |
| `nflstreet_clip_request_operand` | `801777A0` / `80176DD0` | Read the current callback's next operand. |

Each API returns success of the native accesses, independently of original
r3. Group's original r3 is always zero. Replace and transition are void in
the original. Failed accesses clear `ok`; earlier writes remain.

The manager consists of four records at `manager + 100*i + 4`, each 100
bytes; its leading word is outside these records. The channel set's `+2`
is the u16 catalog id, `+4` its u16 channel count, and `+8` a root word copied
to new channels. Channels have stride `0x7C`. Channel state is byte `+1`:
0 free, 1 disposable/ended, 2 playing. A key with bit 15 set permits
allocation to steal that channel by its weight `+30`; there is no invented
separate priority argument. Program byte `+2` supplies channel flag byte
`+7`. Group blend duration defaults to ten updates; catalog rules supply
other durations. These are literal source interpretations, pending oracle
verification, not newly verified memory fields.

Supporting routines include the bank/group lookup and resource lists,
`80177950`, `80177A64`, `80177ACC`, `80177DC8`, `801784B8`, `8017869C`,
`801786D0`, `80178D8C`, `801791A8`, `80179288`, `80179310`, the channel
allocator and program initialization/advance/scans. Rule matching keeps the
**last** match. `80178780`'s fallback flag persists across its four records;
its accesses to following records (`curve+100/+200`) are intentionally
preserved even at the end of the manager. Float operation order follows
the disassembly, including fused interpolation and the double fmod result
entering `80177374`'s first single subtraction.

## Replacement fixture correction

**Confidence: probable (saved-RAM/RNG inspection and disassembly; owner rerun
pending).** The initial context-14 generator selects actor `803B7320`, channel
index 4 (`803B79F4`) for mode 0, case 1, requesting bank 11, clip 403, key
`0073`. That saved slot has state 0 and program pointer `+4C = 0`. The
preparation forced state 2 and `+66 = FFFF`, so `801741AC..B0` passed the null
old program to `8017756C`/`801770FC`/`8017724C`; `80177274` dereferences its
first byte. The native span rejects that null access. This was a malformed
playing-channel fixture, not an unimplemented clip-selection branch.

Edited replacements now seed previously unused slots' `+4C` from a saved
playing channel of that actor (context 14 actor 0: `8087D028`, channel 3).
If no such program is saved, preparation uses the requested loaded catalog
program. Existing nonnull programs stay as captured. Random slot selection,
case counts and RNG draws are unchanged. Old rates, times, duration caching
and all replacement paths remain randomized. State-2 replacement inputs
require a readable initialized old program; raw null-program scans are not
modeled or generated. The original does not supply a null fallback.

Rejection diagnostics print all prepared GPR/f1 inputs, named replacement
arguments, the old channel state/program/duration/rate/time and target catalog
program. The same diagnostics accompany original-run failures and mismatches;
they use the prepared copy, before native writes.

## Providers and limits

Group callbacks (`r13-6718`) and program event callbacks (`r13-6734` normally,
`r13-6730` during program scans) remain providers. The group callback receives
function address, resources, key, set, channels, actor, mode in that order.
The event callback receives actor, command, key and a scanning flag. A
provider can use the start/replace APIs and consume operands. NULL groups
have no effects; NULL events return 1 without consuming operands. To reject
an unsupported provider, clear `ok`. Reentrant provider calls must use a
separate request context sharing the same RAM, so an outer scan's local
stream remains addressable through its context.

**With no group provider, group requests do not execute gameplay callbacks
and do not start ordinary locomotion clips.** Direct starts and replacements,
and the transition clip selected inside `80178780`, are modeled. In the
saved catalog 1, key `4A` calls `800635B8`, `49` calls `80066490`, `54` calls
`80065398`, `64` calls `80067FA4`, and `A0/9D/9F` call `800669B0`. Those
callbacks choose individual bank/clip ids from resources and actor state;
they are not interchangeable with group ids and are not reconstructed here.
The event side effects, callback internals, movement selector execution,
controller handling, stream/frame decoding and pose update are outside this
module. No live-app files or other existing files were changed.

The scan helpers publish a temporary stream pointer to `r13-4F58`. Set
`stack_top` to PPC r1 at the request entry when comparing an original call;
zero selects the oracle fixture's `80F46FC0`. A local shadow models that
scan stream without exposing the oracle stack through the caller's span.
This preserves the otherwise observable pointer value; it is not a native
host pointer. The normal callback stream is a real channel RAM token.

## Exact movement-handler handoff

All offsets below are from gameplay actor `g`. `8003A1F4` visits the array
address `u32[u32[r13-7C30]+0]`, u16 count `+0A`, stride `14D4`, and invokes
`800150E8(g)`. That routine indexes the function table `80244D28` by **byte
`g+20E`**. Do not derive this selector from speed or from a host animation
state. Given `actor[12]` representing `g+200..22F` and `state[21]`
representing `g+1A8..1FB` (the live movement runtime's layouts):

| Input | Representation | Use in request selection |
| --- | --- | --- |
| `+200`, `actor[0]` | f32 bits | Requested magnitude; zero tests and strength floors. |
| `+204`, `actor[1]` | u32 turn word | First requested heading. |
| `+208`, `actor[2]` | u32 turn word | Second requested heading. |
| `+20E` | `(actor[3] >> 8) & 255` | Dispatch selector. |
| `+20F` | `actor[3] & 255` | Status tested for 6/7 by handler 1/2/3. |
| `+210/+214/+218`, `actor[4..6]` | f32 bits / turn words | Selected magnitude/headings written by the handlers, then read by callbacks. |
| `+21E` | `(actor[7] >> 8) & 255` | Selected mode byte written by the handlers. |
| `+1C0`, `state[6]` | u32 turn word | Current heading, used by helper `80014750` and suppression `8001454C`. |
| `+1C4`, `state[7]` | f32 bits | Tested against `80244D58` by suppression `8001454C`. |
| `+1C8`, `state[8]` | u32 turn word | Other current heading, used by suppression `8001454C`. |
| `+1EC`, `state[17]` | f32 bits | Compared with `r13-70C0/-70BC` in direction branches. |

The remaining movement words, including positions `+1A8/+1AC`, do not
directly select a request in these handlers. External gameplay predicates
and callbacks can read more of the actor/world. `g+2/+3` are header bytes;
`+408` is a u32 compared with 4/5. `+310` points to another gameplay status
object. These extra inputs cannot be replaced by the two blocks alone.

Let `K` be `801784B8(u32[g+318])` (first state-3 manager key, else FFFF),
and `D(a,b)=min((a-b)&FFFFFF, 1000000-((a-b)&FFFFFF))` (`80188F28`).
Let `S=f32[g+200]`, `H=u32[g+204]`, `T=u32[g+208]`, `V=f32[g+1EC]`,
`L=f32[r13-70C0]`, `B=f32[r13-70BC]`, and `own` mean
`u8[g+2] == 80103CA0()`. Ordinary copy below means write
`+210=S`, `+214=H`, `+218=T`, `+21E=u8[+20E]`.
All constants in this subsection are hex unless explicitly decimal.

| `+20E` | Handler | Requested group and writes |
| --- | --- | --- |
| 0 | `80014B28` | Ordinary copy. Map K to 4A for K=21,31..37,39..3C,49,54,64,6A..6C,A0; otherwise keep K. Request only if guard G=0 and selected key differs from K. |
| 1,2,3 | `80014CD0` | Header byte +3=2: select 4A, write selected mode=1, selected headings H/H and magnitude S. Otherwise use predicate P and helpers below. Apply guard Q; request if Q=0 and key changed or helper asks to restart. |
| 4 | `80014E24` | Write +21E=+20E, +214=H, +210=S; call G; no group request. +218 remains as stored. |
| 5 | `80014E68` | Keep K only for K=31..37,39..3C,6A..6C with +408=4, or K=19,38,3D,4F with +408=5; otherwise select 4A. Ordinary copy; guard G=0 and key changed. |
| 6,7 | `80015048` | Ordinary copy. Header byte +3=1 or 2: select 64, request when K differs. Other header values use handler 0 (including its guard). |
| 8 | `80015120` | D(H,T)<2E38E3 or S=0: select 4A, mode=1, headings H/T, magnitude S. Else D<56C16C: if V>L preserve K and write mode=1,H/T,S; otherwise select A0 and set selected magnitude=max(S,divs(muls(L,80244D90),L)), mode=+20E and selected H, leaving +218 unchanged. Else if V>B preserve K with mode=1,H/T,S; otherwise select 4A, magnitude S, mode=+20E and selected H, leaving +218 unchanged. Guard Q=0 and key changed. |
| 9 | `80014FC8` | Ordinary copy; select 4A and request when K differs. No G/Q. |
| A | `80014C34` | Copy mode and magnitude; selected headings H/H. Select 4A with G=0 and key changed. |

All requests in this table call `80177EF4` with f1 from
`80244D70/74/7C/80/84/88/94` respectively (all `3F800000` in context 14).
The actual arguments are **values of pointers**:
`manager=u32[g+318]`, `set=u32[g+31C]`, `channels=u32[g+320]`, key, actor=g.
In the captures these pointers equal `g+9BC`, `g+4D8`, `g+4E4`; use the
stored pointers at the state-handler boundary.

For selectors 1/2/3, P is `8006510C(g)`, a gameplay predicate depending on
controller/team/game status, `+B5E`, `+C` flags, `+20F`, `+310` and other
world state. It is not a simple locomotion-word test. If P=0 and +20F is
6 or 7 and S=0, call handler 6/7 and return. Otherwise P=0 passes the
**same boolean** `(+20F-6 unsigned < 2)` as r4 to `80014898`; this argument
is omitted by the decompiler's call display but is explicit at `80014D74..80`.

`80014898(g, forced)` masks H/T to 24 bits; forced replaces T with C00000.
For `D(H,T)<2E38E3` or `S<=0`, choose the neutral result below. Otherwise:

- D<56C16C: if not forced and V>L, neutral. Else choose A0 when own or
  forced, otherwise 49; selected magnitude is max(S,divs(muls(L,floor),L))
  with floor `80244D64` (A0) or `80244D68` (49). Copy mode,H,T.
- 56C16C<=D<=71C71B and own: if not forced and V>B, neutral. Otherwise call
  `8005335C(1.0,g,T,2,0)`, choose 9D when `(T-H)&FFFFFF <= 800000`, else 9F.
  Selected heading +218 is `(T +/- (D-400000))&FFFFFF` (minus for 9D),
  selected +214=H, mode=+20E; magnitude has the A0 floor above.
- Other angles/ownership: if not forced and V>B, neutral. Otherwise choose
  4A and ordinary copy.

Neutral means select 4A; if T!=H call `8005335C(1.0,g,T,2,0)`; set selected
mode=1, headings H/H, magnitude S. These helper calls can modify state.

If P!=0, `80014750(g,D(H,T),&key)` chooses 4A with mode=1,H/H,S when
D<1C71C7 or S=0. Else if D<56C16C and **signed H>800000**, choose 54.
When K=54 and `80178638(manager,54)!=0`, set input T=`u32[g+1C0]` and S=0.
Request a restart when D(H,old selected +214)>400000. Copy mode,H,T,S.
Other cases select 4A and ordinary copy. After either helper, map key 21
to 4A and apply Q.

G=`80014644(g)` is zero unless header byte +3=1 and K=21. In that case
G=1 unless actor flag `+C & 4` is set or `80061158(g)!=0`; either exception
clears flag 4 and returns zero. It always clears input +20E and selected
+21E for this case. Q=`800146CC(g,key)` invokes G for this same K=21 case;
otherwise, when header +3=1 and byte `*u32[g+310]` is neither 3D nor 50,
it invokes `8001454C(g,key)`. That suppression is zero except for key=4A,
selected magnitude!=0, D(+1C0,+218)>5FFFFF and either:
D(+1C8,+214)>755555 with own, f32[+1C4]>=f32[80244D58], and
`8004F214(g)!=0`; or D(+1C8,+214)<AAAAA with own and `8004F214(g)!=0`.

`8017413C` is **not directly called by 800150E8's handlers**. Its callers
are `8006475C` (two channels of the 4A callback) and `80066490` (49 callback).
`80178780` is called by `80177EF4` with old key K, state=2. Thus interpreting
+20E as an individual clip id or directly calling replace for each movement
change would skip the original group/callback decisions.

## Integration and checks

At the existing movement-state phase (the `8003A1F4` position in the game's
update order), restore the runtime's `actor[12]` and `state[21]` into their
big-endian RAM blocks and select requests using the rules above, retaining
the named gameplay predicates. Dispatch the selected request through:

```c
nflstreet_clip_request_group(&requests, weight,
    rd32(g+0x318), rd32(g+0x31c), rd32(g+0x320), selected_key, g);
```

Here `rd32(address)` means a big-endian RAM read in the embedding app.
Supply the group/event providers before expecting new gameplay clips.
Run the existing `nflstreet_actor_channels_update` later at its original
`8003A24C` phase, with the same RAM, as documented by `actor_channels.md`.
Do not issue a fresh group request on every update: preserve the changed-key,
restart and suppression conditions above. This module alone does not
replace the live movement selector or its callbacks.

Build only (already exercised, including `-Wall -Wextra -Werror`):

```bash
bash projects/nfl-street/reconstruction/native_assets/build_windows_clip_requests_original_check.sh /mnt/w/Daywater/research-temp/nflstreet-clips-20261015/
```

Owner-run single-context original check (substitute 15/16 as needed):

```bash
/mnt/w/Daywater/research-temp/nflstreet-clips-20261015/clip-requests-original-check.exe 'W:\Daywater\research-temp\position-neutral-14.mem1' 'W:\Daywater\research-temp\nflstreet-native-windows-local\gGNNE69_recomp.dll'
```

Owner-run full gate, then mutation validation (the second command repeats
base validation before mutations):

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_clip_requests.py
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_clip_requests.py --mutants
```

The checker runs saved-as-is requests and 300 randomized preparations for
all 14 actors for each of replace/group/transition/direct start: 16,856
individually isolated original/native comparisons per context. Inputs use
saved channel ids and loaded catalogs; edits cover protected keys, channel
allocation, manager states/delays/flags, rates, blend times, program flags,
finite synthetic instruction streams and controlled transition rules of
all four types, wildcard precedence and fallback. RAM comparison excludes
only `80F38000..80F47000`; callbacks and consumed operands also compare.
The fixture provider consumes operands using original `801777A0` in the
oracle, then returns at the external callback's entry. It provides test
behavior, not reconstructed gameplay event semantics. No ARAM, GX or other
original helper is hooked. The gate requires two identical fresh processes
per context and protects baseline/input/source hashes. No expected output
digests were invented; it validates the full success record and fresh-run
equality, storing the observed digests in its private report.

`clip_requests_mutants.json` contains ten single-line edits. The optional
mutation gate compiles each in a temporary directory inside this worktree,
puts only executables/logs in the private evidence directory and requires
an actual comparison mismatch (exit 7). Build errors, unsupported fixtures
or crashes are not accepted as mutation catches. Mutation execution and
all original-module results remain pending.
