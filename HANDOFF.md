# Daywater / NFL Street handoff

Updated: 2026-10-13. Latest work: the player draw (80161E10) is native down to its GX calls; before that, both selected lineups load as 14 native visual owners
on `nflstreet-foundation`; see `git log` for the current checkpoint. Read `AGENTS.md` first; this document summarizes current
work and does not replace its instructions.

## Goal and current reality

The goal is a reconstructed/decompiled NFL Street that runs independently on
Windows, without Dolphin, ModernGekko, an ISO or captured RAM at runtime.
Locally supplied original assets and privately prepared data are still required.

**A fresh selected match is not playable yet.** There are two distinct paths:

- The original-game reference uses ModernGekko/Dolphin and original game data.
  It is a comparison target, not the finished native game.
- The independent native reconstruction renders a field, sky, reference players
  and ball, with bounded movement/animation. Native menus retain team, lineup,
  formation and field selections. Fresh selected-player assets now load
  independently, but have not replaced the reference actors in a playable match.

Daywater remains the generic research platform. NFL Street addresses, assets,
behavior and runtime code belong under `projects/nfl-street`, not Daywater's
generic agent/backend code. Preserve the existing Inspect/API and Codex backends.

The current thread has been advancing the original Quick Game menu-to-match
flow, especially fresh appearance ownership and player creation. This connects
to the authorized movement, animation, other-player/contact and ball priorities.
Do not spend the next turn on another small speed experiment.

## How the user wants work handled

- Continue concrete work autonomously; avoid repeated requests to say “keep
  going” or approve routine implementation choices.
- Delegate suitable smaller tasks to smaller models when useful; the user
  explicitly authorized this. Independently check delegated findings against
  source or original execution. Do not accept a failed read as evidence that a
  file is absent.
- Prefer reusable deterministic scripts for repetitive verification/catalog work.
- Notify for meaningful milestones, failures or decisions needing input. Explain
  what works, what remains and why the latest connection matters in plain English.
- Local Git checkpoints are authorized. **External pushes are not authorized.**
- An existing heartbeat is named `continue-nfl-street-decompilation`; do not
  create a duplicate merely to continue. A heartbeat starts another turn; it is
  not evidence that a continuous background worker is already running.
- No user action is currently awaited. No verification job was left running
  after the latest milestone.

## Read these first

1. `projects/nfl-street/STATUS.md` — newest milestone at the top; older paragraphs
   describe historical limits, which later work may have closed.
2. `frontend/roadmaps/nfl-street.json` — user-visible roadmap and current gaps.
3. `projects/nfl-street/reconstruction/native_assets/player_draw.md` — latest:
   the per-player draw `80161E10` reconstructed as the exact GX SDK call
   sequence (lights, instances, materials/TEV presets, textures/TLUTs, vertex
   formats, display lists). 854 original runs per context match every call,
   the render state and all RAM; 20 mutations caught. The module enters
   same-chunk functions directly, so `801626E0` is controlled through its
   cross-chunk helpers. Executing the calls in the renderer remains open.
   Then `projects/nfl-street/reconstruction/native_assets/match_visuals.md`:
   14 simultaneous visual owners from the actual retained menu selection:
   350 slots, 630 material instances, disjoint logical namespaces and deferred
   notifications. Two native processes match single-player owners; no original
   aggregate startup/GPU claim. Body variant 0 and record sides 0/1 are explicit
   fixture inputs. Skeleton/actor/rendering connection remains open.
   Then `projects/nfl-street/reconstruction/native_assets/quick_match_session.md`:
   field confirmation directly creates an owned STAT/GAME roster that survives
   menu buffer teardown, retaining 14 selected rows, 28 formations and field choice.
   The host checks identities after teardown and then exits; no actors yet.
   Then `projects/nfl-street/reconstruction/menus/stat_picker_rosters.md`:
   the native picker now uses loaded STAT membership; new Win32 capture has
   14/14 correct ids. Original fallback projection and native team/field checks
   pass. New output directory preserves older captures. Mirrored teams 1/1 now
   produce separate native GAME lineups after original copy/id verification.
   Then `native_assets/quick_match_roster.md`: selection structs map to GAME
   selected/formation rows and ids; 387 selections and 8,192 original setter
   cases per process. Legacy 12/14 capture is preserved as a negative regression.
   Then `selected_player_visual.md`:
   508 STAT players ×3 body variants = 1,524 complete-record visual assemblies per
   process. The native GAME roster retains its borrowed STAT source; source_rows
   resolves the exact geometry identity and full record. Side input is explicitly
   0 in this corpus; menu wiring, actor/skeleton startup and drawing remain open.
   This supersedes the earlier 402-row controlled-color corpus. Then `player_surface.md`:
   one owner
   builds and releases all 25 slot destinations and their material allocations;
   28 comparisons, 307 failure/reuse checks and 15 record/name builds per process
   across three saved contexts repeated twice. Geometry/actor creation and GPU drawing
   remain separate. Then `slot_materials.md`:
   GX texture sets for the 25 slots, default material settings and the hookup
   onto the 45 player material instances; all 350 live sets and 630 live
   instances match. Then `game_roster.md` —
   the match GAME PLAY table from disc STAT data, selected teams and lineup;
   the live table matches byte for byte. Then `player_record.md` —
   the complete 0x68-byte player record (team colors, color state, side) from
   the match GAME PLAY row; all 14 live records match. Then `palette_recolor.md` —
   color table, dynclut op lists and recoloring; all 350 live slots match. Then
   `slot_textures.md` —
   owned 25-slot texture destinations and the live-match comparison. Then
   `gear_texture.md` —
   the 25-slot gear texture plan, field provenance and supplied services. Then
   `texture_apply.md` and `face_texture.md` in that directory — selected face
   appearance owner, exact evidence and rendering limits.
