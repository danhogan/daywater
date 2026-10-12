# Native ball observation and flight-plane sampling

This isolated NFL Street module supplies bounded ball services for the native
runtime. [Visible rendering](native_ball_render.md) and
[assignment-controlled attachment](native_ball_ownership.md) now work in the
independent inspection scene. [Bounded airborne advancement](native_ball_flight.md)
now moves the released mesh with supplied force/no-spin context. Original
launch/catch/bounce policy and broader free-flight branches remain unfinished.

`ball_state.c` consumes supplied pool bytes or a semantic trajectory object.
It has no raw RAM reader, Dolphin dependency or original module dependency.
It reconstructs active-record selection, position observation, an original
render-position choice and the original XY/time sampler for a target plane.
Unknown state values remain opaque; no enum meanings are assigned.

## Original evidence

Game: NFL Street, GameCube GNNE69 revision 0. DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Dolphin build of source captures:
2603a. Offline original-module oracle; no new live integration claim.

The named `Balls` registration in `0x800BDD14` and `ball` tag in `0x800BE848`
identify this subsystem. `0x800BDDC4` selects the active record, `0x800BE36C`
selects by index, and `0x800BDE3C` searches its first identity word.

| Field | Type, size, byte order | Original evidence and observed values | Confidence |
| --- | --- | --- | --- |
| Pool root | BE pointer, 4 bytes; reference r13 `0x803A9A40` minus `0x73D0`, slot `0x803A2670` | Root `0x803B6F00` in three snapshots; not a universal runtime address | verified for these snapshots |
| Pool count / active index | uint8, 1 byte each, pool +0 / +1 | 1 / 0; active record = root +8 + index *`0xF4`, original `0x800BDDC4` matches | verified selector, multi-record lifecycle unexercised |
| Position | float32[3], 12 bytes BE, active record +`0x0C` | Original `0x800BE608` copies all three words; words `BFD39B58,C20E84E9,3F6B4C4D` in initial tested snapshot | verified accessor; game-wide coordinate semantics not newly verified |
| State | opaque uint32, 4 bytes BE, active record +`0xC4` | Original `0x800BEC50` returns 0 in tested snapshots; enum meanings unknown | verified accessor, unknown semantics |
| Flags | opaque uint32, 4 bytes BE, active record +8 | `0x800BDD64` clears/sets bit 0 for active records | probable flags role from static code; native lifecycle not exercised |

The anchor routine `0x800BE678` starts with record position, follows record +4
then pointee +`0x20`, and compares component 1 against two triples at +`0x10`
and +`0x20`. It selects a whole triple on each strict-less comparison, then adds
DOL `0x80251FB4` (`0x3DC730F6`) to component 1. The native API accepts those
triples directly. Their detailed model/bounds meaning remains candidate.

## Flight-plane sampler

Original `0x800C11DC` uses record position +`0x0C`, three components +`0x54`,
and two components +`0x70` to solve a time and sample the first two coordinates.
The velocity/acceleration interpretation is **probable** from the original
quadratic arithmetic and its callers, not a verified launch-state model.
Position and these component arrays are float32, big-endian: +0x54 spans
12 bytes and +0x70 spans 8 bytes. Their numerical use matches the sampler;
gameplay meanings and units retain probable confidence. Units, target-plane
selection, ownership and bounce/catch scheduling remain unresolved. `0x800C1300` calls it using an original global target, storing time
at +`0x44`, XY at +`0x48`, and the target at +`0x50`.

Native sampling preserves the PowerPC single-precision fused operations,
clamps negative discriminants, uses the original constants at `0x80252174`
through `0x80252184`, and returns current XY when selected time is nonpositive.
For positive time it computes acceleration * (time * (time * 0.5)), then a
fused velocity contribution, then position. No substitute physical constants
or throw/catch rules are introduced.

## Verification procedure

