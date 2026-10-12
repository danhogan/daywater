# Contact selection and event completion

Windows C now constructs the observed contact parameters, checks eligibility,
scans the complete supplied candidate set, selects the matching animation,
advances the random state, builds both requests, and performs the bounded
event-completion writes. It no longer receives the chosen asset or expected
request as an input. Animation-channel allocation and application remain
original-game services; this is not a complete native animation or match.

## Reference and test procedure

NFL Street, GameCube **GNNE69 revision 0**, ISO SHA1
`813c4511ba3135059a13de424555647e8252254b`, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`, Dolphin **2603a**.
All captures restore the first-down state SHA256
`e2faeb8f9481096ff87ae261c8c2943452fbc023cd27510f641808e307603d93`.
The actors are Stewart, roster slot 7 at `0x803C04EC`, and Thomas, slot 8
at `0x803C19C0`, in the roster at `0x803B7320`, stride `0x14D4`.

1. Restore independently for right and forward input. At `0x8005EEAC`,
   capture both complete actor blocks, original contact parameters, incoming
   motion source and angular-scale input. Capture the merged candidate table
   at `0x8005EF28` and **every** resolved candidate asset at `0x8005EFA8`.
   The table's pointer is a source descriptor, not the resolved paired asset.
2. Stop at the geometry predicate entry/return (`0x8005EDC0`, `0x8005EE44`),
   random choice call/return (`0x8005F230`, `0x8005F234`), request builder
   return and contact application boundaries. Capture complete actor outputs
   and manager bytes after event return.
3. Run [verify_contact_selection.py](verify_contact_selection.py) against the
   Windows replay. Native parameter production must match all 60 original
   bytes and native eligibility must admit the original matcher call. The
   native selector receives all candidate assets, geometry, descriptors,
   motion, angular scale and random state. Expected choice and output bytes
   are comparison targets only.
4. Compare selected identity, accepted candidate count, all 24 random-manager
   bytes and 256 captured generator bytes. Build both requests from the
   **native selection**, then compare all 16 bytes of each. Compare the full
   event, both flag words, link slots, request bytes, mode bytes and selected
   record field. Report every changed actor byte outside that owned output
   set as a remaining service effect.

## Selection path

The observed event is code 4, stage 2, mode 2, request selector 4. Current
descriptor starts `1A`, incoming starts `1B`. `0x8007B73C` builds parameters
with group 30, distance limit 4, gate angle `0x2AAAAA`, row-heading tolerance
`0x155555`, position tolerance approximately 0.85 and motion test enabled.
`contact_selection.c` produces these parameters natively for this branch.
It checks that record zero belongs to the current actor, and applies the
distance/heading gate, including the incoming handle-tag/`+0x408` exception.
Other event codes, descriptors and record indices are unsupported.

The original matcher scans nine entries. Each table entry contains a uint16
choice and uint16 asset key, with the key's high bit removed for resolution.
The runtime still receives the source table and all nine **resolved** assets;
bank/asset resolution is not reconstructed here. Metadata row type is
nonzero/zero at row `+2`, row heading is uint32 `+4`, and local XY is float32
at `+8`/`+0xC`. There is one incoming row per candidate in these captures.

For each candidate, native code reproduces:

- motion/type compatibility (`0x8005EC28`, motion source `0x8005EE5C`);
- four wrapped comparisons of relative row and actor headings
  (`0x8005E99C`), including actor heading offsets at `+0x12C`;
- incoming-minus-current position rotated by the negative current heading,
  then bounded against the incoming row's local XY (`0x8005EAB8`);
- the angle between those local vectors, using the captured scale 115
  multiplied by original constant `46603.37890625`.

The motion source comes from actor `+0xC10` → `+0x20` → 44-byte sample.
The observed motion/type result is zero. Its reconstructed test uses sample
`+8 < 1` and sample `+0x28 - +0x18 > 0.5`. Optional ordering/position solving
and multiple incoming actors are rejected; no captured predicate result is
substituted for native matching.

Four successive calls find no candidates. The fifth accepts one: its asset
provides keys `0x249` and `0x24A` for the current/incoming requests. The
chosen entry supplies choice 1. The type-0 generator advances
`seed = seed * 1664525 + 1013904223` modulo 2^32, normalizes and clamps using
original float constants, and chooses index zero from that single survivor.
The manager increments its call count and updates its checksum. Although
choice among multiple survivors is statically reconstructed, these runs
do not establish live behavior for multiple survivors.

**Confidence: verified** for parameter construction, the observed gate,
candidate selection/rejection and type-0 RNG transitions in these fresh
runs. General meanings of metadata keys/flags remain **candidate**.
Unexercised mirrored/dynamic-motion matching and type-2 RNG code are
**probable** from static instructions; type-1 RNG is unsupported.

## Completion fields and remaining services

All multibyte fields below are **big endian**. These are bounded raw runtime
representations; gameplay policy must not depend on these offsets.

| Field / pointer chain | Primitive / size | Observed values and operation | Original readers/writers | Confidence / limitations |
| --- | --- | --- | --- | --- |
| Actor `+0x150`, event stage `+4` | uint16 / 2 | 2→3 after successful selection; full 12-byte event compared | `0x8007C504`–`0x8007C518` | verified for code 4 |
| Both actors `+0x3F0` | four uint8 / 4 total | Current `49 02 01 00`, incoming `4A 02 01 00`, derived from requests | `0x8005FE74` | verified |
| Both actors `+0x274`, `+0x278` | handle uint32 / 4 each | First eligible empty link becomes other handle; observed `0x10101` / `0x101` | `0x800CDF50`, called twice at `0x8007B8C0`/`0x8007B8CC` | verified observed first-slot writes; occupied alternatives probable |
| Both actors `+0xC` | flags uint32 / 4 | Animation callback sets `0x200`; current actor then clears mask 5, preserving other bits | `0x80068F58`, test `0x8006816C`; `0x8007B8F8`–`0x8007B904` | verified; gameplay meaning of bit `0x200` candidate |
| Both actors `+0x20E` | uint8 / 1 | Written zero on success; observed zero→zero | `0x8007B8F0`/`0x8007B8F4` | probable for clearing a nonzero value |
| Manager `0x803B6F00` → index byte `+1` → record `+8+index*0xF4` → `+0xC8` | uint32 / 4 | Store zero; observed index 0, address `0x803B6FD0`, zero→zero | `0x800BDDC4`, `0x800BE4C0`, called at `0x8007C518` | probable for nonzero reset; **not** manager `+0xC8` |
| RNG pointer slot `0x803A9A40-0x61F8` → RNG `+0x14` table → first generator | uint32 pointers / 4 each | Type-0 seed at generator `+4`; count/checksum at RNG `+8`/`+0xC` | `0x801EE24C`, `0x80188660`, `0x80188008` | verified observed type-0 transitions |

The direct flag-writer capture observes both actors at `0x80068F58` applying
`before | 0x200`. Native completion is guarded to group 30; this does not
establish other groups' callback behavior.

Original application also changes the actor's animation-channel blocks and
animation manager state. The verifier reports their changed byte offsets
outside the owned field set rather than silently counting them as matches.
Stewart's changes include the block starting at `+0x560` and stream `+0x5AC`;
Thomas allocates a block starting at `+0x750` and stream `+0x79C`. Both have
additional animation state changes around `+0xA20` through `+0xB4B`.
Existing native stream initialization can initialize a supplied stream, but
channel allocation, bank resolution, blending and animation callbacks are
not yet joined into this replay. The completion API emits the request fields
and owns the listed event writes; it does not reproduce whole actor blocks.

## Results and artifacts

Three fresh captures match **15 matcher calls**: 12 failures and three
successful selections/completions, examining **135 resolved candidates**.
All three successes reproduce both request records, the native gate and
parameters, random transitions and the owned completion fields. Offline
rollback/eligibility checks run without Dolphin. Windows builds and all
source assets, binary inputs and diagnostic reports stay on private W:.

| Private artifact | SHA256 |
| --- | --- |
| `nflstreet-contact-complete-confirm-right-20261008.jsonl` | `b1fc8748560f0008ad04ae2ecc56fe86847124bb8c0e3feed9e4e3e4e058910d` |
| `nflstreet-contact-complete-confirm-forward-20261008.jsonl` | `a3525d4a9f0a9c603c8c0b6604b4fb1205f8cfcef3a22faa531a3936d6beb315` |
| `nflstreet-contact-held-snap-forward-20261008.jsonl` | `47f3eeaa6e991242e41ffa5ab9bc286b2c9b0453e3fd29ad9625bbd9929834fc` |
| `nflstreet-contact-replay-20261008.exe` | `f0491923a0f499a5cebd2043b7d9cb256d85a2f8f4d5e8388ecda9b1481498d9` |

The first right contact completes at dumped frame 49. The first forward run
has delayed snap and completes at frame 406; it is a separate exercised
play branch, not evidence for frame-identical input scheduling. The next
forward run holds A until the event-construction boundary (release frame 16)
and completes at frame 50. Breakpoint timing must not determine button release.
The earlier 120-second exploratory trace has an incomplete stage and is
excluded. Baseline container sources are restored after every capture; the
playable DLL is unchanged.

The older right capture lacks a full manager block at its first matcher
entry. Its initial ownership input is constructed from the pre-call record
handle list and index zero; later manager inputs use preceding captured
states. Both forward captures include complete manager entry blocks. The
reset field stays zero in all three, so nonzero reset behavior retains
static rather than live confidence. Linux and Windows offline checks pass.
Private verification reports include the remaining changed actor byte
offsets: 45 current-actor bytes and 114–115 incoming-actor bytes per success.

Build with `reconstruction/build_windows_contact_replay.sh`. Run the verifier
with `--trace`, `--dol`, `--executable`, `--windows` and a private `--work-dir`.
[contact_checks.c](contact_checks.c) checks malformed row bounds, unported RNG
and positioning rollback, gate rejection and repeated-completion rejection.
Link it with `contact_selection.c`, `pair_requests.c` and
`kinematics/kinematics_replay.c`, defining `NFLSTREET_KINEMATICS_NO_MAIN`.