4. `projects/nfl-street/reconstruction/native_assets/skeleton_startup.md`.
5. `projects/nfl-street/reconstruction/native_assets/headwear_state.md` and
   `hair_postattach.md` in the same directory.
6. `projects/nfl-street/reconstruction/native_assets/player_bound_startup.md`,
   `player_appearance_init.md` and `player_model_binding.md` — actor creation
   exists, but still consumes supplied resource/catalog/service inputs.
7. `projects/nfl-street/reconstruction/menus/quick_game_flow.md` and
   `match_settings.md` — selected menu data and match-start boundary.
8. `projects/nfl-street/reconstruction/native_assets/native_directions.md` and
   `projects/nfl-street/reconstruction/ball/README.md` — movement/ball scope.

The large directory READMEs contain useful history but may have an older opening
summary. Prefer current milestone notes and their verification reports.
`prompts/01_AUDIT_DAYWATER.md`, `02_ADD_CODEX_BACKEND.md` and
`03_NFL_STREET_FIRST_TASK.md` are the historical starting tasks; do not restart
the project at the original score-discovery milestone.

## Latest implemented layers

All files below are under `projects/nfl-street/reconstruction/native_assets`
unless stated otherwise. Implementations are native C; comparison programs load
the original DLL separately as an oracle.