`native_assets/ball_original_check.c` loads each of the three saved MEM1
snapshots, resolves the original active record, executes original accessors,
and compares native outputs word-for-word. It checks one original anchor
input plus three controlled branches selecting current/first/second. It also
checks 12 plane samples per snapshot: original fields at four target planes,
then controlled positive/negative third-component velocity and nonzero XY
velocity/acceleration. All time/XY words must match.

**Verified within these original calls:** three active-record/position/state
comparisons, 12 anchor comparisons and 36 plane samples. These numerical
comparisons do not establish a working standalone ball simulation.

Build with `build_windows_direction_checks.sh`. Inputs, original module,
private trajectories, logs and hashes remain on W: in
`nflstreet-directions-20261009`. Original DLL/source stay unchanged.

## Actor association lookup — 2026-10-09

The native observation now exposes `associated_actor_handle`, and
`nflstreet_ball_find_actor` finds the first record associated with a supplied
valid actor header. It operates on supplied pool bytes and opaque handles;
there is no runtime RAM reader or pointer resolution.

**Verified numerical association:** record+B4 is a uint32 actor handle, four
bytes, BE. It contains `00010101` in position-neutral-14/15/16 from Dolphin
2603a. Original `800BE380` resolves it through `80020944`; `800BE4F8` scans
records in pool order for the associated actor. The resolver requires low
byte 1 and uses only the two following bytes for actor lookup; the top byte
is ignored. This is verified with actual original execution. Caller must supply
an existing valid actor header, not an arbitrary invented roster identity.

The new oracle runs the actual record plus seven controlled pool layouts for
two real actors in each of three snapshots: 48 original lookup results match.
It covers detached/empty pools, first-match duplicates, reversed record order,
high-bit aliases and invalid record tag bytes. For every successful result,
original `800BE380` returns the same actor. Truncated pools and invalid query
headers reject without changing outputs. Pool records retain stride F4 and
start at pool+8. Game revision/hash and reference root are documented above.
These are controlled offline module checks, not new live gameplay captures.

**Probable ownership role:** source `800BE2B4` writes the supplied actor header
to B4 and B8, clears C8/CC links, and updates state/pose; `800BE204` and
`800BE264` clear B4. Full attachment is not a single handle assignment:
`800C25FC` runs state exit/entry callbacks, and `800C0ECC` obtains actor pose,
rotation and model attachment information. Native attach/release, snap/pass/catch,
flight/bounce/rendering and possession-driven animation selection remain open.
No gameplay state enum meanings have been assigned.

Build `ball-association-original-check.exe` with `build_windows_contact_player.sh`.
Its arguments are the original DLL and one saved MEM1 snapshot. New logs and
hashes stay private in `W:\Daywater\research-temp\nflstreet-visible-contact-20261009`.

## Native state callbacks and release join — 2026-10-09

`ball_lifecycle.c` reconstructs bounded original state/history dispatch and both
release wrappers. It runs entirely on an owned 244-byte record. There is no
original-module, Dolphin, snapshot, pointer-resolution or game image dependency
in these native APIs. The original module is used only by the research oracle.

**Verified numerical lifecycle, bounded coverage:** original `800C25FC` writes
new controller state and argument, retains previous state/argument, then calls
old exit and new entry. The audited callback table starts at `8026F798`, stride
28. Supported old states are 0,1,2,3,5,6,8,9 and new states 0,1,2,3. Other states
reject atomically because their callbacks require additional game services.
These numbers stay opaque; this does not identify pass, snap or catch enums.

| Field | Type/size/byte order | Writers and tested interpretation | Confidence |
| --- | --- | --- | --- |
| Controller state / argument | uint32 each, 4 bytes BE, +A4 / +A8 | `800C25FC`, initial values 1 / 0 in all three captures; controlled entries 0..3 and argument `12345678` | verified bounded dispatch |
| Previous state / argument | uint32 each, 4 bytes BE, +AC / +B0 | Same dispatch; preserved before exit/entry callbacks | verified bounded history |
| Exit clock word | uint32, 4 bytes BE, +C0 | Exit `800C1A50` stores `801EE7F8` modular difference; tests null clock 0, ordinary 82 and wraparound 48 | verified numeric callback; gameplay units unresolved |
| Exit association history | uint32 actor handle, 4 bytes BE, +CC | Same exit copies current +B4 before release clears it; +BC is cleared | verified ordering; football role probable |
| Entry marker / flags | uint32 each, 4 bytes BE, +C4 / +8 | Entry `800C1748` writes marker 1, clears +BC and flag bits 8/20; exits 6/8 clear +E0 and bit 8 | verified supported callbacks; flag names unresolved |

