# Second animation-set key and native supporting predicates

## Result

`variant_selection.c` now contains the original second-key decision from
`0x80062E2C`. Both calculated keys join the source hierarchy in
`nflstreet_variant_resolve_pair`; the second key no longer needs to be injected
into that join. The native code also produces the ordinary request predicate,
a bounded ordinary-callback phase predicate, the callback 1/5B additional
predicate and special lookup index.

This is an independently executable selection component, not automatic set
switching in the Windows viewer. The viewer and its launchers retain the prior
verified single-player behavior. It still needs a changing game context, a
supported set catalog, stream eligibility and transition application to use
these decisions during visible gameplay.

## Source inputs and evidence

NFL Street GameCube GNNE69 revision 0; ISO SHA1
`813c4511ba3135059a13de424555647e8252254b`, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Tests use the unchanged original
module and position-neutral14/15/16 MEM1 snapshots from Dolphin 2603a captures.
These are controlled original-stage comparisons, not fresh live gameplay runs.

| Source field / routine | Primitive, size, endian | Numerical role checked |
| --- | --- | --- |
| Actor+418 / +408 | uint32 /4 each /BE | Request state and selector; ordinary 62834 control logic |
| Actor+1C4 | float32 /4 /BE | Strict speed comparison against original 1e-7 |
| Actor+40C | uint32 handle /4 /BE | 20944 resolves only when low byte 1; controlled zero handle yields null |
| Actor+1DC / +1C0 | float32 /4 and int32 heading /4, BE | Ordinary 62950 negative comparison/speed guard and signed heading threshold |
| Context flag root, r13-7078, pointee+34 | pointer /4 /BE then uint32 /4 /BE | 80103D4C(0) returns bit 0; bounded ordinary phase consumes an owned flag word |
| Actor+21F / +21E | uint8 /1 each | Status shortcuts and separate control-byte 9 override |
| Actor+B5E / +B5D | uint8 /1 each | Lazy special/additional selection and key 22/23 branch |
| Actor+4C8 / +4D3 | uint32 /4 /BE and uint8 /1 | Special-index accessor enable test |
| Actor+4CC / +4CD | uint8 /1 each | 8004D840/8004D89C index; callback 1/5B with disabled special uses raw byte, otherwise 32 |
| DOL 8026E51C+index | uint8 /1 | Source key lookup; a private 256-byte view is supplied; tested indices 3/4 produce 47/48 |
| Actor+1A8/+1AC, focus root r13-7078 pointee+C/+10 | float32 /4 each /BE | Callback 2A focus override with strict XY comparisons |
| Global phase root r13-7CF8 pointee+0 | pointer /4 then uint32 /4 /BE | 8002E4F8 result; 80104238 compares with 3 and remaps final keys 1/2 to 43/18 |

Confidence is **verified for the tested numerical/structural roles**, with no
football meanings assigned to opaque statuses, selectors or callback tags.
The source reference r13 is 0x803A9A40 and actor is 0x803C19C0 in these snapshots;
those identities are not universal runtime addresses. APIs consume owned facts
and source bytes, with no RAM reader or original module dependency.

Original request constants are float32 at 8024F9D0 (4.5) and 8024F9D4 (1e-7).
The native 62834 control logic supports selectors 1,2,3,7 through its ordinary
path, unconditional active 4/8 when request state is nonzero, and the state 3
strict low-speed override. Resolved-target separation/distance remain supplied
geometry facts; the checks use a verified null handle for target-query branches.
Their units and target selection are not established by this milestone.

## Native predicate production

- `nflstreet_variant_request` replaces supplied 62834 results in the two-key
  comparison loop. Unsupported missing target geometry rejects atomically.
- `nflstreet_variant_phase_simple` produces the ordinary callback path of 62950:
  negative comparison with nonzero speed rejects the phase; differing actor or
  nonzero service argument returns false; a signed heading below 800000 and
  clear context bit 0 returns true. Special callbacks and required target search
  reject without changing output. This is not the complete 62950 producer.
- `nflstreet_variant_additional_simple` produces 65290's guaranteed false result
  for callback 1/5B. It does not reproduce preceding service calls or side effects.
- `nflstreet_variant_index` produces the direct 8004D840/8004D89C index rule.

The 6510C special game-state predicate remains an explicit supplied result.
Broader target/ball geometry, actor search and source vector-angle production
also remain unfinished. The nonzero 6510C vector/hysteresis branch is ported
from source but unexercised here; its confidence remains probable, and it must
not be treated as a verified gameplay path. Missing lazy facts are rejected.
Original 6510C flag side effects are not reproduced by a boolean fact.

## Two-key join

The pair resolver calculates the first key and its flag, passes that actual
flag to the second decision, resolves the hierarchy and installs both keys in
the 64-byte descriptor. The join is transactional. The source hierarchy still
supplies all clip/curve data; these are calculated selections, not expected
output checkpoints. The current normal controlled state produces 0C/13.

## Verification

Build with `build_windows_locomotion_selection.sh`.
`variant-second-original-check.exe` takes the original DLL and one MEM1 snapshot.
All assets, original RAM, binaries and diagnostics remain private on W: in
`Daywater/research-temp/nflstreet-variant-second-20261009`.

Three source contexts match:

- 1,980 request predicates across selectors 0..10, states 0..4, two service
  arguments and six speed controls, including both neighbors of the 1e-7 bound.
- 174 additional bounded ordinary-phase controls, including signed high-bit
  headings, actor relation, service argument, negative comparison and zero/nonzero
  speed. Unresolved search branches preserve the output sentinel.
- 4,632 complete second-key decisions: all 256 status bytes, both actor relations,
  flags 0/1/2 and eight targeted controls per context. The comparison loop computes
  ordinary request/phase/additional/index facts natively and checks each against
  its original producer before executing the full original 62E2C decision.
- Targeted controls verify special source-table keys 47/48, control-byte override 2,
  focus keys 13/15 with exact 9/6 boundaries, and ending remaps 18/43. The original
  phase word is changed only in each private in-memory test copy.
- 24 complete two-key curve payloads: actual 62C30 and 62E2C outputs feed actual
  hierarchy assembly/selection and 62340 installation; native 64-byte payloads
  match exactly. Native first-key decisions no longer depend on a supplied 62834
  result in this join, and the second key is computed rather than supplied.
- The existing first-key oracle passes again in snapshot 14. Missing-service and
  nonfinite-input rollback checks pass.

No new Windows rendering behavior is claimed. The private manifest records
source hashes and unchanged baselines: original DLL SHA1
`dd521cc38b0db6bca98a0bb261570a2be5e6e0ed`, runner SHA256
`3f9e3a3eb4b5728d9036e9552c01e8c9dc084a1f53805fef95dcf3b1b86f73ea`.
No original DLL/source changes or restoration were required.

Next: supply the remaining native game context and supported set catalog, then
apply checked set changes in the visible player. Other actors, contacts, ball
launch/catch/bounce gameplay and fresh match startup remain unfinished.