| Layer | Entry points / evidence | Current scope |
|---|---|---|
| Selected roster | `menus/team_player_resolve.*`, `team_player_manifest.*` | Exact PLAY/DIGT+DIGP resolution, normalized names and supported archive lookups |
| Body resources | `body_resource_pool.*`, `model_relocate.*`, `model_chunk_relocate.*`, `model_view.*` | Nine processed base bodies; body variant remains explicit |
| Geometry owner | `selected_player_geometry.*` | Owns selected face/hair files, borrows body; derives headwear through PLAY/GEAR |
| Skeleton startup | `skeleton_startup.*` | Six fresh SKL1 entries, relocation, original position/scale edits, mirroring and six vector outputs |
| Face/hair binding | `face_postattach.*`, `hair_postattach.*`, `model_postattach.*` | Original skeleton-name/cluster/parent remapping with bounded namespace identities |
| Face texture | `face_texture.*` | Named lookup/template fallback, owned raw and processed MMAPv2, inspection plane0, literal texture-request record writes |
| Face application | `texture_apply.*` | Original image/palette copies into owned templates; explicit cache/upload services, not original GPU verification |
| Gear texture plan | `gear_texture.*` | Original `800DE3B8`: GEAR records, player record fields, 25 slot index/palette pairs, color requests, slot0 fallback; team-level record bytes captured |
| Slot textures | `slot_textures.*` | Original 800E2958 layout, 800E2AF0 templates, 800E2CAC/800E27F8 loads; `800D9C0C` recolor reported, not executed |
| Palette recolor | `palette_recolor.*` | 80148A88 color table, 800D8918 dynclut parsing, 800D9948 ops, 800D9C0C composition; its gate feeds captured color state |
| Selected match visuals | `match_visuals.*` | Both selected lineups retained simultaneously;14 players, 350 slots, 630 material instances; no actors/drawing |
| Quick match session | `quick_match_session.*` | Owns STAT and GAME past menu teardown; created by native field confirmation; no actors yet |
| Quick match roster bridge | `quick_match_roster.*` | Menu selection structs to GAME rows/new ids, lineup bits and formation mapping; corrected real picker snapshot produces both mirrored lineups; actor startup open |
| Selected visual owner | `selected_player_visual.*` | Joins geometry and owned surfaces; rejects cross-owner token collisions before uploads; completed selected-match record is caller-supplied |
| Owned player surface | `player_surface.*` | Joins gear/face requests, 25 owned slots, recoloring, binding, configuration and actor material instances; staged failure cleanup; geometry/actor creation still separate |
| Player draw | `player_draw.*` | Original `80161E10` per-player draw as GX calls: light scaling, 12 model instances, LOD-0 eyes, `801C7B58` materials (NULL/OnePass/TwoPass TEV, `801C7AD4` texture/TLUT loads, parameters), `801C7F18` vertex formats and display lists; head compile `801626E0` supplied; no GPU execution |
| Slot materials | `slot_materials.*` | Original `800E2BE8`/`801EBD38` texture sets (GXTexObj/GXTlutObj), `80164DD8` settings and the `801C7528`/`801EBBA8`/`801EBC78` hookup onto the 45 material instances; shader/TEV drawing not modeled |
| GAME roster | `game_roster.*`, `db_fixture.h` (checks) | Original `801E05A4`→`801E1628`/`801E78C4` PLAY copy and `801E7154` renumbering; lineup GILP and team ids are caller inputs |
| Player record | `player_record.*` | Original `800DE000`/`800DE6C8`: STAT TEAM fields, 1AHP, default color state, side; consumes the native GAME PLAY table |
| Joined appearance owner | `selected_appearance_assets.*` | Geometry plus face texture, lifetime cleanup and namespace collision checks; not complete material application |
| Actor creation | `player_bound_startup.*` and its dependencies | Connected factory → constructor → appearance → model binding; resource/catalog/render services still supplied |

Latest face-texture results:

- All 402 regular-roster players load; 15 use the original template fallback.
- Per original process: 1,208 selection/cache cases and 388 complete processed
  MMAP files plus repeat no-ops match. Three saved contexts, twice each.
- Actual original formatting, HSH1 lookup, cache writes and MMAP relocation
  execute. Resource synchronization/diagnostics are supplied.
- Two independent native processes and two joined processes agree. The joined
  owner covers 1,206 assemblies across three explicit body variants, prepared
  face/hair skeleton binding, and three texture namespace rejection cases.
- 13,172,736 decoded plane0 pixels are **native inspection coverage**, not an
  original pixel, palette, material or GPU comparison.

Earlier important checks: complete skeleton startup matches 36 initialized
files, 36 published references and 36 vector words; selected hair binding
matches 831 distinct files/views and 2,493 exact temporary allocation/storage/
release sequences per original process. Connected actor creation matches 3,072
controlled initialization cases, not gameplay frames.

Latest connection: the native picker bootstrap uses loaded STAT rows instead of
fallback db_teams membership when a loaded team exists. All 14 ids in the new
Win32 selection match STAT; source field confirmation/back/notification checks
and original fallback projections pass. Older captures are preserved. Mirrored
copying is now verified against the untouched original loader for all 43 STAT
teams: source-id mapping is scoped to destination team, with distinct GAME ids.
The bridge accepts the corrected real picker snapshot and preserves both
lineup/formation maps; legacy wrong-team ids still reject. Read `menus/stat_picker_rosters.md` and `quick_match_roster.md`.
The later `quick_match_session` owner connects creation directly to field
confirmation and survives menu teardown. Current private proof is
`W:\Daywater\research-temp\nflstreet-menu-match-owner-20261013\verification.json`.
No fresh actors or playable match have been created by these milestones.

Previous connection: `nflstreet_selected_player_visual_from_match` consumes native
GAME rows, resolves their exact source STAT identity, builds complete records
and opens geometry/surface with those colors. All 508 STAT players pass with
three body variants. Source provenance rejects a different database; source
bytes must stay alive and unchanged. Local db_teams membership differs from STAT
and is an original fallback path; do not substitute it for loaded STAT rows.
The prior controlled-color limit is superseded. Read `selected_player_visual.md`.
Menu/lineup wiring, skeleton/actor startup and drawing remain unfinished.