`800BE204` and `800BE264` both forward the caller's state and argument registers
unchanged to dispatch. The decompiler omits these arguments; original PPC
confirms them. Both zero the +7C three-component words and clear +B4 after
callbacks. The first also zeros +54 components; the second retains them. Neither
clears +B8 or +CC beyond its state callbacks. Thus release preserves association
history instead of erasing every actor link. Original constants `80251FA8`,
`80251FAC` and callback `802521C0` are all positive-zero words in this revision.

`nflstreet_ball_read_trajectory` converts the owned record's exact position,
+54 and +70 fields to the semantic trajectory object consumed by the existing
native plane sampler. It rejects truncated/nonfinite inputs atomically. This
connects release, association lookup and numeric trajectory sampling; it does
**not** implement frame-by-frame flight, collision or launch selection.

### Original verification

`native_assets/ball_lifecycle_original_check.c` checks the audited callback table
and constants before execution. In each saved position-neutral-14/15/16 context
from Dolphin 2603a it executes 288 controlled dispatch/release combinations
(8 old states × 4 entries × 3 routines × 3 clock conditions), plus one release
of the actual captured record. Every byte of each complete 244-byte output must
match original execution, not a generated expected record. Each controlled
result joins the native association finder and original `800BE4F8`, then samples
two target planes with native and original `800C11DC` time/XY words compared.

Totals: **867 complete ball records, 864 association joins and 1,728 plane
samples match exactly**. Unsupported callbacks, invalid release mode,
truncation and nonfinite trajectory input preserve caller outputs. These are
controlled offline original-module comparisons, not full original gameplay
or fresh live captures. The null/ordinary/wrapping clocks are supplied service
inputs; native clock production is unfinished.

Build the oracle with `build_windows_contact_player.sh`. Inputs, build, logs and
`verification.json` stay on W: under `nflstreet-ball-lifecycle-20261009`.
The unchanged original DLL SHA1 is `dd521cc38b0db6bca98a0bb261570a2be5e6e0ed`;
baseline runner SHA256 is
`3f9e3a3eb4b5728d9036e9552c01e8c9dc084a1f53805fef95dcf3b1b86f73ea`.
Only disposable RAM copies are modified; no DLL/source restoration is needed.

### Attachment still needed

`800BE2B4` binds B4/B8, clears C8/CC, clears actor+0C bit08000000,
selects controller state1, conditionally updates a game-global actor identity,
then calls `800C0ECC` and zeros +54. The full attachment is unfinished.
`80020C2C` gets a signed bone selector from actor model+32 (confirmed in PPC),
then composes actor+280, the selected 64-byte cluster matrix, model+40 translation
and model+50/54/58 rotation. `800C0ECC` applies two original quarter-turn
quaternions, normalizes, and also obtains a model attachment transform through
`800D4BF4`. Existing native cluster posing is available, but this full world
transform/quaternion/model-service join still needs reconstruction and original
comparison. The native viewer therefore gains no possession controls from
this milestone. Ball mesh/rendering, automatic snap/pass/catch, natural contacts
and fresh match startup remain unfinished.

## Attachment position and compact skeleton boundary — 2026-10-09

