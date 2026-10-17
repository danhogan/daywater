# Native match setup: replace the live MEM1 resume boundary

Planning audit, 2026-10-16, branch `codex/setupplan`, baseline `f6cdade`.
Only this document is changed. No constructor, oracle, Dolphin session or live
window was executed for this audit.

## Target and evidence vocabulary

Start with the game's Quick Game choices (controllers, teams, seven-player
lineups, formations, venue and rule/configuration values), load data from the
owner's disc, construct fresh state, and enter the existing native update/draw
loop. Preserve the snapshot launch as a regression mode during implementation.
A menu-selected team must change the actual actors and GAME records; a selected
venue must change the environment, collision/spatial data and lighting. Loading
new geometry while retaining the saved match's gameplay is an intermediate check.

Provenance for addresses below: **NFL Street, GameCube GNNE69 North America
revision 0**, DOL SHA1 `46e3de854998a0f66c9be8c72a1164d22cd29eb4`, MEM1
`[80000000,81800000)`, r13 `803A9A40`. Multibyte guest integers, pointers,
float32 and matrix words are big endian; byte fields have no endian qualifier.
Ranges are half open; offsets and addresses are hexadecimal unless explicitly
called counts. Existing constructor/asset documents identify Dolphin **2603a**
for position-neutral captures; some newer gameplay documents say the build is
unavailable. Retain that provenance discrepancy in a new capture manifest.

- **Verified** means the cited module's documented original comparison passed
  within its stated boundary. Native aggregate composition checks are identified
  separately: they do not establish complete original startup.
- **Probable** means static source/disassembly or repeated asset-layout evidence;
  it does not establish an executed startup path.
- **Candidate** means a proposed connection or an observation needing reproduction.
- **Host choice** describes implementation/lifetime/scheduling decisions, not a
  claim about the original game. Unlocated allocation owners remain unknown.

Repository paths below are relative to `projects/nfl-street/`, except where
absolute. `N/` means `reconstruction/native_assets/`, `M/` means
`reconstruction/menus/`. Original routines were read from
`/home/dan/daywater/cache/binaries/46e3de854998a0f66c9be8c72a1164d22cd29eb4/`
`decompiled/<lowercase-address>.txt`; use sibling `disassembly/` to resolve ABI,
float order and decompiler omissions. `callgraph.json` misses indirect callbacks.

## 1. What the running app inherits

### Entry and actual module closure

`native/live/live_main.c:main` allocates 24 MiB and reads the complete image,
defaulting to `position-neutral-15.mem1`. `venue_present` checks EARS model
names; `hud_present` checks all fourteen saved FNTG fonts. `scene_fixture` then
builds contexts from saved pointers and fixed object addresses. Neither check
creates anything. `NFL_LIVE_DISC` is opened **after** this setup; currently it
loads animation files and is closed before gameplay begins.

The authoritative linked closure is `native/live/CMakeLists.txt`, together with
`native/live/live_ai.c`, `live_clips.c`, `gx_aurora.c` and included generated
native files. Some linked modules have dormant entry points: linking a lifecycle
kernel does not mean its fresh initializer is called. The active order is:

1. Resume scene; identify the controlled actor by distance to saved ring 0 and
   a RAM scan matching gameplay actor +04 and position. This is a **host lookup**,
   not original controller assignment.
2. Map animation using the saved registry; select/install the opposite team's
   play; initialize AI, down policy, down cycle and controlled input adapters.
3. Fixed tick: rings; AI dispatch/commit/movement; controlled input/movement and
   clip requests; manager pass for all actors, channel step for all actors, then
   poses/twist/compact skeletons; actor placement; ball motion/visual/matrix;
   down result/queue/next-play/lineup/snap; turbo producer.
4. Draw: field camera/projection, lights, sky/clouds, passes/marks/shadows,
   lines/rings/field, crowd/props/fans/ball; name projection/palettes/player draw;
   overlay/strips/particles; HUD camera, names and UIS screens.

The loop supplies one game tick per 60 Hz update; bounded catch-up, one-second
snap input, camera follow/orbit and GPU depth-readback latency are host choices.
It now selects plays and advances downs; older `native/live/README.md` sections
saying the AI cannot move or only follows the saved play are historical. Fresh
play choice still reads saved database/script/RNG/configuration owners.

### Snapshot dependency and allocation ledger

The following covers each active module family in that closure. It is a static
object/dependency inventory, **not** a claim that every byte in a listed object
is read on every frame. Indirect graph extents vary with models, banks, scripts
and branches. New constructors must publish bounded allocations and prove the
reached-read inventory; no universal contiguous "match blob" has been found.
Observed context-15 pointer values in this table were read directly during this
audit and are **candidate reference observations**, not newly verified fields.
The cited module documents supply existing verified field procedures/values.

