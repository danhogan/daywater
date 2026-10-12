# Weighted pair reduction at `0x801FFEBC`

**Game:** NFL Street, GNNE69 revision 0, ISO SHA-1
`813c4511ba3135059a13de424555647e8252254b`  
**DOL:** SHA-1 `46e3de854998a0f66c9be8c72a1164d22cd29eb4`; [disassembly](../../../../cache/binaries/46e3de854998a0f66c9be8c72a1164d22cd29eb4/disassembly/801ffebc.txt)  
**Confidence:** `verified` guarded CPU state and touched memory on 100,000
finite synthetic calls with GQR0 zero and disjoint input/output buffers,
plus 64 sampled live Windows calls; `candidate` for the game-level role.

The 21 instructions read six float pairs from `r3` (48 bytes), one pair
and one scalar from `r4` (12 bytes), and write three floats to `r5`. A
single-float paired load supplies `1.0f` in its high lane. For each row
`i` from zero through two, the [source model](weighted_pair_reduction.c)
calculates:

```text
first  = source[4*i]   * coefficients[0]
second = source[4*i+1] * coefficients[1]
low    = fma(source[4*i+2], coefficients[2], first)
high   = fma(source[4*i+3], 1.0, second)
output[i] = low + high
```

The generated function stores earlier output rows before loading all later
input rows. Therefore the portable model assumes disjoint input/output
buffers; it does not claim behavior for overlap. It also omits paired
registers, FPSCR, PC, cycle charge, and exceptional floating-point values.
The separate [guarded CPU-state candidate](full_state_candidate.c) covers
those side effects within a finite, GQR0-zero domain and rejects
input/output overlap. The `ps_sum0` instructions preserve the prior high
lanes of destination registers `f6` and `f12`; the candidate models that
detail.

The [offline checker](check_module.c) called the original Linux whole-DOL
module at `0x801FFEBC` with 100,000 deterministic finite input sets and
GQR0 zero. All 300,000 output words matched bit for bit; there were zero
dispatch failures. Build and run with the local ModernGekko headers/module:

```bash
cc -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off \
  -I/path/to/GXRuntime/include -I/path/to/StaticRecomp \
  check_module.c weighted_pair_reduction.c -ldl -lm \
  -o /tmp/check-weighted-pair
/tmp/check-weighted-pair /path/to/gGNNE69_recomp.so
```

This source candidate is not installed in the Windows DLL.

The [full-state checker](check_full_state.c) also compared the entire CPU
state and touched RAM against the original module on 100,000 deterministic
finite cases. It alternated zero and saved-match FPSCR values and varied
the incoming high lanes of `f6` and `f12`. **All 100,000 matched**, with
zero guarded skips, mismatches, output mismatches, or dispatch failures.
Build it with the same headers and module, adding
`check_full_state.c weighted_pair_reduction.c full_state_candidate.c`.

A [live differential probe](../../native/weighted_fastpath.md) compared the
candidate with the original generated instructions on identical CPU and
memory snapshots from a Windows saved match. It matched 32 early calls and
32 calls after the first 10,000, with no skips or mismatches. An isolated
experimental DLL with the guarded replacement completed a 600-frame
saved-match smoke test. The baseline launcher and DLL remain unchanged.