Previous connection: `nflstreet_player_surface_from_player` consumes a completed
player record and normalized roster name, builds its gear/face requests, loads
and recolors all slots, and owns their texture/material allocations. Its gate
checks composition against the source-matched original-stage evidence, callback
and allocation failures, and cleanup. Read `player_surface.md`. Connect this
owner with geometry and actor creation next; do not claim fresh player drawing.

Previous connection: `nflstreet_actor_materials_hook` reproduces actor startup's
material hookup: the 45 descriptors of `8028DD10` take their slot textures on
stage "base", and the tattoo slots switch eight arm materials to TwoPass. All
630 live instances of the 14 captured actors match, and the original sequence
on fresh arrays matches 819 controlled hookups. Read `slot_materials.md`.

Previous connection: `slot_materials.c` binds the owned slot textures:
`800E2BE8`/`801EBD38` GX texture sets in the `80276C6C` table, and the actor-startup
`80164DD8` default settings. All 350 live sets match except draw-time
`801C7AD4` writes. Per original process, 20,475 selected sets, 513 synthetic
MMAPs and 883 settings runs match. Read `slot_materials.md`.

Previous connection: `game_roster.c` builds the match GAME PLAY table from
disc STAT PLAY/THCD/TEAM, the two selected teams and the lineup GILP values.
The live table is rebuilt byte for byte and the 14 captured records follow.
87 original match-start loads per process (rerun in the snapshot on emptied
GAME tables) match. New DIGPs come from the THCD pass before PLAY. Read
`game_roster.md`.

Previous connection: `player_record.c` completes the per-player record. All 14
captured records rebuild byte-identically from their GAME PLAY rows (disc STAT
TEAM/LCGP/GEAR), and 4,318 original `800DE6C8` calls on the live databases
match all 104 bytes per process. The match's player rows are GAME rows
(renumbered copies of template `00002` STAT rows), not the `db_teams` blocks
used as the gear corpus. Read `player_record.md`.

Previous connection: `palette_recolor.c` completes slot textures. All 350 live
slots of the 14 captured players are byte-identical; the original
`800D9C0C` matches 4,163 compositions and 3,072 op sweeps per process.

Previous connection: `slot_textures.c` loads the planned slots into owned
destinations. The original pipeline matches 819 players per process, and the
14 captured live players match outside palette record 0 data (69 slots await
`800D9C0C`). Gear plans equal the live `800E2E48` records in all 336 non-face
slots. Read `slot_textures.md`.

Previous connection: `gear_texture.c` reconstructs original `800DE3B8`. Per
original process, 930 complete plans (402 roster rows × 2 captured team
contexts, 14 captured records, 112 controlled variants) match every
`800E2E48` slot write (24,180), `800E217C` color request (26,538), GEAR search
(11,160) and the `+65` byte. 564 GEAR records match original `8014B918`;
12,462 player-record fields match `8014F60C`/`800DE000` getters/`800DDEBC`/
`800DDF44`; `800DDBE8` and `800E3218` are swept completely. Eight deliberate
faults are rejected. Three faults in the earlier uncommitted draft (CFRP vs
IKSP at `+40`, missing GEAR text terminator, missing slot 18–22 seeding) were
fixed. Read `gear_texture.md`.

Previous connection: all 402 selected faces match complete original
template destinations after image/palette application and reapplication, with
2,412 ordered cache requests per original process. The joined owner retains
1,206 applied destinations. Read `texture_apply.md`; cache/GPU effects and
remaining palette composition/material consumers are still explicit boundaries.

## Next concrete work

Continue from `selected_appearance_assets`, rather than rebuilding asset loaders.

1. GAME roster, gear plans, slot loading, recoloring, material binding and
   the complete player record are done; do not redo them. Next:
   - Draw selected players: the GX submission (`player_draw`) is done.
     Execute it in the independent renderer. Map TEV presets, texture
     objects to the owned slot destinations (decoded MMAP planes and
     recolored palettes), the light channel and the display lists with the
     skinning matrix palette. Fresh actors must supply the instances,
     matrices (`80162760`) and LOD/draw flags (`80164000`).
   - Native menu choices now create an owned GAME roster directly via
     `quick_match_session`; original lineup-bit decisions and mirrored copying
     are checked. Feed its selected rows into fresh visual/skeleton/actor owners.
     See `quick_match_session.md` and `quick_match_roster.md`.
   - Note: the original DB layer is a statement interpreter (`801B4510`). A
     snapshot-hosted original run works once synchronization returns and
     `r13-6354` marks the `db_teams.dat` open done.
   Do not infer field meanings or choose variants from filenames alone.
