# Quaternion-kernel reconstruction status

The checks use the saved `position-neutral-14.mem1` all-RAM image and the
locally generated GNNE69 recompiled module.  Neither is part of this tree.

| Entry | State | Evidence |
|---|---|---|
| `80188BB0` | verified | `check_80188bb0_module.sh` passes all 65,536 angle words for the saved polynomial and four randomized finite coefficient sets: 327,680 exact result pairs. |
| `801A36EC` | verified | `check_801a36ec_module.sh` passes 100,000 randomized full 24-bit Euler triples.  It follows `8018889C`'s paired lanes and final `ps_sum0`; a Horner translation was not exact. |
| `80216B94` | verified (bounded caller domain) | The complete `[-pi/2, pi/2]` path used by slerp is transcribed: `802171C4` reduction plus `80216EF4`/`80216E0C` kernels and DOL IEEE words.  500,000 randomized positive calls match the original module bit-for-bit.  The large-argument MSL reducer remains outside this caller proof. |
| `802165C8` | verified (normal positive domain) | The restoring integer square-root loop, including round-to-even, passes 500,000 module calls from the normal positive range used by `80215844`.  Denormal handling remains unported. |
| `80215844` | verified (slerp cosine domain) | The two non-negative `acosf` paths and their stored-DOL coefficients pass 500,000 randomized calls in `[0,1]`. |
| `801A383C` | candidate | It now uses the reconstructed MSL sine, acos and square-root helpers.  The former `t=3e5978d9` sine discrepancy is gone; a later randomized vector reaches a remaining one-ULP final weighted lane, so the entry is not yet verified. |
| `801A340C` | unstarted | Depends on the same platform trig/conversion kernels. |

The former deterministic sine mismatch is covered by the reconstructed MSL
path.  The present earliest randomized caller mismatch is case 109 of the
checker: `t=3ef2d8f3`, output lane 1 is original `bfbd3807` versus candidate
`bfbd3806`.  Its normalized dot (after the original sign flip), MSL acos and
all sine results agree; the remaining trace target is the weighted caller
sequence at `801A3904`--`801A3960`.
