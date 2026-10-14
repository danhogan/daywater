# Quaternion-kernel reconstruction status

The checks use the saved `position-neutral-14.mem1` all-RAM image and the
locally generated GNNE69 recompiled module.  Neither is part of this tree.

| Entry | State | Evidence |
|---|---|---|
| `80188BB0` | verified | `check_80188bb0_module.sh` passes all 65,536 angle words for the saved polynomial and four randomized finite coefficient sets: 327,680 exact result pairs. |
| `801A36EC` | verified | `check_801a36ec_module.sh` passes 100,000 randomized full 24-bit Euler triples.  It follows `8018889C`'s paired lanes and final `ps_sum0`; a Horner translation was not exact. |
| `801A383C` | candidate | Dot product, inverse length, `acos`, and the final fused weighting order have been transcribed.  The host `sinf` placeholder is not Gekko's `80216B94` kernel. |
| `801A340C` | unstarted | Depends on the same platform trig/conversion kernels. |

The deterministic first slerp mismatch after fixing the final `fmuls` then
`fmadds` sequence is: `t=3e5978d9`, `a=(bff00000,3fa00600,3fbee800,3ffe2000)`,
`b=(3f8fac00,3ecb5800,bf451800,bd4a8000)`.  At output lane 0, the original is
`3fb282a1` and the candidate is `3fb282a2`.  The original sine called by the
slerp is `3f4ccc94`; glibc `sinf` returns `3f4ccc95`.  This is the next kernel
to replace, by following `80216B94` / its argument reduction rather than
rounding a host result.
