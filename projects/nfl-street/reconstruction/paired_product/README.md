# Paired product candidate at `0x801FFDF0`

**Game:** NFL Street, GNNE69 revision 0, ISO SHA-1
`813c4511ba3135059a13de424555647e8252254b`  
**DOL source:**
`cache/binaries/46e3de854998a0f66c9be8c72a1164d22cd29eb4/disassembly/801ffdf0.txt`  
**Confidence:** `probable` for the algebra derived from the 51-instruction
PowerPC function; `verified` bit-for-bit output agreement on the stated
synthetic finite domain. The function's game role is unknown.

The function loads six pairs from each of `r3` and `r4` with GQR0, loads a
constant pair from `0x803A3958`, and writes six result pairs to `r5`. For
each of three four-float input groups `(x0,x1,x2,x3)`, let the six
coefficient pairs loaded from `r4` be `B0` through `B5`, and let the constant
pair be `K`. Its paired-single operations give:

```text
first output pair  = B0*x0 + B2*x1 + B4*x2
second output pair = B1*x0 + B3*x1 + B5*x2 + K*x3
```

Multiplication and addition are componentwise within a pair. The generated
code executes the products and fused adds in that order, and the portable
[`paired_product.c`](paired_product.c) candidate follows it after the loads.
It does not emulate memory access, GQR settings, FPSCR, or register effects.

[`check_module.c`](check_module.c) calls the existing Linux whole-DOL module's
public dispatch at `0x801FFDF0` with synthetic guest RAM. Across 100,000
deterministic trials, all 12 output words per trial matched the C candidate
bit for bit. Inputs, factors, and constant-pair values were finite float
values in approximately `[-2,2]`, GQR0 was zero, and every dispatch returned
normally without a CPU exception. The constant address is set to synthetic
data for this test; the saved-match [live probe](../../native/live_pair_probe.md)
observed `(0.0f, 1.0f)` at that address. The comparison
does not establish behavior for exceptional floats, other GQR0 values, or
all CPU-state side effects.

[`full_state_candidate.c`](full_state_candidate.c) also models the paired
registers, FPSCR, saved stack values, memory writes, register `r6`, PC, and
cycle charge within a guarded normal-float domain. The differential checker
compared the entire CPUState and touched RAM against the original module on
the same 100,000 trials: **100,000 full-state matches, zero mismatches, zero
guarded skips**. Trials alternated FPSCR zero with the value observed in the
saved Windows match. This does not cover exceptional values or all aliasing
and external-memory cases; the guard declines unsupported cases.

The guarded candidate also permits the observed `0xE0000000` locked-cache
range when external read, write, and validated pointer callbacks are
available. The [locked-cache differential checker](check_locked_cache.c)
compared full CPU state, stack, and locked-cache bytes against the Linux
whole-DOL module on 10,000 finite synthetic trials, including output/input
overlap: **10,000 matches, zero skips or mismatches**. The [live Windows
probe](../../native/locked_cache_fastpath.md) matched 32 early and 32 later
product calls on identical CPU, MEM1, and locked-cache input. These checks
cover the tested ModernGekko/Dolphin backing and finite-value guard, not
arbitrary external devices.

Build and run on Linux with the locally generated module:

```bash
cc -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off \
  -I/path/to/GXRuntime/include -I/path/to/StaticRecomp \
  check_module.c paired_product.c full_state_candidate.c \
  -ldl -lm -o /tmp/check-paired-product
/tmp/check-paired-product /path/to/gGNNE69_recomp.so
```

For the locked-cache cases, use the same include paths with
`check_locked_cache.c full_state_candidate.c` and run the resulting binary
against that module. The checker uses synthetic cache backing and stores no
game data.

The [timed-site profile](../../native/timed_sites.md) ranks this entry PC
second by sampled dispatch time in all three saved-match trials. A separate
[combined Windows experiment](../../native/two_fastpaths_experiment.md)
tested this full-state candidate alongside the polynomial candidate. The
baseline launcher remains unchanged. The later locked-cache build did not
show a repeatable speed improvement in paired saved-match trials.