| Subsystem / readers | Snapshot structures, roots and ranges | Original allocation or producer; replacement boundary |
| --- | --- | --- |
| Scene/venue: `scene_frame.c`, `scenery_draw.c`, `marks_draw.c`, `field_overlay.c` | Main field `8094D31C`, `B58` bytes; five-pass owner `80A9753C`, `4DC` bytes. Main model via object+78; pass models at +3D8. Four line objects `[80DEC17C,80DEC60C)`, stride `124`; rings `80DDFCD0`, `80DE01E4`, `80DE02E8`, each `104`. Contexts also read pass table `8028A004`, pass offset table `8028A090`, sky `80385E8C`, color/table arrays `80310A78`/`8038668C`; ground-mark list roots `802AA7C4`, counts `80271268`, handlers `8028CCB4`. Overlay gates include byte r13-71C0. | **Probable original ownership:** `80004E4C -> 8002727C` selects/loads environment; `800D9D88` registers type 6 (`B58`), creates scene object and publishes r13-7268, then `800D9CF0 -> 80165A50` binds field. `8015B748` registers type 7 (`4DC`, constructor `8015B3C0`), creates/publishes r13-68A0. Rings type 8 use `800E3DC8`; line/mark startup still needs a complete write audit. `80195400` registers types, `801957A4` creates/inserts, `801958E8` orders nodes. Saved addresses are allocation outcomes. |
| Spatial world: ball motion, AI, down rules and venue effects | r13-7054 = `80921B20` in context 15; also r13-7058. Version-105 environment header: XY vertices +14, triangles +1C, spatial tree +98; instance arrays include counts/pointers +58/+5C, +68/+6C, +70/+74 and corresponding 24-byte name tables. | `8010870C` acquires chosen `envrnmt.dat` entry through `801A69C0`, publishes both roots, calls `80108320` relocation and `80108278`. `environment.c` parses selected records but does not own the full runtime tree/BSP. Copying preview triangles cannot replace this dependency. |
| Models/display lists: `draw_engine.c`, `model_view.c`, `player_draw.c`, `cluster_palette.c` | Relocated TMdl files, 52-byte views (chunk/material/array/group pointers), 24-byte chunks, 12-byte vertex-array rows and GX display-list bytes. Player instances are `F0` bytes: model +0C, selector bytes +1C, chunk count/list +5C/+60, palette +E0, material table +E8. Visual instance slots include +4AC, +59C, +68C, +77C, +86C, +95C, +A4C, +B3C, +C2C, +D1C, +E0C, +EFC. Draw record +4A8 controls flags/LOD and held-item models. Shared morphed head view `803102B4`. | `801C5EC8` relocates chunks; `801EB3F4`/`801EB6C4` provide model views; `80162FC4`/`801EBFE0` initialize/select instances. `801639C8 -> 801630B0` constructs/binds player rendering. Current `scene_face` always returns compiled=false and consumes existing head geometry/lists; it never compiles a fresh head. Fresh selector/LOD/slot mapping and complete material publication are still required. |
| Textures/shaders: `draw_engine.c`, `gx_texobj.c`, `gx_aurora.c`, UIS | Each material points to shader, parameters, texture and palette objects and tiled payload bytes. Default texture r13-640C = `80348A54`; null shader r13-5684 = `80348AB0`. Player records `[802AAA60,802AB010)`, stride `68`; slot table `[80276C6C,802799EC)`, 14 x 25 x `20`. Shader setup tokens `801C9D0C`, `801C9DAC`, `801C9DFC`, `801C9D5C`. Bridge decodes texture/TLUT SDK words, including packed physical image pointers, then uploads referenced MEM1 bytes. | `800E2BE8 -> 801EBD38 -> 801C8E80` allocates texture sets/entries/GX objects; `80164DD8`, `801C7528`, `801EBBA8`, `801EBC78` configure 45 materials/player. `slot_materials.md` documents allocation sizes and intentionally preserved allocator bits. Host semantic materials exist; renderer-compatible pointer arrays/shader objects still need serialization. Generic fresh resource support must also handle venue/UI shaders, not assume every asset is a verified player-body case. |
| Visual actors/rosters: `actor_world.c`, `actor_palettes.c`, `actor_scale.c`, `actor_dynamics.c`, `actor_dynamics_math.c`, quaternion kernels | r13-7BBC = list `80BCD300`, fourteen BE32 visual pointers. r13-7BB8/7BB6 are adjacent u16 counts (14/7); do not read the combined word as one count. Type-zero visual objects are `1A90` bytes; examples `80B87D18` and `80B7F848`. Source skeleton/model channels +60/+64, +BC/+104, +194/+210, +70/+34C; world/root +3AC, scale +18, render light state +1A4C..+1A8F. Player GAME id +135C, slot +1363, player record drives appearance. | `80041A08` registers visuals, loads skeletons and calls `80041378`. That routine allocates the pointer list with `8018B7B8(count*4)`, gathers/sorts resource requests, forms fresh descriptors, calls `801956A4` or scene-linked `801957A4`, inserts at order 9, attaches name tags and loads appearance. `801956A4` clears full storage; constructor `800E1F7C`'s own memset has zero length. Pool registry r13-4F14 = `806A16C0`, record stride `14`; type-zero size `1A90`, ctor `800E1F7C`, allocator-owner token `80B7F820`. Native factory uses a supplied allocator, not this pool implementation. |
| Gameplay actors/movement: `live_ai.c`, `live_movement.c`, `command_to_movement/{actor_runtime,actor_update_replay,preparation_replay,command_to_movement_replay,reset_acceleration_replay}.c`, `input_translation/input_translation_replay.c`, `kinematics/kinematics_replay.c` | r13-7C30 = descriptor `803B7300`, base +00 = `803B7320`, u16 side width/count +08/+0A = 7/14. Array `[803B7320,803C96B8)`, stride `14D4`. Each row: header +00, visual +04, controller byte +08; target union +150, position/motion +1A8 (21 words), inputs +200 (12 words), heading +1C0, ratings signed16 +BB4/+BBC, flags +408, attachment matrix +280, queue +BE4. Current AI seeds movement from those saved values; controlled path uses the full down-policy closure. | `80039D30` registers a `C`-byte descriptor via `801EF08C`; its lifecycle callback `80038B8C` allocates/clears count*`14D4`, creates handle/index storage (`800CC504`), then calls `8003B05C`, `8003954C`, queue initialization `8007DF30` and related helpers for every actor. This is a distinct object from the `1A90` visual: reconstructing the visual factory alone does not create movement state. |
| Animation assets/channels: `actor_channels.c`, `actor_pose.c`, `actor_skeleton.c`, `compact_pose.c`, `pose.c`, `clip_requests.c`, `channel_callbacks.c`, `live_clips.c`; `actor_target_input/{asset_loader,animation_channels,locomotion_selection,command_timing,event_signals}.c` | Gameplay manager +9BC, set +4D8, ten `7C`-byte channels at +4E4; descriptor links +318/+31C/+320. Bank count r13-4F70 = 91, table r13-4F6C = `80B4E280`, stride `1C`, header +0C/cache +18. Pose list `8032BF28`, resources `8032C34C`, curve/callback tables r13-6714/-6718/-6734, operand cursor r13-4F58. Stream cache reference `80B40BE0`, `14`-byte rows, double read buffer `8030B8E0`, selector r13-5968. Frame words are ARAM addresses or `F0000000 | cache-slot`. | `80016278` opens animdata, calls `80173B10`, installs event/operand/relocation/release callbacks and initializes cache via `80174FD8`; `8001638C` and later requests populate it. Constructor model channels allocate count*64 transform bytes and 384-byte channel blocks (`actor_model_init.md`). `80041108` builds six SKL1 resources. Actor/set/clip activation, cache/LRU records and bank selection must be produced before live channel updates. |
| Disc stream adapter and optional host pose: `disc_files.c`, `archive.c`, `lzh1.c`, `anim_stream.c`; fallback `live_pose.c`, `live_locomotion.c` | Disc supplies `animdata.dat` or decoded virtual `animdata/NNNNN.bin`. `nflstreet_anim_aram_map` still reads the saved registry, relocated headers, frame tables and cache words to derive each bank's ARAM base. It is not a registry constructor. Explicit host fallback additionally reads NSC1 set catalog from `NFL_LIVE_SETS`, generated from RAM, plus bank 111. | nod/TERF buffers are host-owned and survive disc close. `startup.c` discovers direct context-one banks into an owned registry but explicitly does not reproduce match bank-selection policy. Neither it nor the optional host pose synthesizes fresh gameplay channels. Replace the saved mapping with initialized registry/cache state or a separately verified native file mapping; keep fallback catalog out of the fresh startup contract. |
| AI/play/down state: `actor_ai.c`, `actor_ai_dispatch.c`, `ai_policy.c`, `play_selection.c`, `down_cycle.c`, `locomotion_predicates.c`; `actor_target_input/{actor_target_input,target_selection}.c` | Actor +310 points at backing +BE4 command records; +318/+31C/+320 at manager/set/channels; +324/+328/+32C link other backing blocks. r13-5A98 is play manager (`803CE1C0` observed), two team records stride `7878`, formation +38, expanded play id/data +D1C. Saved script features (`803DD2B4`), playbook/formation DBs, callback tables and RNG stream owner (`803B6AC0`, stream object `803B6B20`) remain inputs. Phase owner r13-7CF8 = `803CD9A0`; current/previous words +0/+4, cache r13-7CF4. Event queues and source-backed SDK lock/scheduler state are also transitive reads of generated closures. | `8003954C` installs backing links; `800F2A94` is an entry-time play subsystem boundary requiring callee audit. Existing choice/expansion/install and down-cycle implementations consume initialized databases/queues/RNG; they are not those owners' startup. Original phase table `80245A80` has entry/update/exit callbacks. `8002E45C` performs transitions; a fresh start must run original entry effects rather than write a guessed phase. |
| Field/rules/score shared by AI, movement, ball, down and HUD | r13-7078 = `803B6C20`, original registered storage size `298`. Target line f32 +08, ball/spot XY f32 +0C/+10, down u32 +30, possession/mirror bytes +38/+39, scores u16 +3C/+3E, geometry terms +90/+94, original-down possession byte +A4. r13-61F0 points at two tick words; it is not automatically a countdown clock. Config cache `8010B60C` and FNIG values are read by downstream closures. | `80103624` registers/initializes `298` bytes through `801EF08C/084/0F8`, reads config and possession, zeroes ordinary-mode scores and sets down 1; its continuation `800FEA5C` and later field/config writers matter. `80004E4C` later reloads scores using queries and `80104294`, so initializer equality alone does not establish the final match state. |
| Ball: `ball_update.c`, `ball_update_motion.c`, `ball_handoff.c`, `ball_draw.c`; `ball/{ball_frame,ball_attachment,ball_orientation,ball_state,ball_ownership,ball_lifecycle}.c` | r13-73D0 = pool `803B6F00`, count byte +0, records +8 stride `F4`; first `[803B6F08,803B6FFC)`. Position +0C, quaternion +18, history +28, velocity +54, force/angular blocks +70/+7C/+88, state +A4, association header +B4, history +CC. Visual pointer +00 = `80DEEE24`, type-3 `2A4` bytes; model/light/root fields and spatial triangle +294. Shadow count r13-6834, entries `80310898`, texture `80310A58`, LOD `803897D8`. | `800BE848(1)` registers `8+F4` pool, installs `800BD410` init callback; that clears records, constructs handles/quaternions and index storage. Later `800BDE90 -> 800D22AC/800D233C` registers type 3 with constructor `800D1F90` and creates each visual, then publishes active ball. Current bounded motion/ownership calls do not allocate these. Fresh setup must also provide spatial tree, light rig, actor handles and first phase/association through actual original transitions. |
| HUD/UIS: `hud_values.c`, `hud_draw.c`, `uis_draw.c`, `gx_texobj.c` | Name-tag array `[8091C568,8091C760)`, fourteen `24`-byte records; each +14 names an FNTG font. Text state/boxes `8039C480`/`8039C4BC`, TLUTs `80331460`/`80331480`. UIS manager r13-63DC = `808AE4C0`, screen camera r13-6CD8 = `808AE484`; four screen roots r13-6C84/-6C94/-6C88/-6C8C, pending list `8030834C`, tint `8034CDB8`, temporary light/texture/TLUT `8039E700/740/780`. Turbo root r13-79FC has four `18`-byte records; amount +08, recharge +10, used/previous bytes +14/+15, cooldown u16 +16, capacity +18. GameBreaker root r13-7A18. | `800DB284` queries name data; `80041378 -> 800DB004/800DB0D4` creates/attaches names. `8012BBD8` prepares in-game UI resources; `8012B9DC` contains screen-owner publication. `8005D48C` registers turbo state via lifecycle services. Menu class/source construction exists, but fresh in-match UIS trees, fonts, script bindings, score/down messages and refresh state are separate owners. Draw and turbo-fraction functions cannot construct them. |
| Cameras: `camera_slots.c`, `camera_projection.c`, `name_tag_project.c`, camera portion of `hud_draw.c` | Nodes `[806A1BA0,806A1EB8)`, three `108` strides: overlay `806A1BA0`, field `806A1CA8`, HUD `806A1DB0`. +40 points to camera data (field position +04 and angles +14/+18/+1C). Type dispatch r13-5780; slots `[80331EC0,80331FC0)` (four `40`-byte matrices), viewport floats `8039E584`, cull projection/modelview `80348AD4/80348B14`. Active node r13-4EEC and screen-camera publication are mutable. | `80004728 -> 801DF14C` allocates view nodes from owner r13-638C via `80180700`, configures viewport, stack/camera and scene link. `80004830 -> 801954E4` creates the scene owner. Native view/projection/slot updates are verified consumers; camera type instances, data allocation and initial callback lists are not created by `scene_fixture`. Host `steer` is not original camera logic. |
| Lights: `light_rig.c`, lighting in player/ball draw | Active rig pointer `8034FD88`; shadow projection/color storage `8034FD8C`/`8034FE8C`; sets at `8034FE9C`, stride `108`; rig lights stride `30`, set members/pointers/scales and callbacks. Sun vector `802A3030`, shadow cap r13-6228. | `8002727C` copies venue-specific sun values from DOL tables. `8015B928 -> 801ED408` installs rig and runs update. `80004870 -> 801ED47C -> 801DF39C` registers update on field-camera phase 1. `801ED3A0` is reconstructed per-frame math; initial rig selection and material/light registration still need fresh ownership. |
| Crowd and effects: `crowd_draw.c`, `effects_draw.c`, `cloud_draw.c`, `select_rings.c` | Crowd/props `[80C4E094,80C53440)`, 45 `1DC`-byte objects; model/skin record +128, extra state +1D0, spatial/light +144, optional fade +13C. Crowd gate r13-72A4; side root r13-5BA4 = `803B72C0`. Strips use visual history +1368, global history `80270CA4`, gates r13-72C0/-7238. Particles use r13-730C list `80D467C0`, u16 count at r13-7310, shader/corner data. Cloud records `80385C8C`, count r13-7284, texture `802AA7A4`, sky record `8030FEB4`, enable bytes r13-71E3/-71E4. Rings update consumes controller-to-actor state, not only ring geometry. | `80004870 -> 800D2CC4`, `800D39B4`, `800E6418`, `800D31F8`. `800D31F8 -> 800D2F60` resolves definitions, creates type `1A` via factory (`1DC`, ctor `800D2E94`), attaches scene order 6, instantiates model state (`800D3A70`), registers behavior and spatial lookup. `800D0830` separately allocates flying-object/effect arrays; it must not be mistaken for all crowd initialization. Live draw routines consume this state; they do not create emitters or the 45 objects. |
| Matrix stacks/palette pools: `matrix_stack.c`, all placement/camera/skin/mark/ball paths | Stack table r13-65F0 = `806A0CC0`, count r13-65EC = 12; descriptors `C` bytes with used/depth/slot pointer; `40`-byte matrix slots, including popped scratch. Current descriptor word r13-65E8, viewport stack index byte r13-4EE8. Palette pool r13-5680 = `80742DA0`, index r13-5678 = 1, used r13-5670 = `7B8` in captured frame; descriptors/matrix destinations and cache flag r13-6200. Sine coefficients `8039E100`, unit words `803A3958/5C`. | `8018908C` allocates `(requested+8)*C` table via `8018B7E4`, clears descriptors and calls `80189664`; per-stack slot allocation and palette-pool constructors still need their own audit. Current `nflstreet_scene_start` resets only used count, and palette math allocates from an existing pool. Host-owned palettes in `match_*` use a different explicit namespace/capacity contract. |
| GX shadow state: `gx_aurora.c`, `draw_engine.c`, clouds/overlay | r13-4C90 = `80352E20`; bridge requires at least `460` bytes: projection type +420, six projection words +424, six viewport floats +43C; eight konst selector words +1B0. Other cloud/overlay paths also consult shadow state. Live `GXInit` initializes Aurora's SDK state, not the guest MEM1 object. `nflstreet_render_state` is host-zeroed each frame except viewport, separate from guest GX memory. | Original GX initialization `802002C8` and game render initialization must establish the guest words or an explicitly verified equivalent bridge input. Replacing them with zero RAM after `GXInit` is insufficient. Replay generated setup calls and compare SDK argument streams plus guest shadow writes. Aurora device/pipeline state is host-owned. |

