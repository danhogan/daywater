# Original team selection: data and carousel

## Current result

Team selection is next after the checked controller-screen completion requests
entry7. Its data reader and three-item, two-sided carousel now run independently
in native Windows code using the private original team records. Fresh complete-source team activation now runs independently and a diagnostic
2D capture draws its original title/logos/prompts. Selected-athlete models, source
input/confirmation and field8 handoff remain open. See [fresh startup](team_screen_startup.md).

This work remains isolated under the NFL Street reconstruction. Daywater's
generic backend, emulator services and web behavior were not changed.

## Implemented pieces

- `team_catalog.c/h`: bounded version8 DB block reader for the raw `MAET`
  table. It reads declared descriptors and used records, extracts unsigned
  MSB-first numeric fields, and copies byte-aligned text with a host NUL.
  It rejects truncated blocks, unsupported versions and fields outside the
  declared record. It does not validate DB checksums or unpack whole archives.
- `team_selection.c/h`: owns up to64 records, indexes them by the actual
  `DIGT` values, and exposes names (`ANDT`) and the three raw fields requested
  by original `8011CDB4`: `FORT`, `EDRT`, `VORT`. It explicitly sorts local
  records by ID; original DB query sorting/filter initialization is separate.
- `team_carousel.c/h`: reconstructs original `8011CE64` operations1 and3
  and its general copy branch for both sides. Forward/backward movement
  updates all three items, wraps, returns three flags and preserves the
  original final cursor at the third returned item. General queries
  `80009E5C/6C` are supplied false; profiles are supplied absent.
- `team_selection_probe.c`: runs without the original module, ModernGekko,
  Dolphin or snapshots. It reads32 private blocks, uses explicitly supplied
  starting IDs, and moves both sides forward/backward through complete wraps.
  This is a data/selection probe, not a visible menu or match launcher.

