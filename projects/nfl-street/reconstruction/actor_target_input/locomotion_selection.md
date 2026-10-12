# Source-backed normal locomotion selection

## Result — 2026-10-09

Native C now joins the existing asset loader to original group metadata,
weighted row selection, the normal locomotion descriptor hierarchy, and
speed-dependent adjacent clip indices. Selected rows, curves and clip pairs
are calculated from source tables; expected original outputs are used only
for comparison. This component does not yet replace the visible viewer's
chosen idle/run cycles or the complete original locomotion callback.

Across three saved source contexts, original comparisons pass:

- 15 assembled group tables (five per snapshot).
- 1,320 weighted descriptor row choices, including source keys and first-row
  ties, with weights 10/9/8/7.
- Three complete 64-byte normal curve payloads after two descriptor choices
  and a final curve lookup.
- 243 speed-to-adjacent-index controls, including exact threshold equality,
  the neighboring float on each side, negative/positive speeds and near-zero
  threshold normalization.
- 972 paired blend/rate controls: entire curve bytes, four complete 124-byte
  channel records and returned rate match original `0x800648EC`, including
  smoothing, optional channels, duplicate channel IDs, zero periods and zero
  tag counts. Eighteen malformed-input checks preserve every output byte.

The actual source bank candidate for this actor is disc `animdata/00111.bin`.
All 4,641 compressed frames, each with 78 rotation components, independently
match the original decoder exactly. The file identification is **probable**:
its bank type, tag 7 and frame count uniquely match resident registry bank 2
among unpacked banks. This does not yet prove every resident loader mutation
or the complete skeleton/channel integration for that bank.

## Original context and confidence

Game: NFL Street, GameCube GNNE69 revision 0. DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. The source captures used Dolphin
2603a. This work executes the unchanged original module offline on three saved
MEM1 snapshots; it does not claim new live Dolphin coverage.

**Verified within these contexts:** table assembly, weighted row selection,
normal source resolution from measured upstream variant keys, and original
speed selector indices. **Candidate:** full movement-to-group/variant service
integration and complete normal callback reconstruction. No gameplay names are
assigned to raw group IDs, variant bytes or curve entries.

**Verified within controlled stages:** paired curve interpolation, normalized
weight smoothing and playback-period rescaling. These comparisons supply
resolved tag counts and controlled initial channel weights; they do not run
the complete callback or original channel allocator.

| Source field | Type, size, byte order | Observed value and original reader/writer |
| --- | --- | --- |
| Registry count/root | uint32 / 4, BE, r13-0x4F70 / r13-0x4F6C with r13 803A9A40 | 91 / 80B4E280; `0x80175F48`, existing native registry reader |
| Actor context | uint16 / 2, BE, Thomas 803C19C0 +4DA | 1; group lookup `0x80175AF8` |
| Active manager record | actor +318 pointer, then record stride100; group +4, state +6, both uint16 BE | Group 004A, state3 in manager record1 at 803C23E0; `0x8017847C` / `0x801784B8` |
| Upstream variant bytes | two uint8 / 1 each, manager payload +3C / +3D | 0C / 13; readers `0x80062594` / `0x800624F0`; their game-service generation remains outside this component |
| Current/primary/alternate curve identities | uint32 / 4, BE, payload +0C / +10 / +14 | 8080537C in tested context; `0x80062340` |
| Other curve identity | uint32 / 4, BE, payload +18 | 80803D34; `0x80062340` |
| Curve bank keys | uint8 / 1, payload +36..+39 | 2 / 2 / 2 / 2; copied registry key, not bank tag; `0x80062340` |
| Curve row count | uint32 / 4, BE, curve +4 | Primary/alternate7, other3; `0x80064638` |
| Curve clip IDs | uint32 / 4, BE, curve +20 + index*12 | Original adjacent startup `0x80064698`; raw IDs remain bank-specific |
| Curve thresholds | float32 / 4, BE, curve +1C + index*12 | Original selector compares next threshold to signed speed; values and boundaries recorded in private logs |
| Curve rate values | float32 / 4, BE, curve +18 + index*12 | `0x800648EC` takes absolute values in place and interpolates the adjacent values using single-precision fused arithmetic |
| Pair descriptor | four uint8 channel IDs, then float32 BE scale at +4 | Controlled primary IDs0/1, optional IDs2/3 or FF; `0x800648EC`; actual allocation remains caller-owned |
| Channel weights | float32 / 4 each, BE, channel +30/+34; channel stride124 | `0x801791A8` installs equal current/target values and clears +38 in this call |
| Channel stream timing | float32 / 4 each, BE, channel +4C then +4/+8/+C; tag count uint16 BE at +1A | `0x801775E8` / `0x801775A0` rescale period, elapsed and countdown; controlled counts0/15/17/36 |

