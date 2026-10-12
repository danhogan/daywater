# Native locomotion curve choice

## Result and scope

**Verified within controlled original stages:** `0x80062624` now chooses the
primary, alternate or other curve from carried movement inputs in native C.
It preserves source identity aliases, inclusive curve limits, unchanged outputs
on rejected candidates, and original transition durations 6/8, doubled 12/16
when leaving the other curve. A valid unchanged candidate still writes duration.

The experimental Windows viewer joins this decision to existing source clip
selection, phase transfer, four-channel fades and frame-weight ordering.
All eight original recorded movement checkpoints and all eight Windows direction,
release and stop segments pass. This run does not demonstrate naturally occurring
other-curve gameplay: the captured ordinary movement context stays on primary.
Controlled original comparisons separately exercise real primary/other swaps.

Upstream variant keys 0C/13 are still supplied by the private captured profile.
Game-service decisions `0x80062C30` / `0x80062E2C`, actor callback eligibility,
body/velocity angle sign, root travel and fresh actor initialization remain open.
This is one independently running player, not a complete football match.

## Source evidence and inputs

Game: NFL Street, GameCube GNNE69 revision 0. ISO SHA1
`813c4511ba3135059a13de424555647e8252254b`; DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`.
Inputs are the private position-neutral 14/15/16 MEM1 snapshots from the
Dolphin 2603a saved-match reference. Comparisons execute the untouched generated
original module, not live Dolphin gameplay.

The original PPC instructions confirm the constant at `0x8024F9C8` is float32
zero; all three snapshots contain `00000000`. The source callback
`0x800635B8` checks the decision only when no retiring fade remains, resets
its local duration input to 8, then starts a new pair when either the index or
curve changes. Native code preserves this ordering within the bounded loop.

| Source field relative to actor | Primitive / size / endian | Numerical role / native input | Reader |
| --- | --- | --- | --- |
| +1C4 | float32 /4 /BE | Exact-zero test and curve-limit speed; movement state 7 | 80062624 |
| +1DC | float32 /4 /BE | Negative/zero/positive branch; state 13 | 80062624 |
| +1E8 / +1EC | float32 /4 each /BE | Compared bounds; state 16/17 | 80062624 |
| +21E | uint8 /1 | Equality with 10 when comparison is zero; actor word 7 bits 8..15 | 80062624 |
| Curve+C / +10 | float32 /4 each /BE | Inclusive lower/upper limits | 80062624 |
| Descriptor+C / +10/+14/+18 | uint32 opaque identities /4 each /BE | Current/primary/alternate/other; pointer equality preserves aliases | 80062340,80062624 |
| Descriptor+36..39 | uint8 /1 each | Current and source registry keys; observed 2 | 80062340,80062624 |

Confidence is verified for these numerical/structural roles within the
specified source function, not for unnamed gameplay meanings or universal
RAM addresses. Actor input storage comes from the documented capture adapter:
12 actor words start+200, 21 movement words start+1A8. The viewer passes
carried values, never expected recorded outputs. The selected actor in these
snapshots is `0x803C19C0`; this is a reference-instance identity.

Observed snapshot 14 values: speed 0.0910693333, comparison 0,
boundA0.1204705983, boundB0.0946693346, status 0. Primary and alternate share
identity `0x8080537C`; other is `0x80803D34`. Primary/alternate limits are
approximately[-0.0845640823,0.2731481194]; other limits are
[-0.00000160907075,0.1440220028]. No football state names are inferred from
status 10 or the compared values.

## Native connection

`locomotion_selection.c` accepts neutral float/status inputs plus three opaque
source identities and limits. It returns current identity, bank, duration and
change flag transactionally. It rejects nonfinite inputs and malformed ranges;
these safety restrictions do not claim original behavior for invalid data.

NLC2 exports include original identities alongside raw curve data and registry
keys. This avoids assuming identical bytes imply shared identity. The exporter
resolves the source hierarchy and verifies its complete payload against the
original first. Runtime profiles contain no RAM image or expected checkpoints.
NLC1 and the existing `step(speed,delta)` retain the legacy fixed-primary path.
The new `step_actor` consumes source choice only outside a fade and forces
rebind on changed identity even if the pair index is unchanged. Reset restores
primary. The default chosen idle/run viewer and launcher are preserved.

## Checks and reproduction

Build with `build_windows_locomotion_selection.sh` and
`build_windows_live_player.sh`. Selection export accepts a fifth argument
`--identities` for NLC2; four arguments keep NLC1. All builds and inputs remain
private on W: under `Daywater/research-temp/nflstreet-curve-choice-20261009`.

- Three contexts: 6,318 exact original decisions, including signed zero,
  lower/upper exact and adjacent float limits, negative/zero/positive comparison,
  bound order, three statuses, three current identities and both distinct/aliased
  source configurations. Some configurations are controlled hypotheses, not
  observed gameplay. Counts: 2,151 changes,2,070 unchanged duration outputs,
  1,362 doubled durations. Invalid-input rollback passes.
- Three joined context sweeps: 1,440 complete curve-choice/playback updates,
  24 source-identity swaps,69 pair starts,504 fade updates,63 completions and
  162 gated pair requests. Original numeric pose-list ordering/weights match
  6,570 entries. Source descriptor comparison covers 64 bytes after omitting
  the independently checked source identity/bank slots; source curve bytes,
  current identity, status byte and channel+30..7B match exactly. Other channel
  metadata and full original callback/actor scheduling are outside this claim.
- Existing three-context fade and phase-only regression oracles pass.
- Actual Windows source mode: 60-update recorded run and 528-update eight-direction
  run pass. Default recorded run passes; five inspection BMPs remain byte-identical
  to the preceding baseline. This is native diagnostic rendering, not original
  pixel equivalence.

Private verification JSON records results and unchanged baseline hashes:
original DLL SHA1 `dd521cc38b0db6bca98a0bb261570a2be5e6e0ed`;
runner SHA256 `3f9e3a3eb4b5728d9036e9552c01e8c9dc084a1f53805fef95dcf3b1b86f73ea`.
No original DLL/source edits or baseline restoration were required.

Next: generate changing variants from original actor/game-service inputs, then
join other actors, contact and ball scheduling into visible native gameplay.

The subsequent [directional playback join](native_directional_playback.md) connects the original distinct heading thresholds for clip selection and blending to this native controller. Full gameplay callback eligibility and changing upstream variants remain unfinished.
