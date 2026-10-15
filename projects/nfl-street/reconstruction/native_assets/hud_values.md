# HUD value producers (work in progress)

This is a **partial reconstruction**, not a completed live HUD. The native
producer slice and its Windows oracle compile here. Original execution and
mutation results are pending the external runner. No emulator integration or
full HUD verification is claimed.

Target: NFL Street, GNNE69 North America rev 0, DOL SHA-1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`, MEM1 big endian,
r13 `803A9A40`. Saved contexts 14/15/16 are the private inputs. The Dolphin
build used to capture them was not supplied; no new Dolphin run was performed.

## Implemented boundary

- `nflstreet_hud_turbo_update`: `8005D96C`, four records with 24-byte stride.
  The u16 cooldown decrements with wrap, independently of recharge. Recharge
  adds unsigned words and clamps against the unsigned capacity. The used byte
  is copied to its previous byte, then cleared. `80020944`/`8003A4A0` resolve
  original actor references. `8005D768`/`801784B8` decide recharge from actor
  channels and the `4A` substate exclusions. Active controller input paths
  through `8003DCAC` are rejected before any stores; neutral input executes
  fully in the original check. There are no original hooks.
- `nflstreet_hud_turbo_fraction`: `8005D418`, unsigned word conversions, each
  rounded to single before `fdivs`; zero uses the original constant.
- `nflstreet_hud_gamebreaker_fraction`: `80059920`, signed word conversion,
  zero and 10000 special cases, divisor from r13-4E50. The exact constants
  and conversion bias are read from supplied RAM. `gekko_float.h` expresses
  operation order; compile with `-ffp-contract=off`.
- `nflstreet_hud_turbo_publish`: `80119590`, writes the original UIS array
  payload in slot order 2, 3, 0, 1. This is a real saved-screen variable write;
  running its consumer scripts to update geometry remains open.
- `nflstreet_hud_timer_cleanup`: `801D15B0`, removes completed timers by
  shifting later records forward, and promotes newly installed timers to 2.
  This helper does not execute the timers.

All interpretations below are **candidate**, based on static disassembly
and retained RAM until this gate has executed. Even a passing producer gate
verifies only this bounded original-module behavior, not complete game-state
semantics or the complete live HUD.

| Field/path | Primitive, size | Readers/writers and evidence |
| --- | --- | --- |
| `[r13-79FC] + 18*i + 08` | u32, 4 | Turbo amount: `8005D96C` recharge; `8005DA48` spend; `8005D418` normalized reader |
| same record `+10` / `+18` | u32, 4 each | Recharge increment / capacity, consumed by `8005D96C` |
| same record `+14` / `+15` | u8, 1 each | Current/previous usage byte; copied and cleared by `8005D96C` |
| same record `+16` | u16, 2 | Cooldown; decremented by `8005D96C` |
| `[r13-7A18]+08/+0C` | i32, 4 each | Team GameBreaker amount, read by `80059920`, written by `800599B0` |
| `r13-4E50` | i32, 4 | GameBreaker divisor used by `80059920` and `80057110` |
| retained timer object `+A8` array | u32 dimensions and four f32, 24 total | `80119590` publishes after skipping dimensions+1 words; +A8 comes from the saved timer's bytecode |
| UIS screen `+54/+58` | u32 / pointer, 4 each | Timer count/table; `801D15B0`, 52 bytes per timer |

Tests use the saved values as-is, then 400 edited cases per context. Edits
cover amounts, unsigned overflow, capacities, increments, usage flags,
cooldowns, channel keys and substates, unresolved references, both GameBreaker
amounts and divisor, dirty UIS destinations, and timer states. Every case
runs eight consecutive updates, retaining the prior output. After each update
and after getter calls, all 24 MiB outside the reserved oracle stack
`80F38000..80F47000` must match. Scalar return bits are checked separately.
The declared suite has 6,416 all-RAM comparisons, 12,832 turbo fractions and
6,416 GameBreaker fractions per context/process, repeated in two fresh
processes. These are the **required counts**, not an executed result here.

## Gate

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus \
  python3 projects/nfl-street/reconstruction/native_assets/verify_hud_values.py
```

`--build-only` compiles the check and all ten scratch mutants without running
Windows code. The default gate builds/runs each declared one-line mutant and
requires an original value/RAM mismatch, exit 6. Loader errors, rejection,
crashes and compiler errors do not count. Sources (including transitive
headers), protected baselines and saved inputs are hashed before and after.
The report's `full_hud_values_complete` stays false and an executed producer
success is named `passed_producer_slice`.

Private evidence directory:
`W:\Daywater\research-temp\nflstreet-hudvals-20261015`.
Add `--probe` to execute original score refreshes, down refresh and eight
UIS ticks with every routine unhooked, recording dispatch entries and changed
RAM words in `publication-probe-{14,15,16}.log`. This is discovery evidence,
not a native comparison.

## Remaining work and source trace

- Score publication: `80109030` reads the side score through `80104274`,
  formats into r13-5A88, updates descriptor `802ECD90` and sends UIS message
  `8000000B` through `801D49F0`. It is invoked by score-event `8002676C`;
  publishing every render would be a new scheduling choice.
- Down: `801090D8` reads `80103964` (`[r13-7078]+30`) and sends message
  `80000010`. The displayed down/distance wording and its UIS scripts still
  need reconstruction. `80109114` is an event/banner string publisher
  ("1st Down", "Fumble", etc.), not evidence of a numeric distance field.
- GameBreaker writes: `800599B0` clamps [0,10000], emits crossing events,
  calls `80057110` to send UIS `80000012`/`8000002F`, then stores the amount.
  Awards, spends, active-state transitions and message consumers remain open.
- Turbo spending: `8005DA48`, with controller/status/event services, remains
  open. Recharge alone does not implement turbo controls.
- UIS update: `801CF610` plus `801D399C`/`801D16A0`, rates/continuations,
  text layout invalidation and meter geometry. The saved scoreboard's first
  timer calls `80119590` via `80119798`/`8011903C`/`8012A414`. Implementing
  the producer does not execute this VM or refresh the drawn screen.
- Clock, if the active mode has one: `80119620` formats timer slot 4 through
  `800225C8`/`80022630`/`800225F8`. No match clock display is claimed here.

## Integration boundary

No shared file was edited. Add `hud_values.c` to the native app's source
list and include `hud_values.h` in the eventual update integration. Construct
`nflstreet_hud_values c = {span, memory, r13, 0}` using the same MEM1 span
callback, memory user pointer and r13 that the live scene already uses.
Call `nflstreet_hud_turbo_update(&c)` once per 60 Hz gameplay update, after
that update's spending services and before HUD timer scripts. A failure must
stop that update and report `c.rejected_address`; do not silently continue.
The spending services and script tick are not yet reconstructed, so this is
only the placement for the eventual completed update path.

`scene_frame.c` should continue to draw through `nflstreet_scene_hud`; do
not call the producer on each render. Its `uis_draw.c` path needs the future
native UIS tick to consume the updated array and change meter geometry.
When reconstructing the existing external script callback, replace only its
`80119590` case with `nflstreet_hud_turbo_publish(&c, payload)`, where
`payload = array + 4 * (read_be32(array) + 1)` and `array` is the original
callback argument. Call `nflstreet_hud_timer_cleanup` only at the two exact
cleanup points in the future `801CF610` reconstruction, not separately in
`scene_frame.c` or `live_main.c`. **This slice alone cannot be integrated as
a complete live HUD refresh.**