These offsets have verified reader/installation behavior in the tested
source contexts. Curve units, variant meanings and fresh initialization are
not inferred from these comparisons.

## Resolution chain

`nflstreet_asset_group_rows` exposes the existing bounded source-table assembly
transactionally. Original `0x80175AF8` builds rows from registry banks and group
references. `nflstreet_locomotion_select` implements original `0x800211D8`:
compare four metadata bytes, score exact equality and retain the earliest row
on ties. An empty table returns row -1; invalid source spans preserve outputs.

For normal group 4A, source key 0C selects an indirect key A27C. Original code
masks bit15 and resolves group 227C; key13 then selects the next group 22B4.
The final rows identify primary and other curves. `nflstreet_locomotion_resolve`
follows this hierarchy and reproduces the full curve payload installation of
`0x80062340`, including original defaults when alternate curves are absent.
The selected data is exported as small private NLC1 source-curve inputs;
these contain no RAM image, controller checkpoint or expected actor output.

`nflstreet_locomotion_pair` follows the original `0x80064638` instructions.
The decompiler omits its return value and obscures the early return: the
original increments the returned index only after a threshold is passed.
An initial comparison exposed an off-by-one error; the corrected code and
boundary controls now match. Near-zero thresholds use original constant
1e-6 and normalize to zero. Adjacent clip fetch offsets follow original
`0x80064698`; original helper comparisons verify the selected index.
The added boundary harness initially reused the ordinary-speed array instead
of the generated boundary speed; that harness error was corrected before
accepting the 243-case result.

`nflstreet_locomotion_blend` joins those pairs to original `0x800648EC`
weight/rate arithmetic. It clamps the speed ratio, interpolates absolute curve
rates with `fmaf`, applies original weight smoothing constants (0.33 threshold,
0.22 correction), and rescales all selected stream periods through original
`0x801775E8` / `0x801775A0` arithmetic. Optional and repeated channel IDs follow
the original sequential updates. Curve and channel changes commit together.
Malformed source spans, unresolved tag count FFFF, nonfinite inputs and
degenerate thresholds fail with outputs unchanged. The original FFFF tag-count
scan and its callback routing are not reconstructed by this API.

## Corrected callback map and remaining integration

The source registry identifies these actual callbacks for context1:

| Group | Original callback |
| --- | --- |
| 4A | 800635B8 |
| 49 | 80066490 |
| A0 / 9D / 9F | 800669B0 |

The previous static exploration mentioned `0x80066490`; that callback belongs
to group49, not normal group4A. Group4A uses a hierarchy, paired channels,
speed curves, ankle queries and game-service inputs. Its interpolation/smoothing
helper `0x800648EC` and playback interval conversion `0x801775E8` now match in
the bounded native component. They still need connection to stream creation,
paired pose rendering and the viewer before replacing its cycle switch.
Original channel transitions and upstream key changes remain open.

The subsequent [paired playback milestone](../native_assets/native_paired_playback.md)
now connects this component to native stream creation, decoded poses and an
optional viewer path. Its fixed upstream variant and immediate pair restart
remain explicit limitations; the default viewer still retains its earlier cycles.

The subsequent [phase-rebind milestone](../native_assets/native_phase_rebind.md)
replaces immediate experimental pair restarts with checked original duration
and elapsed-phase transfer. Retiring channels, transition blending and upstream
variant generation are still separate unfinished work.

Build with `build_windows_locomotion_selection.sh`. Source headers, native
curve exports, original comparisons, builds and logs stay private in
`W:\Daywater\research-temp\nflstreet-locomotion-selection-20261009`.
Verification JSON records hashes and original counts. Original DLL and runner
source hashes remain unchanged. The existing movement/idle viewer and launcher
remain the passing baseline while this component is integrated.

The subsequent [native curve-choice join](../native_assets/native_curve_choice.md) connects original primary/alternate/other identity decisions and durations to paired playback. Upstream variant generation remains unfinished.

The later [first animation-set key reconstruction](variant_selection.md) joins original 80062C30 decisions to the source curve hierarchy. Game-service fact producers and second-key generation remain unfinished; the viewer still uses its captured set.