`ball_attachment.c` reconstructs the position part of `80020C54/80020C2C`.
It accepts an owned actor world matrix, owned attachment matrices, selector
and local offset; no original pointers, module or snapshot are runtime inputs.
It preserves original paired-single fused concatenation and scalar fused
translation ordering. Model rotations happen after that translation and do
not alter its position. Selector -1 produces the original zero position;
invalid ranges, nonfinite values and overflowing arithmetic reject atomically.
Orientation, ball-model transform and attachment lifecycle are still open.

**Verified bounded numerical position:** in each saved context, the oracle calls
actual original `80020C2C` for both real actors, comparing all three float words.
It uses each actual selector plus a bounded sweep from -1 through that selector,
with original roots/offsets and controlled translated roots, changed offsets
and scaled roots. It then associates the original ball with that actor and
calls complete `800C0ECC`, checking the record's three position words against
the same native result. Totals: **576 direct positions and 576 associated-ball
position joins match exactly** across position-neutral-14/15/16. This checks
original routing and position math, not the entire ball/quaternion/model output,
fresh live gameplay or a reconstructed native hand-matrix producer.

| Input field | Type/size/byte order | Original evidence / observed values | Confidence |
| --- | --- | --- | --- |
| Actor world matrix | float32[16], 64 bytes BE, actor+280; native API consumes first 12 | `80020C54` copies it into the original matrix stack; actual and controlled roots compared | verified position input |
| Attachment selector | int16, 2 bytes BE, actor model+32 | PPC wrapper `80020C2C` reads it; actors `803C04EC` and `803C19C0` use 18 and 24 | verified selector, animation-dependent choice not native |
| Attachment matrix array | BE pointer, 4 bytes, actor model+60; each matrix 64 bytes | Read by `80020C54`; native API takes owned matrices. Range bounded by source SKL1 bone count | verified reader; bounded native producer added below |
| Local offset | float32[3], 12 bytes BE, model+40 | Source triples `(-.09375,.1015625,-.0517578125)` and `(.1181640625,.0478515625,-.017578125)`; controlled offsets compared | verified positional use; appearance-dependent values supplied |
| Compact skeleton | BE pointer, 4 bytes, model+64 | Both model pointers resolve to SKL1 at `80B96C40` in context14; all 29 translation/parent rows equal private animdata/00095.bin | verified for these models; fresh roster loading unfinished |

### Do not reuse the render cluster index directly

The original attachment matrices use the **29-bone compact animation skeleton**.
Its named entries 18 and 24 are `rwrist` and `lwrist`. The existing native body
renderer uses a different **128-row TMdl Clst palette** with intermediate weighted
clusters. For example, row24 in that palette is a weighted `lhip`, not `lwrist`.
The reference body's full-weight wrist clusters are 97 and 127, but selecting
those alone does not establish that their ancestor transforms equal the compact
bone transforms. A direct numeric-index reuse would attach the ball incorrectly.

Source `800D4E0C` allocates model+74 from SKL1 count*64 and binds model+60 to it.
`8015E9C8`, called through `800D4CBC`, computes compact pose angles and chooses
selector18/24 from source animation flags and model+20. Those selector branches
are **probable from static original code**, not yet verified native gameplay
selection. The subsequent milestone below adds native compact skeleton matrix evaluation.
Original matrix-to-Euler and quaternion composition/normalization must still
be connected before claiming a fully
native animated attachment. No attachment point was added to the visible viewer
using the wrong render palette.

The oracle derives its matrix bounds from the actual source SKL1 count. An early
experimental sweep beyond that count encountered nonfinite unrelated data;
it was discarded, and no out-of-range rows contribute to verified results.
Build `ball-attachment-original-check.exe` with `build_windows_contact_player.sh`.
Private build, logs and hashes remain in W: `nflstreet-ball-attachment-20261009`.
Original DLL and baseline source remain unchanged. Existing visible movement,
contact inspection, launchers and generic Daywater behavior are preserved.

## Native compact bone matrices and animated wrist join — 2026-10-09