2. Direct `80164CE4` image/palette application is now reconstructed and
   verified; do not redo it. Connect remaining composition/material consumers.
   Useful source routines:
   `800E27F8` (texture acquisition/application), `80164CE4` (image/palette copy),
   `801C588C`, `801C58C0`, `801C5958`, and later composition helpers.
   `800E3104` is the verified named face path; `800E318C` is a separate cap path,
   which the new face owner does not implement.
3. Establish fresh resource-pool/descriptor activation and constructor inputs,
   then supply owned selected assets and prepared skeletons to
   `nflstreet_player_bound_create`. Remove supplied reference dependencies one
   explicit owner at a time; do not replace callbacks with silent success stubs.
4. Verify a selected actor from fresh assets, then render it through checked
   material mappings. Connect selected actors, field and ball to native match
   startup before claiming the menu starts a game.

Original `80041378` is another match-player creation trace point. The existing
bound-startup notes explain descriptor requirements and unresolved services.
Keep movement/contact/ball scheduling limitations visible while doing startup.

## Environment and private storage

Repository: `/home/dan/daywater` in WSL. Windows storage:
`/mnt/w/Daywater` corresponds to `W:\Daywater`.

- Assets: `/mnt/w/Daywater/private/nflstreet/GNNE69-r0/assets/`.
  Relevant archives: `plaface.dat`, `plarosterhair.dat`, `plahair.dat`,
  `plamodel.dat`, `platex.dat`, `animdata.dat`, `db_teams.dat`.
- Roster DB blocks: the same root's `unpacked/db_teams/00001.bin` through
  `00032.bin`. GEAR table: `unpacked/template/00002.bin`.
- Prepared private catalog: `native/skeleton-startup.nsc1` under that root.
  Exported by `export_skeleton_startup_catalog.py` from the hash-checked DOL;
  native startup reads this data file, not a DOL. Do not commit its contents.
- Saved oracle contexts: `/mnt/w/Daywater/research-temp/position-neutral-14.mem1`,
  `position-neutral-15.mem1`, `position-neutral-16.mem1` (24 MiB each).
  Their source Dolphin build was 2603a.
- Latest report: `/mnt/w/Daywater/research-temp/nflstreet-slot-textures-20261012/verification.json`
  (gate `verify_slot_textures.py`). Gear plan report:
  `nflstreet-gear-texture-plan-20261012` (gate `verify_gear_texture.py`). Both
  use private catalog `native/gear-texture.gtc4`. The stale outputs
  `nflstreet-gear-texture-20261012` and `native/gear-texture.gtc1/2/3` are
  superseded and unused.
  The face application report remains under `nflstreet-texture-apply-20261012`
  and the face-texture report under `nflstreet-face-texture-20261012`.
  Other reports use `nflstreet-skeleton-startup-20261012`,
  `nflstreet-hair-binding-20261012`, `nflstreet-headwear-state-20261012` and
  `nflstreet-hair-resources-20261012` below `research-temp`.
- Source listings: ignored local
  `cache/binaries/46e3de854998a0f66c9be8c72a1164d22cd29eb4/decompiled/` and
  `disassembly/`, with lowercase address filenames such as `800dd720.txt`.
  Use actual DOL disassembly when a cached decompilation is ambiguous.
- Windows compiler: `/mnt/w/Daywater/tools/zig-x86_64-linux-0.16.0/zig`.
  Cache: `/tmp/daywater-zig-cache`.
- Oracle ABI headers:
  `/mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/include`.
- Movement inspection launcher: `W:\Daywater\Start-NFLStreet-MovementPlayer.cmd`.
  WASD/arrows support eight directions, braking and restart. It uses reference
  player resources; it is not proof of fresh selected-player startup.
- Daywater README documents web UI `http://localhost:7860` and Inspect viewer
  `http://localhost:7575`. Their live availability was not checked during this
  handoff. The web UI is a research/roadmap interface, not the native game.

In this session, default shell execution sometimes failed with ENOENT. Explicit
`/bin/bash`, `login:false`, and an authorized escalation worked. W: is outside
the default writable roots, so writing/building there may require escalation.
Do not copy bulk assets into the repository or C: to bypass that restriction.

