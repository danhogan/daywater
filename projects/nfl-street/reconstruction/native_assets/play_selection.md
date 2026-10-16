# Play selection and assignment installation

**Confidence: candidate pending acceptance by the external Windows gate.**
NFL Street, GameCube USA GNNE69 revision 0, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Dolphin build is not recorded in
these captures. The author writes and compiles code and does not execute Windows
programs. Compilation does not establish original behavior or live movement.

`play_selection.c/.h` run the game's script-driven offensive and defensive play
choice, database expansion, and installation into gameplay actors. The native
implementation has no DLL dependency, native stand-in for an original service,
runtime opcode decoder, host playbook, host RNG, or replacement football policy.
`play_selection_native.inc` contains 5,445 source-derived C functions with
original instruction PCs and single-precision rounding points. Its breadth
comes from the original script event handlers, database helpers, actor state
entry callbacks, and their helper closure. Unsupported system instructions and
unknown indirect targets reject with the precise failure diagnostic.

`build_play_selection_native.py` reuses the existing source-construction code in
an independent namespace, extending its roots without modifying shared files.
`play_selection_sources.json` records the disassembly SHA256 hashes, actual
saved indirect callback bindings, uncached source-byte ranges/hashes, and
unsupported instructions. Regeneration needs the private captures, disassembly
cache and Capstone; builds require only committed sources. Never regenerate or
edit pinned sources while a gate is running.

The trace from assignment readers to pre-snap callers is:

| Original | Source evidence / candidate interpretation |
| --- | --- |
| `8003954C` | Actor initialization points actor+310 at backing actor+BE4; not a play choice |
| `80037B88`, `800378C8`, `8003729C` | Pre-snap offensive/defensive choice 0, followed by expansion `800F2718` with flags 3 |
| `800F4E54`, `800F4EE0` | Refresh script features through respective callbacks, interpret actual saved script, execute its choice |
| `800F3FE0`, `800F41C4`, `800F422C` | Build common features and offensive/defensive adjustments |
| `800F46AC` | Original script comparisons, branches, callback dispatch and result types; no guessed heuristic |
| `800F42A8`, `800F4488` | Script actions dispatch weighted choice `800F34DC` or explicit play `800F3484` |
| `8012F930` | Query actual playbook DB, sort candidates, apply `800F6190` offensive/defensive weights, sum u16 weights and choose using the game's RNG |
| `801EE24C`, `80188660`, `80188008` | Actual RNG object callback, scaling/rounding, state update and RNG audit fields |
| `800F33E0`, `800F2718` | Retain selected IDs and expand formation/play DB records |
| `800F946C` -> `800EF234` -> `800EF2D8` | At gameplay handoff, resolve each of seven actor slots, mirror/override assignments, replace first command and enqueue remaining commands, executing original state entry callbacks |
| `8007E650`, `8007E4EC`, `8007DDE8` | Install and enter actor command records; the existing dispatch subsequently reads them |

The common script features include target-score deficit, down, distance to the
first-down target, signed distances to each end of the field, a random value in
[0,100), controller/team identity, and retained play/formation fields. It would
be incorrect to describe the first lane as a game-clock heuristic: `80104350`
reads the target-score option and `80104274` reads score. This captured Street
match is untimed. The checker also edits the source-backed elapsed-tick pair
read by `801EE7F8`; dependence on that timer is determined by the original,
never imposed by the host.

For RNG stream 0 in these captures, the object's callback is `80188008`: its
32-bit seed becomes `seed * 0x19660D + 0x3C6EF35F`, then the original integer to
float conversion, single-precision scale and clamp run. `80188660` scales to
the requested bound with the original rounding/conversion behavior. `801EE24C`
also increments manager+8 and updates manager+C to `(old ^ bound) + 7`.
The implementation transcribes the complete call chain, including its table
lookup; it does not substitute an equivalent host modulo or library RNG.

All memory findings below remain **candidate** until accepted comparison.
Title/hash/region and unknown Dolphin build are recorded above. Scalar
multibyte values are big-endian. The procedure for every row is to load each
pinned capture, prepare identical edited inputs, call native and unhooked
original from identical RAM, and compare every byte outside the oracle stack.
Observed capture pointers: play manager `803CE1C0`, field state `803B6C20`, RNG
manager `803B6AC0`, stream-0 object `803B6B20`, script features `803DD2B4`.
These observations are not fixed-address requirements of the native API.

