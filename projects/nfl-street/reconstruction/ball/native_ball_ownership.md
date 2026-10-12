# Native ball assignment and visible ownership — 2026-10-09

## Independent scene

Subsequent [airborne inspection](native_ball_flight.md) optionally advances the
detached mesh through a checked no-spin branch. The fixed release mode below
remains available.

The optional `--ownership-inspection` mode makes the checked ball record choose
which of the two native reference players carries the visible GameBall. Current
world transforms and decoded compact left-wrist poses drive its position and
orientation. It runs without Dolphin, the original module or an ISO.

**B is a host inspection control**: player 1 → player 2 → release → player 1.
It is not an original controller mapping or a reconstructed pass/catch action.
R resets movement and queues assignment to player 1. Existing WASD/arrows,
IJKL, V and C controls remain available. Existing launchers are unchanged;
the separate private launcher is on W: under
`Daywater/research-temp/nflstreet-ball-ownership-20261009/`.

Release clears the current association through the bounded original release
service. The renderer retains the last posed mesh while detached. **It does
not simulate flight.** Reassignment resumes animated attachment. Ownership
events are supplied by the inspection host, not natural contact or game rules.

## Complete original assignment

`ball_ownership.c` reconstructs `800BE2B4`, accepting caller-owned identity,
actor flags, context words, clock delta and attachment inputs. No raw RAM
reader or gameplay policy is added to Daywater.

The original sequence clears actor flag bit `08000000`, clears ball history
words CC/C8, writes B4/B8, transitions to opaque state 1 with argument 0,
queries/caches a context word, optionally stores the actor header when that
word equals 3, clears D0, runs the complete attached update with delta 0 and
zeros the velocity triple at 54. The original writes the new owner **before**
old-state exit callbacks. Consequently an old-state-1 exit writes the newly
assigned header to CC; reordering these operations changes the result.

Unsupported inputs reject atomically, preserving the record, actor flags,
context and output pose. Supported old states are 0,1,2,3,5,6,8,9; this path
enters 1. Other entry callbacks and original upstream eligibility remain open.

## Evidence and confidence

NFL Street GameCube GNNE69 revision 0; DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Source snapshots are
position-neutral14/15/16 from Dolphin2603a. Confidence: **verified bounded
assignment effects**, using controlled calls into the untouched original
module; not verification of fresh full-match possession gameplay.

| Field | Type / size / byte order | Observed effect and readers/writers |
|---|---|---|
| Actor +0 | uint32 / 4 / BE | Class-1 headers `00000101`, `00010101`; supplied to `800BE2B4` and association lookup |
| Actor +0C | uint32 / 4 / BE | `800BE2B4` clears bit `08000000`; flag semantics remain unnamed |
| Ball +B4 / +B8 | uint32 / 4 each / BE | Assignment copies header to both; release clears B4; attached update resolves B4 |
| Ball +C8 / +CC / +D0 | uint32 / 4 each / BE | Assignment clears them, with CC potentially rewritten by state-1 exit in `800C25FC` |
| `803A1D48` | pointer / 4 / BE | At reference r13 `803A9A40` minus7CF8, points to `803CD9A0` in captured context; `8002E4F8` reads its uint32 value (3 captured) |
| `803A1D4C` | uint32 / 4 / BE | `8002E4F8` caches the queried word here |
| `803A3040` | uint32 / 4 / BE | `8014629C` stores the assigned actor header when query equals 3 |

These are snapshot addresses and measured effects, not universal addresses or
named game-mode enums. Actor/model pointers, attachment and complete ball
field metadata are documented in [README.md](README.md).

`native_assets/ball_ownership_original_check.c` executes the full original
wrapper without patching it. Each snapshot checks 192 assignments: two real
actors/selectors, selector sweep including absent selector, translated/scaled
roots, changed local offsets, eight old states, supplied context words
0/2/3, null/ordinary/wrapping clock cases. **All 576 complete 244-byte records,
actor flags, context effects and visual-model poses match exactly.** Attachment
positions/quaternions and ordinary render bases are checked too. Malformed
header/selector rejection preserves all native outputs.

## Actual Windows checks

| Check | Result |
|---|---|
| Direction/set ownership | 528 ticks; all eight direction checks pass; four set changes; six assignments, two releases; 211/160 attached updates; 162 frozen-record checks; 529 ball layers, 16,753 shaded samples |
| Paired contact ownership | 200 ticks; two contacts complete; six assignments, two releases; 75/64 attached updates; 66 frozen-record checks; 201 ball layers, 7,115 shaded samples |
| Existing fixed attachment | 528-tick direction/set run passes; all 12 BMP captures remain byte identical to the preceding visible-ball build |

The existing scripted reset contributes one of the six assignments. These
integration checks establish the native inspection behavior, not original
frame/pixel equivalence or automatic possession during tackles.

The native host supplies context value 3, reference identity forms, left wrist
24, NAP3 model inputs and 1/60 attached-update time. It copies current position
to previous position before each update. Flags/context outputs are separate
inspection state, not replacements for the movement actor or full original
global services. Compact root/extra-bone scheduling remains provisional.

Next required gameplay work: original release/launch entry callbacks and
free-flight advancement, snap/pass/catch eligibility and event routing, natural
contacts, roster assembly and fresh match startup. Some original callers pass
state 5 to release; its entry side effects are not reconstructed by this mode,
which supplies opaque state 0. No flight meaning is assigned to either number.

Private logs, binaries, capture hashes and verification manifest remain on W:.
The untouched original DLL and runner source are hash checked after testing.