**r13 is an address anchor, not one recoverable game-state structure.** Besides
the roots above, fixtures consume fog controls r13-6230/-622C/-6218/-6214/-6210/
-620C/-6208/-562C; default shader/texture state; render scratch r13-6430/-642C/
-6428; HUD option/profile/text roots r13-6334/-6FF0/-6608/-6618/-6610/-6650;
UIS color/mode/cache words around r13-6400..-63A8 and -566C..-5648;
texture-filter bytes r13-6090; resource readiness r13-6538; scene gates including
r13-7258/-7240/-723F/-7280/-71DC; error/refresh hooks; animation callback and
registry globals; scheduler/clock/RNG/DB owners traversed by generated closures.
The full field-to-context assignment is in `N/{scene,venue,hud,light_rig}_fixture.h`.
Every such mutable value needs an initializer or explicit unavailable result;
copying the r13 neighborhood from a snapshot would retain the central dependency.

DOL constants and immutable tables are a separate data source: shader names,
material descriptors, skeleton constants, phase/state/function tables, sine
coefficients and query strings are currently present because MEM1 contains the
loaded executable. Keep them private and read them from the owner's matching
DOL/disc during preparation. `disc_files.c` exposes FST files and TERF entries;
the separate `disc_dol.c` owner now reads pinned boot DOL metadata through nod.
Use that verified bounded route; do not invent
`disc_read("main.dol")` as an existing API or treat NSC1 catalogs as disc files.
BSS, writable data and heap state need reconstructed initialization separately.