| Field / chain | Type / size | Reader/writer / interpretation |
| --- | --- | --- |
| `*(r13-5A98) + team*7878 + D1C` | u32 / 4 | `8012E6F8`, expanded selected PBPL database ID; used for logging |
| same team base `+38` / `+D1C` | original DB layouts | Formation / expanded play supplied to `800EF234`; preserved database format |
| `*(r13-7078)+30` | u32 / 4 | `80103964`, down/phase input, source distinguishes 0 and 6 |
| field `+8`, `+C`, `+10`, `+94` | f32 / 4 each | `80103C18`, `801039D4`, `801046F0`, target line, ball XY, field half-length |
| field `+38` | u8 / 1 | `80103C88`/`80103CA0`, possession side / opposite side |
| field `+3C`, `+3E` | u16 / 2 each | `80104274`, team scores |
| `*(*(rng+14)) + 4` | u32 / 4 | `80188008`, saved stream-0 seed; all three captures use RNG type 0 |
| rng `+8`, `+C` | u32 / 4 each | `801EE24C`, stream-0 audit count/hash |
| `*(r13-61F0)+0/+4` | u32 / 4 each | `801EE7F8`, elapsed tick subtraction; not an asserted countdown clock |
| actor `+310` -> command records | original pointer / 4; records / 4 each | `8003954C` establishes link; `800EF2D8` and state-table helpers install/enter commands; `8007E0B4`, AI target/policy routines read them |
| actor `+150/+154/+158` | union byte / f32 lanes | Original entry callbacks initialize their own fields; never overwritten by a host universal-follow rule |

Native API and live connection:

```c
nflstreet_play_selection selection;
nflstreet_selected_play chosen;
nflstreet_play_selection_init(&selection, (nflstreet_actor_ai){span, ram, r13});
if (!nflstreet_play_select(&selection, team, &chosen) ||
    !nflstreet_play_install(&selection, team)) { /* stop and report diagnostics */ }
```

Retain the context while calling. It owns private call temporaries, like the
existing policy provider; gameplay RAM is supplied through the caller's span.
Calls use the existing source-backed SDK caller/scheduler scope, with the same
unmodified SDK lock/unlock calls on the oracle side. No original function is
hooked or skipped. Writes before failure remain; the live startup rejects a
failed operation before entering its update loop.

`native/live/live_main.c` applies this once to `controlled_actor[2] ^ 1`, before
AI initialization and the first update. The opposite-side choice is a host
startup decision; it neither rewrites controller ownership nor selects a play
for the human side. It logs `play selected: <PBPL ID> <name> for team <n>`.
`8012E89C` reads the actual `eman` database field into expanded-play+1F0;
the API copies its bounded 28-byte buffer. The top-level team+10 field is a
formation/type ID and is not used as the play ID. Live CMake includes the
new source with MSVC `/bigobj`, and the build copier includes the new `.inc`.
No protected shared module needs changing.

To select each down, call select then install once at a reproduced pre-snap
transition after updating possession/down/score/field state and before actor
updates. For both AI sides, select/expand offense first, then defense (the
latter's weights inspect the offense), then install both. Refresh existing
movement adapters from RAM after callbacks. The full original huddle, formation
positioning, reset, snap, clock and physics flow still needs reconstruction;
do not call selection every frame or infer a new-down event solely from a
changed down number, since possession changes can retain that number.

Owner-run gate from the worktree root:

```sh
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_play_selection.py
```

Private builds/logs/report:
`/mnt/w/Daywater/research-temp/nflstreet-play-selection-20261016/`.
The gate pins original DLL/input/source/transitive-header/live integration
hashes, runs two fresh processes for contexts 14/15/16, and requires identical
logs. Each process compares 401 saved/edited cases for each team, after choice
and installation, plus 401 standalone RNG cases. Edits vary down, distance,
yard line, score, elapsed ticks, seed and RNG audit fields, preserving actual
DB pointers, script data and scheduler ownership. It requires multiple chosen
IDs and changed installed queues on both teams. All RAM except
`80F38000..80F46FFF` and defined choice/ID/name/RNG results must match.
Eight JSON one-line mutants are built independently: RNG multiplier/audit,
down/distance/yard-line/score features, first-command append, chosen ID.
Only an actual RAM or return mismatch counts; compilation failure, crash,
timeout and native rejection fail the gate. Result lines, exact ID populations,
mutant kills and positive no-input live movement remain for the external runner
to establish. No gate pass or new live-movement measurement is claimed here.
