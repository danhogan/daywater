# Live Windows paired-function probe

**Date:** 2026-10-04  
**Game:** NFL Street, GNNE69 revision 0, ISO SHA-1
`813c4511ba3135059a13de424555647e8252254b`  
**Confidence:** `verified` agreement for the captured 64 calls in this saved
match replay; no claim of complete replacement equivalence.

The opt-in [probe patch](moderngekko-pair-probe.patch) records the first 32
calls at each measured hot entry PC, including the input words and output
words around native module dispatch. It was applied to the local vendored
Dolphin checkout, built as a separate
`moderngekko-run-pair-probe.exe` (SHA-1
`218a7cf88b95f69cc754040148bef032d9985366`), and run with
`--pair-probe` from the saved first-period Chiefs–Falcons state. The normal
runner and source were restored after the experiment; the normal runner's
SHA-1 remains `d1afca7d18e3691e12837888f872f8f3ba4b31ea`.

The bounded replay began at emulated frame 16,562 and reached 16,684, for
122 observed frames. The runtime reported 14,444,451 native dispatches,
zero interpreter fallback steps, and zero failed code chunks. The private
log and validation result are on W: as
`W:\Daywater\research-temp\nflstreet-pair-probe-live.log` and
`W:\Daywater\research-temp\nflstreet-pair-probe-validation.txt`.
The log SHA-256 is
`7b2f61c8ff92d58fe624526fe47f78bfe211555d2fb090250762c89f35e9baf9`.

The [validator](validate_pair_probe.py) compared each captured output word
with the two portable C candidates. Both functions matched **bit for bit on
all 32 captured calls**:

| Entry PC | Captured calls | Output words per call | Differences |
|---|---:|---:|---:|
| `0x80188BB0` | 32 | 2 | 0 |
| `0x801FFDF0` | 32 | 12 | 0 |

The captured polynomial calls had GQR0 = `0` and GQR6 = `0x0F070007`, the
value set by the DOL initialization routine at `0x8015668C`. The captured
paired-product calls had GQR0 = `0`; the constant pair read from
`0x803A3958` was `(0.0f, 1.0f)` in these calls. These observations establish
the tested call conditions, not the purpose of either function.

The candidates currently compute output values only. The generated
functions also modify guest registers, FPSCR, stack memory, cycle accounting,
and program counter. A replacement in the Windows module must match those
effects, and then pass a longer live replay and matched performance test.

To rerun validation of an existing log on Linux, compile the small C
candidates into a shared library and run:

```bash
cc -std=c11 -O2 -shared -fPIC -ffp-contract=off \
  projects/nfl-street/reconstruction/odd_polynomial_pair/odd_polynomial_pair.c \
  projects/nfl-street/reconstruction/paired_product/paired_product.c \
  -lm -o /tmp/libnflstreet-pair-candidates.so
python projects/nfl-street/native/validate_pair_probe.py \
  /mnt/w/Daywater/research-temp/nflstreet-pair-probe-live.log \
  /tmp/libnflstreet-pair-candidates.so
```