### Static startup ordering that the plan must preserve

**Probable static contract** from original bodies, not a fresh run:

```text
field FFFD / 8011F170
  -> 80027270(selected raw venue key), 8003F7F0(two 14-word arrays)
  -> 801EA480 -> 801E577C -> 801E51D4 and downstream session services
  ... dispatch/scheduling handoff still needs tracing ...
80004E4C (game-loop entry)
  -> QKL/resource setup, igdata.dat
  -> 80004830 (scene), 80004728 (field view), UI/graphics helpers
  -> 8002727C (venue/config/environment), 800D9D88 (field), 8015B748 (passes)
  -> 80004870 (state/resources/players/gameplay/crowd initialization)
  -> 801EF034 (lifecycle phase), later DB/actor/ball/UI startup
  -> 8002E45C(initial phase 1 or 10, depending on query result)
```

Do not call `80004870` as a complete match constructor: it assumes owners made
by the first half of `80004E4C` and is followed by material initialization,
actor/ball activation and initial phase transitions. Its order includes
`80103624`, `800BE848`, `80016278/8001638C`, `80041A08`, `80039D30`,
`800F2A94`, `8005D48C`, and environment object creation. Lifecycle callbacks
may allocate later than registration. `801E577C`'s stage-A callback is through
r13-62F8 after `801E3278`; stage-C dispatches r13-62EC. Stage-B `801E0E7C`
and finalizer `801E4B6C` are literal return-zero bodies in this revision.
The callback registration and actual recipient must be traced; the address gap
between session orchestration and game-loop entry is not filled by a guess.

Entry-call closure checklist: the following additional direct calls in
`80004870` have **not** been assigned verified startup ownership by this audit:
`8004DBB8`, `800220DC`, `80007A24`, `800444F0`, `800C7030`, `8007DE88`,
`800CE480`, `8007F748`, `8003D374`, `8002E2C0`, `800B8A6C`, `80035650`,
`80136E28`, `800B77BC`, `800B6550`, `800CE3D0`, `80036634`, `8001E290`,
`8010A880`, `800F1B54`, `80034F04`, `8002FA04`, `8002CF3C`, `8001F6A4`,
`80020388`, `8002AEE4`, `800B00D8`, `8005C518`, `8005C310`, `80142B44`,
`80032050`, `80109B78`, `800F5ED0`, `8001AC60`, `801089C0`. Some may only
register lifecycle callbacks; that is a question for the write audit, not a
reason to omit them. Likewise the post-`801EF034` portion of `80004E4C` includes
`8001C534`, `800226B8`, `80156974`, `800F2AFC`, `8003EDE0`, `800429D0`,
`800CEF6C`, `8002F3BC`, `80104844`, `8004ECE8`, `8000ACE8`, `80004BF0`,
`80039F68`, `800FF73C`, `800FF834`, `80102E9C`, `80102ED4`, `800BF90C`,
`80004D38` and query/phase/UI/resource calls visible in the cached body.
Audit each reached child and its indirect callbacks before declaring the parent
join closed. Kernel coverage inside a generated policy closure does not prove
that these startup entries are exposed, reached or initialized correctly.

## 2. Existing native work to reuse

These are outputs already implemented, with the limits stated in their `.md`
evidence. A startup/preview executable's name alone does not establish a native
match constructor.