The DB format starting point came from George Greer's
[Madden NFL 2003–2004 database documentation](https://www.m-l.org/~greerga/madden/file-format.html).
The game-specific version8 header offsets, descriptors and record extraction
were checked against this game's blocks and original routines; the reference
alone was not treated as proof of compatibility. No external parser code was
copied. Raw table/field tags are recorded in this game's byte order.

## Verification and confidence

Game: NFL Street, GameCube, GNNE69-r0. Original DOL SHA1:
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`.
Original reference DLL SHA1:
`dd521cc38b0db6bca98a0bb261570a2be5e6e0ed`.
Contexts14/15/16 are existing Dolphin2603a saved match RAM, **not fresh team
menu captures**. Comparisons execute untouched original routines with explicitly
seeded scratch rows/window storage and supplied database/profile services.

| Finding | Confidence | Procedure and limit |
| --- | --- | --- |
| Version8 raw MAET record extraction | verified, controlled | All1,216 numeric and96 text fields across32 private blocks match original `801B94C8` and `801C4388` in each of three contexts. Text bytes match; the host adds its own NUL after the declared width. Original archive/database initialization is not executed. |
| DIGT is the record ID and ANDT supplies its name | verified, controlled | Those tags are used by original `80150978`/`80150944`; extracted values match original field readers for all32 records. IDs are checked unique; file entry order differs from numeric ID order. |
| Original two-side three-item carousel | verified, controlled | `8011CE64` matches384 supplied-catalog cases per context across sizes1–64, both sides, operations1/3/general and wraps, plus6 cases using actual private records. Compare both windows, returned words/flags, final cursors and scratch-storage guards. |
| No-profile flag branch | verified, controlled | Original `8011D060` executes with `80127900` returning FFFFFFFF for both profiles; supplied `SIVT` is read through `801495BC`. Original predicate is `(SIVT == 0)` and `8011CE64` returns its inverse. Profile-backed eligibility and unlock filtering are not covered. |
| Three raw numeric values projected for a row | verified extraction only | Read tags FORT/EDRT/VORT used by `8011CDB4` and compare their original primitive field values. Complete `8011CDB4` query lifecycle and gameplay meanings remain unverified. |
| No-profile startup/default windows | verified, controlled | `8011C414` now agrees with native initialization in48 cases per saved context. Database/profile/random/actor services are supplied; full source-script activation remains separate. See the startup section below. |
| Team commit and full menu lifecycle | candidate/source mapped | Entry7 selects callback `8011E9D4`; `8011D19C` changes selected state and additional roster/actor systems. Complete input/commit, profile-backed bootstrap and source screen lifecycle are not reconstructed. |

The supplied original window pointer table is seeded at `r13-5A38`
(`803A4008`, with this module's `r13=803A9A40`). Original cursor objects are
`802F507C`/`802F50A8`,44 bytes apart. The checker supplies seek/read/wrap,
cursor save/restore and raw SIVT query boundaries. These are source function
references and controlled scratch state, not newly verified live menu RAM
observations; their original initialization/writers remain a later gate.

The original generated module can jump directly into `8011D060` within a chunk
without returning to the dispatcher at its entry. An initial predicate-entry
hook was therefore ineffective. The final checker executes that original
function and hooks its lower services; this avoids silently comparing against
unsupplied saved-match database state.

## Reproduction

```bash
python3 projects/nfl-street/reconstruction/menus/verify_team_selection.py
```

The driver performs strict Windows builds, checks all three original contexts,
runs the independent native selection probe, hashes inputs/builds/source and
checks the original DOL/DLL/runner hashes before and after. `--no-build` reuses
existing binaries. Original inputs, EXEs, detailed record values and diagnostics
remain under W:, with the verification record at:

`W:\Daywater\research-temp\nflstreet-team-select-20261010\verification.json`

There were no experiments that changed the baseline source or DLL. Original
module use is confined to verification; the native probe does not load it.

## Next connection

Reconstruct original team-query initialization, including its additional runtime
fields/profile filtering. Join the bounded startup and
the verified row/carousel services to the entry7 source scripts and render
the original team artwork/portraits. Execute actual source input and confirmation
before connecting the entry8 field recipient. Choosing these32 NFL records in
a local probe does not establish the complete original set of selectable teams,
career/unlock state, chosen roster propagation or fresh match startup.

## No-profile startup and source object construction

`team_start.c/h` reconstructs original `8011C414` with actual local records,
absent profiles, supplied profile mode6, `80149AF0=true`, `80150A00=false`,
random selection response0 and `80004668=false`. Resource requests are retained
in order; actor previews and actual resource I/O are external.

The original routine and its `8011C294`, `8011C1DC`, `8011C3A0` and
`8011D060` helpers execute intact in the checker. Only lower database,
profile, resource, random and actor-service boundaries are supplied. The
comparison checks stale/absent prior identities, valid/missing stored team IDs,
both query modes, windows/cursors, eligibility counts, opaque flag bytes,
four cursor records' initialization, scratch-storage guards and every retained
resource request in order. Actual32-team records, a supplied mixed-SIVT catalog
and a single-item supplied catalog cover48 cases per context (144 total).

Two source details matter when wiring this to scripts:

- With no stored ID found and supplied random0, initialization uses the first
  raw row even if SIVT is zero. A valid stored ID also remains selected for
  positioning regardless of SIVT. Eligibility counting is a separate step.
  The stored selected-ID word itself is not changed by this startup branch.
- Initial windows contain previous/center/next IDs, but the cursor ends at
  center+2 because startup advances after all three reads. Later carousel
  operations end at the third returned row instead. Query mode0 clears
  flags at r13-6E44/-6E43, preserves the remaining six modeled bytes and the
  previous eligibility counts, and zeros the three modeled counter words.

`team_start_probe.c` joins this native initialization to the existing carousel
on real private records. Both sides start from the original fallback procedure
and then move through original operations1/3. It runs without snapshots or the
original module; invalid mode/catalog inputs reject with state unchanged.

`team_classes.c/h` constructs the original team source's image0/text8 objects
using checked generic constructors and four original fonts. The independent
probe and original checker use a supplied registry/mask0x101; class3 actor
preview links are omitted. Source object construction is separate from creation
and selected-activation scripts, timers or drawing a visible screen. The
constructor comparison checks the complete253,987-byte file, normalizing only
line-table allocator addresses, and compares every allocated line range and
ordered image/font acquisition and font lookup identity. Each context matches
1,186 callbacks (1,073 image and113 text),388 ordered resource identities and
the complete source file. Text construction acquires its font through original
`801D9788` before the lookup inside `801D8E54`; both requests are retained. Explicit newline labels and
word-wrapped source objects retain their original dimensions/ranges. The
existing generic single-line behavior is unchanged.

```bash
PYTHONDONTWRITEBYTECODE=1 python3 projects/nfl-street/reconstruction/menus/verify_team_start.py
```

The driver repeats startup and constructor comparisons in contexts14/15/16,
both independent probes, strict builds and baseline/provenance hashes. Private
outputs are under `W:\Daywater\research-temp\nflstreet-team-start-20261010`.
Confidence is **verified within these controlled branches**. Fresh original
menu activation, original query/profile bootstrap, source input/confirmation,
rendered team portraits and selected-match creation remain unverified.

## Source callback memory bridge — 2026-10-10

`team_callback.c/h` connects the checked no-profile startup and carousel to
original `8011E9D4` properties `80000001` and `80000004`. Source requests write
only caller-owned screen bytes or host VM stack words; they never read live RAM
or perform external resource I/O. Startup returns raw result0. Carousel retains
the existing result word, writes previous/center/next IDs and eligibility outputs,
and commits its updated windows/cursor together. Array headers use the original
`descriptor + (dimension_count + 1) * 4` arithmetic. Dimensions above8,
overlapping output arrays, ambiguous screen/stack ownership, unsupported
properties and invalid destinations reject; broader original alias branches
are not claimed.

Evidence is **verified within the supplied no-profile branch**:

- Original startup and dispatcher execute intact with the same supplied lower
  services:48 direct and48 callback comparisons per context (144 of each
  across contexts14/15/16). All prior startup checks still apply.
- Carousel now runs both original direct8011CE64 and actual
  8011E9D4/80000004 framing:780 comparisons per context,2,340 total. This is
  the previous1,170 direct comparisons plus1,170 actual callback comparisons.
  Array dimensions0/1/2, both sides, operations1/3/other, supplied catalogs
  sizes1..64 and actual private32-team records are covered. Original outputs,
  result retention, windows, cursors and surrounding storage are compared.
- The independent native startup probe joins callback initialization to
  carousel changes with one output in screen memory and the other on the VM
  stack, then reverses those owners. Six invalid-request guards verify screen,
  stack, result and complete callback state remain unchanged. Mixed-memory
  checks are offline native integration, separate from the original dispatcher
  comparisons whose outputs reside in supplied original scratch memory.

Private reports:
`W:\Daywater\research-temp\nflstreet-team-callback-20261010\verification.json`
and
`W:\Daywater\research-temp\nflstreet-team-callback-start-20261010\verification.json`.
Run the existing verification drivers with `--output` pointing to those private
folders. Strict Windows builds and all original baseline hashes pass.

This bridge does **not** execute the complete team creation/activation event
sequence. Source callback3 now has verified bounded team label/raw-field outputs.
Regular-team logos, text updates and class3 actor requests also have controlled
original comparisons. Callback5 refresh and callback6 selected-ID resource
effects remain supplied stages; full callback12 roster/pose setup and the
selected-athlete renderer are unfinished. Full original database/profile
bootstrap, visible navigation/confirmation and field8 handoff remain unfinished.

See [full team screen progress](team_screen_progress.md) and
[selected-player asset trace](team_player_loader.md) for subsequent work.
