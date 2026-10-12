# Original stream phase transfer on locomotion clip changes

## Result — 2026-10-09

The later [retiring-channel milestone](native_retiring_fade.md) now retains the
outgoing pair, applies the bounded original countdown/weights and renders the
original numeric frame-weight order. The phase evidence below remains valid;
its then-unfinished retiring-channel join is now connected in the experimental
launcher. Full callback and changing upstream variants remain open.

The experimental native paired viewer now preserves source cycle duration and
elapsed cycle position when its clip pair changes. It no longer restarts both
streams at frame zero for every speed boundary. This operation is checked
against the actual original channel-rebind caller, not an invented phase rule.

This closes the phase-transfer portion of the previous
[paired playback milestone](native_paired_playback.md). It does not reconstruct
the complete original transition: original `0x8006475C` clones channels and keeps
the old pair for a crossfade. The viewer still owns two host slots and a fixed
upstream source variant/primary curve. Retiring-channel weights, original
allocation and changing variants remain unfinished.

The private experimental launcher remains
`W:\Daywater\Start-NFLStreet-PairedPlayer.cmd`, now pointing at the phase-rebind
build. The default movement launcher retains its passing baseline.

## Original behavior and confidence

Game: NFL Street, GameCube GNNE69 revision0. Original DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Source snapshots came from
Dolphin2603a. Comparisons execute the untouched original module offline;
no new live-gameplay comparison is claimed.

**Verified within controlled rebind calls:** original `0x8017413C` reads the
old stream period and tag count, computes their single-precision product,
and reduces elapsed time modulo that duration through `0x80215CC0`. It sets
up the new stream, derives its period from the same duration divided by the
new tag count, then advances the new stream by the retained remainder.
The native `nflstreet_stream_rebind_phase` reproduces those operations.

| Source field | Type, size, byte order | Original use / controlled observations |
| --- | --- | --- |
| Channel status +1 | uint8,1 | `0x8017413C` takes status2 branch; supplied explicitly by the oracle, not evidence for native allocation |
| Stream +4 (channel +50) | float32,4,BE | Old period multiplied by resolved tag count, then replaced by duration/new count; representative source-derived period1.380720019 for the clamped D9 rate0.3912039995/count17 |
| Stream +8 (channel +54) | float32,4,BE | Elapsed time reduced modulo old cycle duration; controlled0, exact1/3 cycles and neighboring floats |
| Stream +1A (channel +66) | uint16,2,BE | Source programs use counts17/36/40/30/15; FFFF/zero unsupported by this bounded API |
| Stream +C / +10 / +12 / +14 | float32 countdown; uint16 previous/next frame; float32 fraction, allBE | Full64-byte original stream compared after setup, period conversion and phase advance |

The original remainder helper takes float operands and returns float result.
Native `fmodf` matches the full caller comparisons here. Original setup/timing
rounding is retained; the remainder is not converted into a substituted
normalized animation clock.

## Comparison procedure and results

The oracle uses the same private disc bank as native playback, relocated in
scratch RAM for the original registry bank2 lookup. This is a controlled source
input. It does not assert complete byte equivalence with the captured resident
bank; identification of disc bank00111 as this actor's source remains probable.
Neither original DLL nor generated original source is modified.

Across three saved contexts:

- 1,440 carried source-selection/blend/timing updates match.
- 786 full64-byte stream comparisons pass through actual `0x8017413C` on pair
  changes (262 per context). Metadata/status writes outside the stream are
  excluded from this phase-only claim and restored in the controlled harness.
- 18 extra full-caller comparisons match exact cycle boundaries and adjacent
  float values, including elapsed time spanning multiple cycles.
- 1,440 original wrapped-short pose interpolation comparisons still pass.
- Truncated programs and unsupported callback words fail without changing
  stream bytes or flags. Native rollback/reset checks still pass.

Native rebind requires resolved nonzero tag counts and a verified flat loop
program consisting of frame tags followed by8001. Other commands are rejected
before state changes. The original channel metadata, allocation, readiness
decisions and callback eligibility are outside this API.

## Windows integration and remaining work

A60-update actual Windows run renders the phase-preserving paired poses while
preserving all eight recorded movement checkpoints. A528-update key-handler
run passes all eight starts, releases, braking periods and held stops. The
default eight-checkpoint run still passes. These prove native integration;
they do not establish full original gameplay animation equivalence.

Private builds, logs, captures and verification hashes stay in
`W:\Daywater\research-temp\nflstreet-phase-rebind-20261009`.
Build with the existing live-player and locomotion comparison scripts.

Next: retain and blend retiring channels through the original transition
countdown, derive upstream variant changes, and connect other-player/contact/
ball scheduling to the visible runtime. Fresh match initialization remains open.