| Existing code and evidence | What it produces; what remains external |
| --- | --- |
| `M/quick_game_flow.md`, `controller_{selection,screen,devices}.md`, `team_screen_startup.md`, `team_live_preview.md`, `field_screen_startup.md`, `field_commit.md` | Reconstructed source routing and bounded controller/team/field screen operations; actual native team/field confirmation; retained seven-player choices, two 14-word formations and venue slot/key via `quick_match_selection.c`. Some clock/profile/archive/query responses are supplied. Original commit check stops at `801EA480`, not in gameplay. Controller completion is still supplied in the later team/field host; a complete front-end chain needs joining. Rules are not an implemented live-app editor. |
| `M/match_start.c`, `match_settings.c`, `match_session.c`, `match_pipeline.c`; `fresh_match_start_contract.md`, `match_settings.md`, `match_session.md` | Verified controlled outer caller `801EA480`, mode-zero session `801E577C` and 25-word settings normalization `801E51D4`. Preserves status/continuation/initialization and roster-query ordering. Query/table/resource/downstream operations remain services. Pipeline success means orchestration completed, not actors or a playable match. |
| `N/game_roster.c`, `quick_match_roster.c`, `quick_match_session.c` and matching `.md` | GAME PLAY rows copied/rekeyed from disc STAT/schema, unique new ids, GILP lineup bits, selected and formation mappings. Native session owns a STAT copy through menu teardown. Supports verified ordinary mirrored selections; rejects created-team PYTT 5 and other unsupported cases. Does not serialize the whole original query engine/database manager. Menu host currently validates and exits. |
| `N/player_record.c`, `selected_player_visual.c`, `player_surface.c`, texture/resource modules; `player_record.md`, `selected_player_visual.md`, `slot_textures.md`, `slot_materials.md` | `68`-byte record, face/hair/body resolution and 25 slot textures/45 semantic materials per player. Corrected picker uses STAT membership (`M/stat_picker_rosters.md`). Existing selected-resource callbacks and archives should replace snapshot-derived constructor resources. Matching LCGP/custom-player and unverified variants remain explicit limits. |
| `N/model_view.c`, `model_relocate.c`, `model_chunk_relocate.c`; `model_view.md`, `model_relocate.md` | Raw and relocated body files/views with original dispatcher/chunk checks on nine body assets. Lite/MAnm are unsupported; other handlers have narrower coverage. This is not verified relocation for every environment/UI asset. |
| `N/skeleton_startup.c`, `skeleton_resource.c`, body/face/hair postattachment; `skeleton_startup.md`, `body_postattach.md`, `hair_postattach.md` | Verified `80041108` six SKL1 entries 95,96,97,99,100,101; position/scale/mirror operations and six vector outputs. Needs private DOL-derived 530-byte NSC1 constants catalog. Bound geometry does not initialize natural clip choice, actor descriptor policy or manager queues. |
| `N/actor_factory.c`, `actor_model_init.c`, `player_constructor.c`, `actor_tail_init.c`, `player_startup.c`, `player_bound_startup.c`; corresponding `.md` | Verified controlled factory `801956A4 -> 800E1F7C -> 801639C8 -> 801630B0`; `1A90` actor, transform/channel allocations, tail reset, appearance/model binding and ordered explicit services. Allocation/pools, fresh selected descriptors, cache/resource lookup, scene insertion and some render services remain supplied. Bound-startup comparison covers all RAM outside its declared stack; older isolated gates often compare only owned spans/requests. |
| `N/match_visuals.c`, `match_skeletons.c`, `match_bodies.c`; matching `.md` | **Verified native aggregate composition**, not complete original startup: 14 visual resource owners, 350 slots/630 materials; 84 independently initialized source skeleton resources; 14 private bound bodies. Host namespaces, staging/rollback and retained lifetimes are implemented. Six resources per player is host isolation, not recovered original sharing. Body variant/target choices remain supplied. |
| `N/match_body_palettes.c`, `match_attachment_palettes.c`, `match_model_instances.c`; matching `.md` | Host aggregate matrices and original-layout `F0` instances: retained instance fixture has 84 instances/views/descriptors and 2,724 serialized matrices. Body/attachment fixtures use explicit roots/angles/LOD/scales; material-table tokens in instance test are synthetic and unpopulated. Actual actor slots, group selectors, natural pose roots and frame eligibility remain open. Reuse initializer/kernel evidence, not fixture choices as gameplay defaults. |
| `N/startup.c`, `asset_startup.c`, `model_startup.c`, `field_startup.c`, `scene.c`, `scene_props.c`, `environment.c`; `README.md`, `native_field_render.md` | Owned archive/direct-bank and asset preview work. `build_windows_scene_startup.sh` compiles `model_startup.c` with `NFLSTREET_SCENE_PREVIEW`; there is no separate complete `scene_startup.c` constructor. Six venue meshes and supported placed props use original assets; rotations/lighting are inspection choices. `field_startup.c` uses captured NFR1/NFR2 camera/state profiles. These viewers do not produce original scene objects, collision tree, gameplay or fresh cameras. |
| `N/disc_files.c`, `archive.c`, `lzh1.c`; `disc_files.md` | Host nod I/O and TERF codec 0/5 decoding. Whole archives or virtual entry bytes can feed all existing parsers; keep handle alive through loading and retain borrowed buffers. No original DVD timing, async cache, DOL virtual path or fresh game state. |
| Native runtime/render modules listed in section 1; `actor_palettes.md`, `actor_channels.md`, `player_draw.md`, `scenery_draw.md`, `light_rig.md`, `crowd_draw.md`, `play_selection.md`, `down_cycle.md`, ball/HUD documents | Existing verified or bounded consumers and update math. Keep their APIs/order; construct their inputs. Full new-match startup, natural full UI/replay/audio schedule and arbitrary unsupported gameplay branches do not follow from their saved-state checks. |

Private evidence spot-check: existing reports under
`/mnt/w/Daywater/research-temp/{nflstreet-disc-20261015,nflstreet-play-selection-20261016,nflstreet-down-cycle-20261016,nflstreet-ball-20261015,nflstreet-ballplay-20261015}/verification.json`
say `passed`; their repository reconstruction source fingerprints match this
worktree. Down-cycle live-file fingerprints also match; older disc/play live
integration fingerprints describe earlier wiring. The HUD producer report is
`nflstreet-hudvals-20261015/verification.json`, status `passed_producer_slice`.
These are previously recorded results, not newly executed gates or a claim of
snapshot-free integration. Recheck all transitive inputs/ABI hashes in any new
gate. Some `.md` files predate these owner-run results and still say pending.

## 3. Private starting states and verification contract

### Existing captures found on W:

A recursive filename inventory found these candidate **pre-match savestates** in
`/mnt/w/Daywater/research-temp/nflstreet-moderngekko-win-vulkan-automation/artifacts/`:
`menu.sav`, `quick-game-menu.sav`, `controller-select.sav`,
`after-controller-continue.sav`, `after-team-select.sav`, `after-both-teams.sav`,
`after-auto-pick.sav`, `after-ready.sav`, `after-field-select.sav`,
`after-play-select.sav`, `early-play.sav`. Names suggest useful boundaries;
**their PC, format, revision, phase and resource readiness were not validated**.
They are 19–23 MiB `.sav` files, not 24 MiB raw MEM1. Do not feed them directly
to an original checker or infer that `after-field-select` precedes construction.
Use the compatible original/ModernGekko state loader, or resume privately in the
matching environment, then export MEM1 plus registers (especially PC/r1/r2/r13),
clock, callback bindings, allocator state and required ARAM/resource information.

Reusable menu materializations also exist:

- `nflstreet-team-materialized-fixtures-20261011/{context14,logical-context14,logical-context15,logical-context16}.mem1`;
- `nflstreet-team-dispatcher-probe-20261011-materialized/context14.mem1` and
  `original-run/original-mode6-context14-{pre,post}-event7.mem1`;
- `nflstreet-team-input-20261011/event7-source-dumps/original-context14-{pre,post}-event7.mem1`.

These are **prepared oracle fixtures**, not established natural pre-match
captures. `M/team_screen_startup.md` documents unresolved full startup/query
bindings despite useful bounded checks. Keep supplied patches/queries explicit.
`clean-match-selection-probe/clean_match_first_active.mem1` and later snap/input
images are useful **post-entry references**, not evidence of pre-entry state.
Position-neutral 14/15/16 remain runtime regression contexts.

**New private capture needed if** none of the `.sav` states can reproduce the
entry or if fresh descriptors, asynchronous completion, controller/rule state,
phase entry or another venue cannot be observed there. Capture before and after
`80004E4C`, and targeted child boundaries, not merely another settled in-match
image. At least one second team pairing, a second venue and a nondefault rule
configuration need independent evidence before claiming selection-sensitive
startup. Existing native menu JSON is useful input, not original startup proof.

