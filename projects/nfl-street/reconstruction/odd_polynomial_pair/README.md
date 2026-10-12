# Two-lane odd polynomial candidate at `0x80188BB0`

**Game:** NFL Street, GNNE69 revision 0, ISO SHA-1
`813c4511ba3135059a13de424555647e8252254b`  
**DOL source:**
`cache/binaries/46e3de854998a0f66c9be8c72a1164d22cd29eb4/disassembly/80188bb0.txt`  
**Confidence:** `probable` for the algebra below, directly derived from the
29 PowerPC instructions; `candidate` for any numeric C translation. The
function's game role and the caller's input meaning remain unknown.

The routine loads two values from a stack halfword pair through `psq_l` with
GQR6, then loads six coefficient values from `r4` through three paired loads
with GQR0. Let the quantized load result be `(t0, t1)` and the coefficient
loads be `(a0, a1)`, `(b0, b1)`, and `(c0, c1)`. The PowerPC paired-single
operations produce the following algebraic results, written as singles to
the addresses in `r5` and `r6`:

```text
P(t) = a0*t + a1*t^3 + b0*t^5 + b1*t^7 + c0*t^9 + c1*t^11

*[r5] = P(t0)
*[r6] = P(t1)
```

The exponent pattern follows from `ps_mul` constructing `(t²,t²)`,
`(t³,t³)`, and `(t⁴,t⁴)`; `ps_merge00`/`ps_merge11` making `(t,t³)` for each
lane; and two `ps_muls` rounds producing the fifth/seventh and
ninth/eleventh powers. The three coefficient pairs feed one multiply and
two multiply-adds per lane, followed by `ps_sum0`.

This is an **algebraic model**. It is not an exact C replacement yet:
`psq_l` depends on GQR configuration, and GXRuntime's paired-single helpers
apply PowerPC rounding, exceptional-value handling, and FPSCR updates after
individual operations. A native fast path must be compared against those
effects before it can replace the generated instruction sequence. The
[timed-site profile](../../native/timed_sites.md) identifies this entry PC as
one of two repeated high-time samples in the saved match segment.

[`odd_polynomial_pair.c`](odd_polynomial_pair.c) is a small portable C
candidate for the numerical core **after** the GQR loads. It follows the
observed multiply and fused-add order. It deliberately does not read guest
memory or emulate GQR, FPSCR, or PowerPC exceptional values. The finite
known-value check runs with:

```bash
cc -std=c11 -Wall -Wextra -Werror -ffp-contract=off \
  odd_polynomial_pair.c test_odd_polynomial_pair.c -lm -o /tmp/odd-pair-test
/tmp/odd-pair-test
```

Passing that check confirms the coefficient and exponent order in this
candidate; it does not establish numerical equivalence to the DOL.

## Native module differential check

[`check_module.c`](check_module.c) loads the existing Linux whole-DOL native
module by its public ABI and calls its dispatch at `0x80188BB0` with
synthetic RAM and register state. For 100,000 deterministic trials, it used
GQR6 value `0x0F070007` (signed 16-bit loads with scale 15, as set by the
initialization routine at `0x8015668C`), six finite float coefficients in
approximately `[-2, 2]`, and a randomized full 16-bit `r3` value. Both
output words matched the candidate **bit for bit** on every trial; all
dispatches returned normally with no CPU exceptions. This is a verified
comparison for that synthetic domain, not proof about every GQR setting,
exceptional float value, FPSCR side effect, or live-game call.

The full function also writes stack halfwords, updates general register
`r7`, restores `r1`, changes several paired-single registers and FPSCR,
charges 29 guest cycles, and returns through LR. Those observable CPU-state
effects are absent from the portable numerical core.

[`full_state_candidate.c`](full_state_candidate.c) models those effects in a
guarded normal-float domain. It returns false without changing state when the
required RAM, GQR, FP, or finite-value conditions fail. The differential
checker compared the entire CPUState and the touched RAM against the original
module: **99,942 full-state matches, zero mismatches, 58 guarded skips**
among 100,000 deterministic synthetic trials. Trials alternated FPSCR zero
with the `0xA2308104` value observed in the saved Windows match. This check
does not cover exceptional values, external memory, or every aliasing case.

Build and run on Linux against the locally generated module:

```bash
cc -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off \
  -I/path/to/GXRuntime/include -I/path/to/StaticRecomp \
  check_module.c odd_polynomial_pair.c full_state_candidate.c \
  -ldl -lm -o /tmp/check-odd-module
/tmp/check-odd-module /path/to/gGNNE69_recomp.so
```

The locally tested module was
`W:\Daywater\research-temp\nflstreet-moderngekko-modules\GNNE69\a065ae2f4c192124266a56171aa5eed4e2dccb586d32670c5dab2e2ed3ad980c-c4dcfb815f5c2267\gGNNE69_recomp.so`.
The test is fast and uses only synthetic RAM; it does not launch Dolphin.