`native_assets/compact_pose.c` implements the compact SKL1 evaluator
`801EAE1C`. It loads the 16-byte translation/parent rows directly from the
private file, consumes owned signed-short compact pose angles, and composes
29 bone matrices independently. The root matrix is supplied by the host;
child parent selectors come from the signed halfword at row+12. A root parent
of -1 and prior-parent ordering are required. The native result uses affine
3x4 matrices; the original storage stride is 64 bytes. Unused fourth-row
storage is not compared or assigned game meaning.

Unlike the existing approximate render-cluster evaluator, this compact path
uses the original initialized six-coefficient polynomial at `8039E100`,
paired-single rounding, fused operations and wrapped-short quarter-turn cosine.
The coefficient words are `40490FDB C0A55DE8 402335E5 BF196969 3DA83C1D BBF183AD`,
confirmed against the original snapshots and evaluator. `801888F0` initializes
that table. Local rotation construction follows original PPC, and concatenation
uses `801FFDF0` ordering. It does not substitute host sine/cosine or interpolate
weighted render-cluster angles.

### Original comparison and confidence

**Verified bounded matrix evaluation and attachment join:** the oracle executes
actual `801EAE1C` with the original loaded SKL1, actual captured compact angles
from model+5C, plus controlled perturbed angles and translated/rotated/scaled
root matrices. Scratch descriptor, output and matrix-stack addresses are
private harness allocations, not new live memory findings. It bypasses only
`801F0BD0` cache flushing after the evaluator; no pose/math branch is replaced.
The original evaluator calculates its own matrix output, which is compared
against every native float word, not a copied expected native result.

Across two actors × 16 combinations × 29 bones × three saved contexts,
**2,784 complete compact matrices (33,408 float words) match exactly**.
For each combination, both wrist selections18/24 join the native attachment
position helper. Original `80020C2C` receives the independently original-computed
palette: **192 joined wrist positions match exactly**. This is controlled
original-module execution, not reconstructed full original animation scheduling
or a fresh live-game check. Captured angles are oracle inputs, never a runtime
module dependency. Nonfinite roots, invalid parents and truncated skeletons
reject without changing output matrices/count.

### Standalone viewer connection

Optional `live-player --attachment-profile <private-NAP1.bin>` evaluates the
compact palette from the viewer's **currently decoded native animation pose**,
then calculates both wrist attachment points in bone and camera space. It runs
for each player pose, including native locomotion and finite contact playback;
it does not replay original matrices. The 24-byte NAP1 profile supplies the two
checked selectors and original local offset. It is private context data on W:,
not a public asset or an automatically selected possession role. Diagnostics
require finite points and observe bone-space changes, so root translation alone
does not satisfy their animated-joint check. Rendered pixels and player behavior
are unchanged. This is an internal hand-position connection, not a visible ball.

The viewer currently copies its decoded compact angles and zeros the absent
extra head components; it supplies an identity compact root before the existing
inspection camera/root transform. Original `8015EC30` root-angle and extra-bone
routing, model root translation/heading, automatic selector choices and complete
animation-context producers still need reconstruction. Therefore the exact
service comparisons do not make the viewer's entire pose scheduling identical
to original gameplay. Ball orientation is reconstructed below; model attachment, ownership lifecycle
application, mesh/rendering, snap/pass/catch and fresh match startup remain open.

Build the viewer with `build_windows_live_player.sh`; build the matrix oracle
with `build_windows_contact_player.sh`. Its arguments are original DLL, MEM1,
and SKL1 file. All builds, NAP1 data, captures, logs and verification hashes stay
private on W: under `nflstreet-compact-pose-20261009`. Neither the original DLL
nor baseline runner source is modified. Existing launchers remain on their
preceding builds until a visible ball milestone is ready.

**Actual standalone Windows checks:** contact inspection passes 200 ticks with
403 compact poses, two completed contacts and changing bone-space wrist points.
Eight-direction movement passes 528 ticks with 1,059 compact poses; both actors
start, brake and stop in all directions. A separate direction/set-switch run
also passes, including four supported set changes. All camera-space attachment
outputs are finite. Existing eight captured player/movement checkpoints still
match. Fourteen contact captures and twelve direction/set captures are byte
identical to their corresponding previous baselines. Nine additional direction
captures match with diagnostics enabled versus disabled under identical inputs.
A final 60-tick ordinary inspection check also preserves its five captures.
An initial diagnostic profile lifetime bug was fixed before these successful
checks; original source/DLL and existing launchers were never altered.