### Contract for every new reconstruction gate

Use `N/actor_palettes_original_check.c`, `verify_actor_palettes.py` and
`gx_oracle.h` as the pattern. All artifacts stay in a new private W: directory.

1. Pin DOL/module, captures, disc/archive inputs, source/transitive headers,
   generated closure and ABI hashes. Preserve originals and shared fixtures.
2. Prepare identical original/native RAM, allocations, fills, source resources,
   register arguments and deterministic service responses. Original routines
   execute in the untouched DLL. Translate owned native spans into the same
   comparison tokens **before** comparing; log allocations and complete storage.
   All RAM outside the one declared oracle stack must match after each phase;
   also compare return bits and every GX call/pipe write/referenced payload.
3. Saved-as-is plus at least 400 edited cases per applicable context, three
   contexts repeated in two fresh processes. A pre-match capture can be the
   as-is case for startup; 14/15/16 remain controlled and regression contexts,
   not three independent fresh boots. Dirty outputs and untouched allocation
   bytes must exercise branches masked by the settled snapshot.
4. Inject 5–10 real one-line faults per new gate: wrong table index/size, omitted
   publication, wrong side/order, skipped clear, bad relocation, callback order,
   stale destination. Each must cause a RAM/defined-result/GX mismatch. Build
   errors, crashes, timeouts and native rejection do not count as kills.
5. Explicit unresolved service boundaries may return only audited unavailable
   results in both paths; skipped original routines return early and never call
   a native stand-in. Such a gate verifies only the bounded owner, not its
   omitted resource/gameplay work. No broad gameplay or allocation-range mask.
6. `NFLSTREET_GATE_FAST=1`: one context 15, one process, reduced edited cases,
   no mutants, same summary format. Full gate uses all contexts/two processes;
   independent mutant builds run in separate directories in parallel, stable
   result order. No new execution is proposed for this documentation commit.

## 4. Ordered implementation gates

Each numbered row is intended as **1–3 focused days**, including its bounded
oracle. Estimates are planning assumptions. Do not combine neighboring rows
into an unbounded "implement match startup" task. A routine with additional
reached callbacks gets another bounded gate before its parent join can pass.
`Existing` means listed captures suffice for controlled checks; `validate SAV`
means prove the existing pre-match state usable first; `capture if absent`
means a new private capture is a prerequisite if that evidence cannot be recovered.
Every row inherits the full comparison/mutation contract above.