## Protected original baseline

Use `menus/verify_controller_checks.py::baseline_hashes()` to check these:

| File | Expected hash |
|---|---|
| `W:\Daywater\research-temp\nflstreet-native-windows-local\game\sys\main.dol` | SHA1 `46e3de854998a0f66c9be8c72a1164d22cd29eb4` |
| Same root, `gGNNE69_recomp.dll` | SHA1 `dd521cc38b0db6bca98a0bb261570a2be5e6e0ed` |
| `W:\Daywater\tools\ModernGekko-Template\lib\ModernGekko\tools\moderngekko_run.cpp` | SHA256 `3f9e3a3eb4b5728d9036e9552c01e8c9dc084a1f53805fef95dcf3b1b86f73ea` |

Recent work did not replace or patch these files. Preserve their hashes; if an
experiment changes a baseline source/DLL, restore it and verify restoration.

## Verification workflow

Run from the repository root. The scripts build Windows executables and run
them through WSL; outputs and diagnostics go to W:.

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus \
  python3 projects/nfl-street/reconstruction/native_assets/verify_face_texture.py
```

Latest gate: `menus/verify_team_field_picker.py --output-dir /mnt/w/Daywater/research-temp/nflstreet-stat-picker-20261013 --live-window`. Bridge gate: `verify_quick_match_roster.py`. Visual gate: `verify_selected_player_visual.py`. Surface gate: `verify_player_surface.py` (requires current slot-stage evidence). Stage gate: `verify_slot_textures.py` (about 2 minutes). Gear plan gate: `verify_gear_texture.py`. Face application gate: `verify_texture_apply.py`.
Other targeted gates in that directory:
`verify_skeleton_startup.py`, `verify_hair_binding.py`,
`verify_headwear_state.py`, `verify_hair_resources.py`,
`verify_selected_player_resources.py`, `verify_face_bound_resource.py`,
`verify_model_postattach.py`, `verify_skeleton_resource.py`.
Find the relevant actor/menu verifier in its milestone note before changing
those layers. Build scripts generally require a private W: output directory.

Do not rerun every gate after every edit. Run the new gate and affected existing
ones. Verify that the final report is `passed`, its source hashes still match
the final code, and protected/private input hashes are unchanged. Shared check
harness edits can invalidate earlier reports even when behavior is unchanged.
`verify_player_startup.py::source_hashes()` tracks quoted include dependencies;
explicitly include separately compiled C files in verifier fingerprints.

Evidence must distinguish native corpus coverage, controlled original-stage
comparisons and live gameplay. A ModernGekko oracle run is not a new live Dolphin
integration. Use `candidate`, `probable`, `verified` with the tested scope.
Record primitive types, widths, endianness, source functions, procedure,
observed values, revision/build and limitations for verified memory findings.
Opaque namespace identities chosen by a checker are not discovered game addresses.

## Git and privacy

The initial handoff was committed as `8cd3aa4`. It is updated alongside later
project milestones; inspect current status instead of assuming a clean tree.
All project checkpoint commits are local; nothing was pushed.

Recent checkpoints, newest first:

- `f25ce4d`: owned selected slot textures.
- `4d4f8b8`: selected gear texture plans.
- `a253c65`: original face image/palette application.
- `36c9982`: owned selected face textures and joined appearance assembly.
- `4a1f9aa`: complete skeleton startup and prepared geometry binding.
- `db1445f`: selected hair binding to fresh target101.
- `0e0bb04`: automatic headwear and joined geometry ownership.
- `ec09d0d`: selected hair plans and processed models.
- `9e348d4`: fresh skeleton loading and connected face binding.
- `45a5ae5`: selected roster face/body resources.
- `8a40a5a`: actor factory/model binding.
- `d62e963`: earlier reconstruction/roadmap checkpoint.
- `b3a7221`: generic Codex backend/workflow changes.

Inspect current status before editing; other threads may add work. Do not reset
or discard their changes. Before a local checkpoint, stage explicit intended
files, run `git diff --check`, inspect staged content for binary assets/secrets,
and validate relevant reports against final source. Repository code, research
addresses/hashes and documentation have been committed; original assets,
captured RAM, decoded pixels, private catalogs, generated dumps and credentials
must remain untracked on W:. Do not treat an ignored file as permission to move
it into a tracked directory. No external publication or messaging is authorized.