NAP1 format: ASCII magic `NAP1`, two BE uint32 selectors18/24, then three BE
float32 local offset words. The private test profile uses actor2's source model
local offset from position-neutral-14; it is an inspection input shared by both
reference instances, not a reconstructed roster-specific attachment profile.


## Exact attachment orientation (2026-10-09)

**Verified bounded service:** `ball_orientation.c` reconstructs the quaternion
output of `80020C2C` (wrapper over `80020C54`) and the two quarter-turn products
and normalizations used by `800C0ECC`. The actor service concatenates the world
and compact bone matrices, applies source model Z/Y/X rotations, extracts Euler
angles, then evaluates the original 24-bit-angle quaternion polynomial. The
ball service multiplies by an X minus-quarter-turn, normalizes, multiplies by a
Z minus-quarter-turn, then normalizes again. Quaternion component order is
X/Y/Z/W. Selector -1 produces zero position and identity actor quaternion.

Float operation order matters: `802165C8` returns a **float** square root. Using
a double root before the reciprocal caused a one-bit discrepancy. The native
code uses `sqrtf`, explicit `fmaf` and disabled implicit contraction, preserving
both normalizations and original sine/atan polynomial rounding. This precision
correction was established by comparing the intermediate original products and
normalized quaternions, then rechecking final output.

The oracle calls the untouched original module with real captured actors,
compact matrices and model rotations. It sweeps selectors -1 through each
actor's captured selector (18 or 24), repeats the actual selection, and varies
root translation, local offsets and root scale. Across three saved contexts,
**576 complete actor attachment poses and 576 associated-ball position/rotation
joins match every float word exactly**. The latter executes full original
`800C0ECC`; only its position/quaternion output is reconstructed and compared.
No original math or model-service branch is patched. Invalid inputs preserve
native outputs. These are controlled original-module comparisons using saved
matches, not independent full-match gameplay or fresh live-game observations.

### Orientation field evidence

- Game: NFL Street, GameCube GNNE69 revision0; DOL SHA1
  `46e3de854998a0f66c9be8c72a1164d22cd29eb4`.
- Capture build: Dolphin2603a; MEM1 snapshots position-neutral14/15/16.
- Pointer chain: actor `803C04EC` or `803C19C0` → BE uint32 pointer at actor+4
  → model+50/+54/+58. Each rotation is a 4-byte BE int32 storage word interpreted
  modulo 24bits by the original angle routines. The fields route as X/Y/Z to
  `80189B24`, which applies Z/Y/X matrix operations.
- Context14 observed model rotation words, X/Y/Z: actor1
  `(11610368,16209664,15275264)`; actor2 `(9530290,590463,1512601)`.
- Quaternion output: association-resolved ball record+18, four BE float32 words,
  16bytes, X/Y/Z/W. Writer `800C0ECC`, actor producer `80020C54`, products
  `801A2E54`, normalization `801A2D20`/`80188EAC`. Readers and runtime model
  producers are not fully mapped; no automatic-selection meaning is inferred.
- Procedure: resolve actual pointers in each snapshot, execute original actor
  and associated-ball stages, independently compute native results and compare
  exact words. Confidence: **verified within this controlled service scope**.
  The tested rotations are captured inputs, not a reconstructed producer.

### Native connection and remaining work