| Gate / effort | Original routines or exact boundary and deliverable | Concrete gate and live connection | Capture need |
| --- | --- | --- | --- |
| 01 / 1 day: entry evidence | Validate/export the listed SAVs and resolve `801E577C` callback registration through `801E232C/801E3524`, game dispatch and `80004E4C`. Record actual PC/arguments and lifecycle/resource order. | Replay one original pre-entry state twice to a child boundary with identical MEM1/trace; retain original entry and first-active references. No native success stub. Produces the input/expected-boundary manifest for subsequent gates. | Validate SAV; capture if absent. |
| 02 / 1–2 days: venue/config scalar producer **(recommended first code task; status 2026-10-16: native producer + all-RAM fast/full parallel-mutant gate implemented and cross-compiled; recorded full pass 2026-10-17: contexts 14/15/16, six fresh processes, 513 cases/process, 8/8 mutants; source/input hashes audited (not a fresh rerun), see venue_setup.md)** | Reconstruct `80026C34` and `80026CCC`: raw venue key to archive base indices, and source-backed weather/overlay configuration writes. Keep `8014B588..8014B708` reads explicit until their FNIG owners are connected. | All-RAM compare eight raw keys and dirty `802A301C..802A308F`/`802A391C..1D`, varied query outputs and source constants. Publish selected env indices/config before resource load. This is bounded and can begin while gate 01 validates entry. | Existing 14/15/16; no new capture for controlled behavior. |
| 03 / 1–2 days: disc/constants owner | Reuse `disc_files.c`; add a proven route to matching DOL tables/initialized sections and zeroed BSS, replace each required private catalog's origin with disc-derived data. | Byte comparison with pinned private DOL/archive exports, reject wrong revision and truncated/overlapping sections; no MEM1 read at runtime. Supplies constants plus fresh backing span before any consumer init. | Existing disc/DOL. **Bounded slice verified 2026-10-17: full Windows gate passed, contexts 14/15/16 twice, 201 cases/process, all 24 MiB equal, 17 invalid inputs/process rejected, 8/8 production mutants caught: pinned nod metadata DOL route (`disc_dol.c/h`), structural section loader and explicit host BSS preparation (`dol_sections.c/h`), independent export/all-RAM gate with production mutants (`verify_disc_constants.py`). Disc-backed skeleton, gear and material catalog adapters now pass their full native composition gates (six processes each, 21/21 production mutants total); see `disc_catalogs.md` for boundaries and reports. Row 03 data ownership is complete for current catalog consumers: `match_disc_inputs.c` joins retained visuals/skeletons with owned disc inputs (six fresh processes, three full aggregates/process, 8/8 production mutants). Final live entry remains in rows 28/29. See `disc_constants.md` and `match_disc_inputs.md`.** |
| 04 / 2–3 days: rule/config adapter | Connect `match_settings`/`match_pipeline` to actual menu/default FNIG, MTTS side contexts and ordinary-mode GAME schemas; implement only the exact queried tags/copy operations needed, including `801E5628/801E5698/801E4D4C` boundaries. | Joined original boundary trace plus all-RAM/owned-table writes for default and one changed rule, errors/cancel/retry. Retained setup owns configuration through menu teardown. Do not invent readable meanings for raw rule tags. | Validate SAV for live FNIG; capture changed rule if unavailable. |
| 05 / 2 days: registered object pool | `80195400`, relevant pool allocator `801812D4`, `801957A4/801958E8`, and scene owner `801954E4` path used by `80004830`; separate registration, allocation and insertion if needed. | Exact registry/pool/node bytes, allocation trace, order/list links, clearing and failure cleanup for a few known types. Native scene tokens replace allocator-outcome literals. Leave uncalled types unavailable. | Existing controlled; validate SAV for natural registry lifecycle. |
| 06 / 1–2 days: matrix storage | `8018908C`, `80189664` and reached per-stack allocation helpers; separately identify palette-pool creation behind `801CA4B0`. | Table/slot allocations, identity/reset, selected stack and dirty popped slots all-RAM equal. Allocate native stacks before view/object constructors; wire existing `matrix_stack.c`. Palette creation with a new closure is a separate 1–2-day follow-up, not an assumed memset. | Existing controlled; capture if pool origin unobservable. |
| 07 / 2 days: view nodes | `80004728 -> 801DF14C/801DF8B4` and reached field/HUD camera type constructors; callback registration `801DF39C`. | Equal node/data/callback bytes and viewport/stack publications; camera view/projection gate from these new nodes. Populate field/HUD handles passed to scene, replacing `hud_cameras[]`. | Validate SAV; capture if fresh camera data missing. |
| 08 / 2–3 days: environment resource | `8010870C -> 80108320/80108278`; bounded full version-105 runtime relocation/tree setup. Reuse `environment.c` parsing. | All relocated pointers, full owned environment and source headers match for EARS and one other venue; spatial lookups `801087B8/8010862C` agree on edited XY. Publish r13-7054/-7058 before ball/AI. | Existing disc + controlled states; capture second venue if needed for natural load. |
| 09 / 2 days: venue field model | `800D9D88/800D9CF0 -> 80165A50`, required `801EC318` resource/view/material construction only. | Fresh type-6 object `B58`, model/shader/texture allocations and field GX stream equal; actual selected venue model name. Publish main-field handle, no `VENUE_MAIN_FIELD` literal. Split unsupported Lite/MAnm handlers into individual 1–2-day gates if reached. | Existing resources; validate SAV for descriptor. |
| 10 / 2 days: five passes | `8015B748 -> 8015B3C0`, reached `8015B388`, `801EC348`, instance setup helpers. | Full `4DC` object, all five model/material/view bindings and pass-order GX stream equal; marks/shadows remain supplied until their gate. Publish field-pass handle. | Existing controlled; validate SAV. |
| 11 / 1–2 days: initial light/sky state | `8002727C` sun/table writes, `8015B928 -> 801ED408`; sky/cloud producer boundaries reached by the entry. | Rig selection/callback installation and initialized records equal; existing `light_rig_update` then matches GX objects for two venue selections. Cloud initialization needing extra callbacks is another 1–2-day child gate. Reuse current draw updates. | Existing controlled; capture if natural sky setup differs. |
| 12 / 1–2 days: field/rule runtime | `80103624`, ordinary-mode config continuation `800FEA5C/8010B60C` as bounded child gates. | Fresh dirty `298` bytes, config outputs, initial down/possession/score and registry writes equal; seed actual rules rather than saved spot/score. Later score-query overwrite in entry remains in the join. Publish r13-7078. | Existing controlled; validate SAV for natural initial config. |
| 13 / 2 days: selected constructor descriptor | Slice `80041378` resource gathering/ordering and descriptor publication, using `80041914`, selected GAME records and six fresh skeleton resources. | Original descriptor bytes/acquisition order equal for selected players; prove mapping GAME DIGP, visual slot, team and resource variants. Supply `nflstreet_player_bound_create` from fresh assets, without inverse reconstruction from saved actor fields. | Validate SAV; capture actual pre-constructor descriptors if absent. |
| 14 / 2–3 days: one complete visual | Join existing factory/player-bound initialization to selected-resource services; implement actual material/shader pointer tables, actor-slot mapping and group selectors from `801639C8/801630B0` and instance helper callers. | Full `1A90` actor plus all allocations/model views/materials equal for one ordinary player; saved and edited LOD/variant/resource cases. Then draw using existing palette/player path with actual material tokens. Additional missing service gets its own child gate. | Existing controlled plus gate 13 evidence. |
| 15 / 1–2 days: selected visual roster | Finish `80041378` list/scene/name insertion and `80041A08` orchestration using 14 owned outputs. | Both nonmirror and mirror lineups: original list/order/header/resource ownership, all-RAM equality at boundary; late failure cleanup. Populate r13-7BBC and u16 counts, no captured actors. | Validate SAV; independent selected-team capture if unavailable. |
| 16 / 2 days: gameplay roster | `80039D30`, `80038B8C`, `8003954C`, handle/index owner `800CC504/800CC644` in separate bounded slices if needed. | Descriptor plus all fourteen `14D4` rows, visual links, stats, backing links/queues and handles match before movement. Publish r13-7C30; initialize live AI/control directly from these newly built rows. | Existing controlled; validate SAV to confirm lifecycle activation. |
| 17 / 2 days: animation registry/cache | Split `80016278 -> 80173B10/80174FD8` into registration and cache allocation gates; bind known event/operand/relocation callbacks. | Full fresh bank/cache/request records, tables, limits and callback words equal, poisoned old records cannot survive. Supplies r13-4F70/-4F6C and stream metadata without `anim_aram_map` deriving it from a capture. | Existing banks; validate SAV for fresh residency/order. |
| 18 / 2–3 days: animation activation | `8001638C`, required `80016428`, bank relocation/request completion (`80175448/80174B20/80175FAC`) and initial set/clip callers, each bounded when more than one owner is reached. | One actor's natural initial manager/set/channels plus first decoded frame equal, then 14 actors; cache/LRU and callbacks included. No mutation may reuse saved clip pointers. Reuse existing clip/channel/pose loop afterward. | Validate SAV; capture if async completion/initial clip unavailable. |
| 19 / 2 days: play/phase/RNG owners | Audit/init the actual `800F2A94`, phase registration and RNG/clock/DB/script owners consumed by play selection/down closure; do not reimplement verified choice/install. | Fresh script features and playbook references, seeded original RNG object/audit values, phase owner and event queues equal. Existing select/install gate passes on this state. Publish roots before live's first play call. Each additional DB/script loader is a separate 1–3-day gate. | Validate SAV; capture absent initial queues/seeds. |
| 20 / 1–2 days: ball pool | `800BE848 -> 800BD410`, index/handle creation, then pool activation lifecycle. | Full `8+count*F4` initialized pool, all handles/association/history/quaternion fields equal, including zero and multiple records. Publish r13-73D0; no snap or motion inferred from pool presence. | Existing controlled; validate SAV for phase order. |
| 21 / 2 days: ball visual/first association | `800BDE90 -> 800D22AC/800D233C/800D1F90`, active-ball publication and actual phase/lineup event assigning carrier. | All ball visuals, draw/shadow objects, spatial/light links and first association equal; existing motion/handoff checks run on fresh result. Replace `HUD_BALL` with produced handle. Initial loose state may reach currently unsupported physics: reconstruct that reached branch in its own 1–3-day gate before advancing. | Validate SAV; capture if first association/branch missing. |
| 22 / 2 days: names/fonts/turbo | `800DB284`, `800DB004/800DB0D4`, font/name producer and `8005D48C` lifecycle initializer as separate bounded owners. | Fourteen `24`-byte tags, strings/font/GX objects and four turbo records equal; existing name/HUD/turbo gates consume fresh storage. Publish count/base instead of `HUD_NAME_TAGS`. | Existing disc/control checks; validate SAV for real strings/controller map. |
| 23 / 2–3 days: in-match UIS | `8012BBD8`, `8012B9DC`, required `8012AA2C/801CFD50`, screen load/class/script bindings at actual first-match screen boundary; reuse menu UI machinery where compatible. | Manager/screen tree/resource callbacks, camera and first messages equal, then UIS GX stream. Down/score publication `801090D8/801091B8` needs a subsequent 1–2-day gate if reached. No blank-success screen substitution. | Validate SAV; capture missing original UI lifecycle/readiness. |
| 24 / 1–2 days: lines/rings/marks | Trace initializers called by `800D9D88`, `800D6F24`, `800DD454` and their registered constructors; verify exact records separately from existing draw routines. | Fresh line/ring/mark lists and their scene nodes equal; controller ring maps to the original assigned actor. Publish arrays/counts for scene and controlled lookup replacement. | Validate SAV; capture if controller assignment unobservable. |
| 25 / 2 days: crowd instance creation | Slice `800D31F8 -> 800D2F60/800D2E94`, definition/model binding `800D3A70`, then `800E899C` separately for its record family. | All created objects/ordered node links, spatial refs, named-lookup skip and full allocations equal for supported record family. Enumerate actual produced crowd count/base; no fixed 45 assumption. Other record families get their own 1–3-day gate. | Existing env/definition/model files; validate SAV. |
| 26 / 2 days: remaining scene effects | `800D0830`, `800CE480` and reached emitter/fan/strip/overlay startup child boundaries; one owner per gate. | Allocated arrays, counts, shader/matrix history and enable-byte writes equal; compare first reached effects GX stream. Do not turn gates off to pass. Feed existing effects/cloud/overlay consumers. | Validate SAV; capture if saved states never exercise creation. |
| 27 / 1–2 days: guest GX defaults | `802002C8` and game render defaults (including `801C769C`); exact bridge-consumed shadow fields. | Equal guest projection/viewport/konst words and emitted setup; first scene draw GX/payload stream agrees with original from fresh state. Keep Aurora `GXInit`; independently initialize MEM1 shadow inputs. | Existing controlled; capture pre-first-draw shadow if unavailable. |
| 28 / 2–3 days: entry join | Reconstruct remaining `80004E4C/80004870` orchestration with proven child owners, exact lifecycle `801EF034`, material/actor/ball activations and phase entry. The unaudited calls listed in section 1's original entry remain a checklist, not assumed no-ops. | Compare all RAM/GX after each completed child stage from pre-match capture. Every reached read maps to disc/DOL immutable data, an initialized allocation/global, or documented host scratch. Reject unexplained dependency; split newly reached owner into another 1–3-day gate. Connect menu commit to native startup owner before first tick. | Valid pre-entry capture required. |
| 29 / 1–2 days: live acceptance | Replace fixture context assembly, RAM-scan control choice and resume-time play selection with fresh constructor outputs/actual controller identity; preserve proven tick/draw order. | No MEM1/NFR/NAP/RAM-derived set catalog runtime input. Owner-disc launch from two team pairs/two venues and changed rules; one first frame plus retained ticks/snap/down cycle, source-pinned original comparisons and live smoke. Failure logs exact unresolved PC/field. Snapshot mode retains existing gate results. | New independent private captures if existing corpus cannot cover choices. |

