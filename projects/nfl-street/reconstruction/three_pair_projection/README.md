# Three pair projection at `0x801FFF10`

**Game:** NFL Street, GNNE69 revision 0, ISO SHA-1
`813c4511ba3135059a13de424555647e8252254b`  
**DOL:** SHA-1 `46e3de854998a0f66c9be8c72a1164d22cd29eb4`; [disassembly](../../../../cache/binaries/46e3de854998a0f66c9be8c72a1164d22cd29eb4/disassembly/801fff10.txt)  
**Confidence:** `verified` for the guarded CPU and memory behavior in the tested
finite, GQR0-zero domain; `candidate` for its game-level purpose.

The 21 instructions at `0x801FFF10` through `0x801FFF60` read six float
pairs from `r3` (48 bytes), a coefficient pair and scalar from `r4`
(12 bytes), and write three floats to `r5` (12 bytes). For each row `i` from
zero through two, the observed numerical operation is:

```text
product0 = source[4*i]   * coefficients[0]
product1 = source[4*i+1] * coefficients[1]
output[i] = fma(source[4*i+2], coefficients[2], product0 + product1)
```

The pair's high lanes, register writes, FPSCR result class, PC, and cycle
charge are modeled by [full_state_candidate.c](full_state_candidate.c).
It accepts MEM1 or validated locked-cache pointers and declines unsupported
FP modes, exceptional values, and inaccessible memory. Input is loaded before
the output stores, including when output overlaps input. The portable
[three_pair_projection.c](three_pair_projection.c) states just the numerical
calculation.

The [offline differential checker](check_module.c) ran 100,000 deterministic
finite cases against the original Linux whole-DOL module. It alternated zero
and saved-match FPSCR values, and half the cases overlapped output with the
input. All 100,000 matched the full CPU state and touched RAM, with zero
guarded skips or mismatches. The Windows live differential probe also compared
the original generated function against the candidate on identical CPU and
memory snapshots from a saved match: 32 early calls and 32 calls after the
first 10,000 matched, with zero skips or mismatches. The 600-frame later run
reached frame 17,165 and exited successfully. These tests establish behavior
only inside the stated guard and for the tested module and live state.

Build the offline checker with the local ModernGekko headers and Linux module:

```bash
cc -std=c11 -O2 -Wall -Wextra -Werror -ffp-contract=off \
  -I/path/to/GXRuntime/include -I/path/to/StaticRecomp \
  check_module.c three_pair_projection.c full_state_candidate.c \
  -ldl -lm -o /tmp/check-three-pair
/tmp/check-three-pair /path/to/gGNNE69_recomp.so
```

The Windows diagnostic staging helper is
[stage_projection_live_probe.py](../../native/stage_projection_live_probe.py).
The diagnostic DLL and logs stay on the private W: drive. The bundle's
baseline DLL was not replaced.