Optional `--attachment-profile` now also accepts **NAP2**: the previous NAP1
layout with magic `NAP2`, followed by three BE int32 model rotation words at
byte24, total36bytes. Existing NAP1 remains accepted. The private NAP2 fixture
uses actor2's context14 rotation/offset and the two checked wrist selectors.
Each current native animation pose calculates both hand positions, actor
quaternions and corrected ball quaternions, checking finite/unit-length output.
These diagnostics use neither the original DLL nor captured compact matrices.
They run in bone space; original root/head scheduling and roster-specific
profiles remain provisional. The correction makes the ball's orientation
service available to the future attachment renderer, without asserting the
inspection profiles represent live possession.

Actual Windows checks pass: 200 contact ticks, 403 compact poses and **806
orientation calculations**, with both inspection contacts completing; 528
direction/set ticks, 1,059 compact poses and **2,118 orientation calculations**,
including starts/braking/stops in all eight directions and four set changes.
All 14 contact and 12 direction/set captures match the preceding baseline.
The old NAP1 ordinary inspection also passes and retains all five captures.

Model attachment and velocity update are reconstructed in the subsequent
section. Ball mesh rendering, world/root scheduling, ownership application,
natural contacts, snap/pass/catch and fresh match startup remain open. A visible ball is not rendered yet. Existing user launchers
remain on their prior builds. Code and evidence are isolated in this project;
private builds/profile/captures/logs/hashes are on W: under
`nflstreet-ball-orientation-20261009/verification.json`. Original DLL and runner
source hashes remain unchanged.


## Complete attached-ball update and visual-model pose (2026-10-09)

**Verified bounded update:** `nflstreet_ball_update_attached` now joins the actor
attachment pose, corrected ball quaternion, visual-model attachment and velocity
calculation of original `800C0ECC`, after the caller resolves its association.
It updates an owned 244-byte ball record and returns an owned visual-model pose.
It takes structured transforms and a selected compact bone rather than raw RAM
addresses. State/association choices remain external; it does not create a
possession, release, launch or catch event.

`800D4BF4` starts from **model+3AC**, scales its nine basis coefficients by
**model+18**, concatenates the compact bone, applies the local translation and
Z/Y/X rotations, then extracts position/quaternion. It does not use the actor's
world matrix or the corrected ball quaternion. These two transform outputs must
remain distinct. The native service reuses the checked original attachment math
with the scaled model root. Selector -1 bypasses model inputs, producing zero
position and identity visual quaternion.

The ball velocity is single-precision `(new_position - supplied_previous)`;
for a nonzero delta, the original rounds `1/delta` to float and multiplies the
three components. For delta0, it retains the difference without division.
Previous position is **not** changed by this stage. Nonfinite inputs and invalid
bounds reject atomically, preserving both record and returned model pose.

### Original comparison and field evidence

`ball_model_original_check.c` executes the untouched original full `800C0ECC`,
including its association lookup and `800D4BF4` calls. The native service starts
from the same complete input record and independently calculates outputs.
Across position-neutral14/15/16, two actual actor selectors, bounded compact
selector sweeps including -1, and four controlled combinations of translated
roots, local offsets, model scale and delta0/1/1⁄60/-0.5:

- **576 complete 244-byte ball records match exactly**.
- **576 complete visual-model position/quaternion outputs match exactly**.
- Existing actor attachment position/quaternion checks also pass.
- Malformed delta/root rejection preserves native outputs.

The comparison includes unchanged bytes, quaternion, velocity and history input.
This is controlled original-module execution from saved matches, not a fresh
full-gameplay test. All pointer/offset findings below are verified in this
bounded execution scope, not independently complete producer/consumer maps.

| Field | Location/type | Evidence and limitations |
|---|---|---|
| Model root | actor+4 BE pointer → model+3AC, 12 BE float32 coefficients, 48bytes | Original `800D4BF4` loads it via `8018962C`; controlled translations reproduce outputs. Full producer/root scheduling remains open. |
| Model scale | same model+18, BE float32, 4bytes | `80189C54` scales the basis. Context14 actor1=1.013888955116272, actor2=1.0277777910232544; controlled0.75/1.5 also match. |
| Previous position input | ball record+28, three BE float32, 12bytes | `801DE5DC` reads it; `800C0ECC` leaves it unchanged. Its original update scheduling remains open. |
| Attached velocity | ball record+54, three BE float32, 12bytes | `801DE5DC` writes difference; `801DE18C` applies reciprocal. Exact complete-record comparisons cover zero and nonzero delta. |
| Visual-model pose | ball record+0 BE pointer → visual+4 position (12bytes), visual+6C quaternion (16bytes), BE float32 | `800D4BF4` writes it; rendering pose update `800C09D8` reads these fields. Context14 visual record resolves to80DEEE24. |