Rows with multiple bounded slices explicitly require **separate** 1–3-day child
tasks, rather than a promise that the whole subtree fits in the parent estimate.
Read/write auditing may reveal further owners (especially DB/script loaders,
initial ball physics and effects); no finite list is evidence that the fresh
entry is already closed.

## 5. Proposed live seam and completion criteria

**Host architecture recommendation:** retain a fresh big-endian MEM1-compatible
span initially, because generated gameplay closures and the GX bridge read
original global addresses and encode texture pointers as physical MEM1 values.
Populate immutable data from the matching disc/DOL and mutable allocations from
constructors. This avoids rewriting every verified consumer to host structs.
Token-only match asset owners must serialize/map their bounded buffers into
that address space; logical tokens outside MEM1 are not automatically valid
for `gx_aurora.c:host`. Compare against original in controlled allocation
namespaces; do not relocate arbitrary guest pointers with an ad-hoc heuristic.
Original heap fidelity can be narrowed behind explicit allocators, provided all
pointer/publication effects and untouched bytes are accounted for in the gate.

Add a new startup owner (proposed name `native_match_setup`, **not an existing
API**) holding disc/resource buffers, configuration/roster, allocations,
initialization ledger and outputs: draw context, scene/field/HUD view handles,
field/pass/line/ring/ball/name/crowd handles, gameplay descriptor and controlled
actor identity. Lifetime lasts through the live loop; unwind in reverse order.
Use new files; changes to shared modules should be narrowly described and
integrated after their gates, not folded into this planning commit.

The future change in `live_main.c` is precisely at the current file-read through
`scene_fixture` startup block: open `NFL_LIVE_DISC`, run menu/setup/constructors,
obtain initialized `ram` and contexts, then construct bridge and adapters from
those outputs. `nflstreet_scene_link` can connect populated contexts without the
fixture. Open disc before asset constructors; close only after all required
synchronous bytes are owned, or retain it for future proven requests. Feed real
initialized bank metadata into animation rather than scanning saved registry
words. Replace `controlled_actor`/ring-distance scan with verified assignment.
Start existing update/draw calls only after lifecycle and initial phase effects;
do not run the old resume-time play choice twice over already initialized plays.
`CMakeLists.txt`/build copier eventually link new owners; verified consumer
modules need no wholesale rewrite.

Completion requires an executable that starts with menu choices plus the
owner's disc and creates every reached mutable dependency. Changing teams must
change GAME identities, appearance and gameplay rows; changing venue must change
field/pass models, spatial lookup and rig; changing a tested rule must reach
both normalized settings and the runtime reader. First draw, first lineup,
first snap and an actual down transition must consume fresh outputs with no
unexplained original callbacks or hidden captured profiles. Keep byte/oracle
acceptance separate from Aurora pixels and live smoke; neither compilation nor
reading an old `passed` report is new integration evidence.

## 6. Recommended first task and exact gate

After gate 01's capture triage starts, implement **gate 02: venue/config scalar
producer `80026C34 + 80026CCC`**. This is small enough to close independently,
uses the actual menu's raw venue key, and produces concrete state used by both
resource loading and sky/effects. It avoids making the much larger entry
routine the first indivisible task. Confidence for the recommendation is a
planning judgment; the routine contract is **probable** from the cached source.

Proposed new gate name: `N/verify_venue_setup.py` (does not exist yet).
Its acceptance is:

- Native vs untouched original on contexts 14/15/16, two fresh processes each;
  as-is key/config plus at least 400 edited cases/process covering all eight
  keys, each source configuration reader, threshold branches and dirty outputs.
- Source-backed base-index mapping: keys 0..7 yield respectively
  `4D,2C,16,0B,00,21,37,42`; verify against original execution rather than
  copying that table into a test of itself. `80026CCC` writes base, base+1,
  base+2,+3,+4,+9,+A into the original fields; compare every RAM byte outside
  the declared stack, including `802A391C/1D` clears and retained bytes.
- Keep query-reader services named and ordered; either run their original
  backed FNIG implementation or supply identical explicit values and label
  the gate a controlled scalar producer. Do not claim full weather/rules
  ownership from callback capture. Cover DAT_80245820 scaling in instruction
  order and validate actual destination widths in disassembly.
- At least six mutations: wrong venue branch, wrong resource increment,
  swapped config result, omitted state clear, threshold inversion and missing
  copied field; each must fail with a real RAM/return mismatch. Fast mode follows
  section 3. Record private evidence without adding game bytes to Git.

Integration output is the selected venue's **fresh archive-index/config block**,
called after selected key publication (`80027270`) and before environment
acquisition (`8010870C`) in the later venue-startup owner. No shared live file
needs changing to prove this first gate. The next useful join is that output
plus a disc-backed environment resource; a running match still requires the
remaining owners above.
