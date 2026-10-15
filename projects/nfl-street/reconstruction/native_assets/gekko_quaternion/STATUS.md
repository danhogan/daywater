# Quaternion-kernel reconstruction status

The checks use the saved `position-neutral-14.mem1` all-RAM image and the
locally generated GNNE69 recompiled module.  Neither is part of this tree.

| Entry | State | Evidence |
|---|---|---|
| `80188BB0` | verified | `check_80188bb0_module.sh` passes all 65,536 angle words for the saved polynomial and four randomized finite coefficient sets: 327,680 exact result pairs. |
| `801A36EC` | verified | `check_801a36ec_module.sh` passes 100,000 randomized full 24-bit Euler triples.  It follows `8018889C`'s paired lanes and final `ps_sum0`; a Horner translation was not exact. |
| `80216B94` | verified (bounded caller domain) | The complete `[0, pi/2]` path used by slerp is transcribed: `802171C4` reduction plus `80216EF4`/`80216E0C` kernels and DOL IEEE words.  The zero-tail `80216EF4` path is evaluated separately from its general formula, as in the instructions.  500,000 randomized calls match the original module bit-for-bit.  The large-argument MSL reducer remains outside this caller proof. |
| `802165C8` | verified (normal positive domain) | The restoring integer square-root loop, including round-to-even, passes 500,000 module calls from the normal positive range used by `80215844`.  Denormal handling remains unported. |
| `80215844` | verified (slerp cosine domain) | The two non-negative `acosf` paths and their stored-DOL coefficients pass 500,000 randomized calls in `[0,1]`. |
| `801A383C` | verified | It uses reconstructed MSL sine, acos and square-root helpers, with PPC `fdivs`, `fmuls`, and tie-corrected single FMA rounding points made explicit.  The former `t=3e5978d9` sine discrepancy and the later case-109 final-weight discrepancy are covered.  500,000 all-RAM randomized quaternion/Slerp vectors match the original module bit-for-bit. |
| `801A340C` | verified | `80188D98` atan2 polynomial, 24-bit conversion and the `80188EF8` singular branch are transcribed; 500,000 quaternion cases from exact `801A36EC` results match the original module. |
| `801EAD98` / `801CA900` | probable; compiled, comparison pending | Extracted into `actor_slerp.c`; `actor_palettes.c` routes flag `0x4000` through it. The checker has an optional `slerp` mode with 401 cases over all 14 actors, edited compact poses, weights and enable bytes. See `actor_slerp.md` for the operator's command and limits. |

The checker's `80216B94` probe includes the former `t=3e5978d9` path.  Its
all-RAM Slerp probe includes case 109 (`t=3ef2d8f3`), where the final weight
was formerly one ULP low; both now match exactly.