Game/build identity: NFL Street GNNE69r0, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`; saved captures from Dolphin2603a.
Actor pointers are803C04EC/803C19C0; the captured model roots/scales are supplied
inputs. Confidence: **verified for these services and controlled combinations**.

### Independent native connection

NAP3 extends the private NAP2 attachment fixture with a BE float32 scale at
byte36 and 12 BE float32 model-root coefficients at byte40 (total88bytes).
It supplies actor2's context14 model root/scale; NAP1/NAP2 remain supported.
Each current native compact animation pose now updates separate diagnostic ball
records for each reference actor and each wrist, including visual-model pose and
velocity. The host copies the preceding computed position into the next input's
previous-position field and supplies1⁄60 delta. These are **bone-space inspection
records** with supplied model roots; original world/root and previous-position
scheduling are not reconstructed. They do not mean either player possesses a
ball, and no mesh is drawn yet.

Actual Windows checks pass: contact200ticks with806 full attached-record updates
and two completed inspection contacts; direction/set528ticks with2,118 updates,
all eight directions starting/braking/stopping and four set changes. All14
contact and12 direction/set captures are unchanged. NAP2 ordinary inspection
remains compatible and retains all five captures. Existing launchers are
unchanged; original DLL/runner hashes remain intact.

### Reuse existing ball rendering evidence next

The existing `model-matches.json` from the original render capture already has
**105 unique draws** whose full original position arrays match both
`statmod/00011.bin` (`GameBall.ea3`) and `00012.bin` (`GameBallLow.ea3`). The first
is frame1 draw4224, 370 entries at model offset128, stride6; source arrays are
shared, so this does not uniquely choose between the two assets. Captured state
is one TEV stage, blending enabled, color0=18F8AF, alpha0=18F2F0.

The saved visual record has+18 pointing to a descriptor associated with
`statmod.dat`, and+1C holds11. Interpreting that as the selected mesh index is
currently **candidate** until its renderer reader is traced. The loaded candidate
TMdl at80DF11E0 has relocated directory pointers/flags, so it does **not** match
the raw unpacked file byte-for-byte; no full-file identity claim is made.
Reuse the existing FIFO/profile export and native primitive renderer to establish
primitive/UV/texture/matrix routing for a visible ball. The player renderer
currently accepts only six player asset categories, so statmod support still
needs a deliberate extension and verification.

Private executable, NAP3 fixture, logs, captures and verification hashes remain
on W: in `nflstreet-ball-model-20261009`. Build its original oracle through
`build_windows_contact_player.sh`. Natural contacts, visible possession,
snap/pass/catch and fresh match startup remain unfinished.


## Visible football milestone

The subsequent [native ball rendering](native_ball_render.md) connects the
verified transform to original GameBall geometry and CMPR texture. An optional
Windows inspection now renders one football following the first player's left
wrist through movement and contact playback. Ownership/snap/pass/catch and full
match startup remain unfinished; earlier no-ball descriptions above record the
preceding service milestones.

## Original frame caller join — 2026-10-10

The [native ball frame milestone](native_ball_frame.md) connects original history
copying, attached/detached update selection and per-frame input clearing in
800C0FE4. Across three saved contexts,2,112 complete record comparisons and
retained environment-query inputs match the untouched original caller. The
optional Windows frame inspection uses this path for both owners and detached
movement. Environment lookup, natural launch/catch/bounce and full startup remain
unfinished; this is a bounded caller join, not a complete ball simulation.
