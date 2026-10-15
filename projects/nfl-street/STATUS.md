# NFL Street project status

**Goal:** A decompiled NFL Street that runs natively on Windows without a
GameCube emulator or game image at runtime. This is a long-term reconstruction
goal, not the state of the current Windows build.

**Current focus: original Quick Game menu-to-match flow, at the user’s request.**

**Latest selected-player connection: owned draw instances and matrix destinations.**
The retained selection now builds84 original-layout model instances, copied
views/palette descriptors and2,724 big-endian matrix records. Initialization
matches the original across32 fill/token cases per context; aggregate ownership,
count validation and rollback pass in two Windows processes. Material tables
remain supplied tokens and group selectors remain pending. This connects the
owned model/matrix layers; it does not yet draw fresh selected actors. See
[selected model instances](reconstruction/native_assets/match_model_instances.md).

**Earlier selected-player connection: owned face and hair matrices.** All 14
selected players now generate palettes from their 70 private bound face/hair
models (442 matrices in the retained selection). Original attachment scaling
blocks match 480 controlled runs and 27,648 complete matrices per process,
across three saved contexts repeated twice. Two native processes confirm
composition, unchanged geometry and late-failure rollback. Roots, scale-slot
assignments and LOD are explicit fixture inputs; fresh actor instances, natural
head roots/pose selection and matrix token publication remain open. See
[owned attachment palettes](reconstruction/native_assets/match_attachment_palettes.md).

**Latest rendering: field overlay, sky node/clouds, live ball and native locomotion selection are connected.**
The scene now runs entry 14 (`800E5CA0 -> 800D1118`) after the players,
while the field camera is still selected, before strips/particles; its
original `r13-71C0` byte gates the draw. Entry 1 runs the sky node and
clouds with their original gates. Projection-vector calls update both the
software executor and Aurora; the overlay's full-matrix projection and
depth observations are also handled. Software checks read their current
EFB depth. Aurora uses real asynchronous Z24 readback, retaining the last
sample per probe (initially hidden); this latency is a host choice.

Each live 60 Hz update selects the controlled player's clips through the
reconstructed locomotion predicates, before all actors' channel passes,
and retains their input/movement writes for the next movement step. After
poses and actor placement, each ball record runs motion, visual pose and
matrix publication with delta 1 game tick. Full ball state/force dispatch,
spin/wind/contact branches and the locomotion event services `80076C48` /
`80076E18` remain unmodeled; reached unsupported branches stop updates.
`NFL_LIVE_HOST_LOCOMOTION=1` explicitly bypasses native selection and uses
the host controller for those unreconstructed event paths or missing
streamed banks. The default requires only `NFL_LIVE_ANIMDIR` for animation.
See [field overlay](reconstruction/native_assets/field_overlay.md),
[ball update](reconstruction/native_assets/ball_update.md),
[locomotion predicates](reconstruction/native_assets/locomotion_predicates.md)
and [live app](native/live/README.md). Integration is compiled here; the
external gate owns render rebaselining, original-module checks and live
build/smoke verification. No new execution or pixel claim is made here.

**Earlier rendering: the frame gains fans, strips and particles, live camera projection and the scaled-actor path.**
Each matches the original module on all RAM (and every GX call and pipe
write where there are any) in all three saved contexts, with planted
faults caught:
- **Effects:** the actor fans `801646CC`, actor strips `801649C4`/`801590F4`
  and particles `80158798`
  ([effects_draw.md](reconstruction/native_assets/effects_draw.md)).
- **Camera slots 1 and 2:** the screen mapping `801DF8C4` and projection
  `801DF938` ([camera_projection.md](reconstruction/native_assets/camera_projection.md)).
- **Scaled actors:** `800DFBA8`/`800DFB34`, feeding the palette step
  ([actor_scale.md](reconstruction/native_assets/actor_scale.md)).
- **Sky node and clouds:** `800E398C` with the cloud draw
  ([cloud_draw.md](reconstruction/native_assets/cloud_draw.md)). In all three
  saved frames the clouds come out effectively invisible in the original
  too, so they are verified but not yet wired into the scene.

- **Animation channels:** the game's own per-update channel step `8003A24C`
  (manager, clip program interpreter, pose list, channel roots) with a
  stream service that serves frames from the bank files
  ([actor_channels.md](reconstruction/native_assets/actor_channels.md)). In
  the live app the other 13 players now animate from their saved channel
  state; clip requests and the group callbacks are not reconstructed yet.

- **Per-player skinning is complete:** the slerped body form `801EAD98`/
  `801CA900` (flag 0x4000) and the head and hand secondary motion
  `8004C268`/`8004BF1C` are native, so `80162760` now runs with no skipped
  routine (only the cache instructions, which touch no RAM, return early in
  its check). The game's own sine, square root and arc cosine (`80216B94`,
  `802165C8`, `80215844`) are reconstructed bit for bit for it
  ([actor_slerp.md](reconstruction/native_assets/gekko_quaternion/actor_slerp.md),
  [actor_dynamics.md](reconstruction/native_assets/actor_dynamics.md)).

- **Clip requests and channel callbacks:** `8017413C`/`80177EF4`/`80178780`
  and the group/event callbacks (r13-0x6718 tables, `8001532C`) are native
  ([clip_requests.md](reconstruction/native_assets/clip_requests.md),
  [channel_callbacks.md](reconstruction/native_assets/channel_callbacks.md)).
  In the live app the other players' programs continue, and the controlled
  player's clips come from the game's own requests where the selector
  inputs are reconstructed (`NFL_LIVE_HOST_LOCOMOTION=1` restores the host
  controller). Written by Codex; checked and integrated through the
  automated write/check loop.

The scene and the live app now run all of these. Two Codex-written modules
(camera projection and scaled actors) were checked here before merging.

**Earlier rendering: the controlled player runs animated, and the frame gains the crowd, live lights and following markers.**
Six pieces landed in parallel, each matching the original module on all
RAM (and every GX call where there are any) in all three saved contexts,
with planted faults caught:
- **Animation:** the pose blend `8015E9C8`, root `8015EC30`, twist `80053404`,
  the streamed-frame decode `8015E938` and the compact skeleton `8015F08C`
  ([actor_animation.md](reconstruction/native_assets/actor_animation.md)).
  The live player decodes `animdata/00111.bin`; which clip plays is still
  the existing host controller's choice.
- **Lights:** the field camera's per-frame light update `801ED3A0`
  ([light_rig.md](reconstruction/native_assets/light_rig.md)), which
  replaced the live app's light stand-in.
- **Crowd and street props:** the scene node's 45 `800D2F20` entries, with
  the spectators' CPU and GPU skinning
  ([crowd_draw.md](reconstruction/native_assets/crowd_draw.md)). It also
  lists the remaining frame gaps: clouds, actor strips, particles.
- **Markers:** the select-ring update `800E4A24`, the camera slots of
  `801DFAB0` and the name-tag projection `800DB0E4`
  ([player_markers.md](reconstruction/native_assets/player_markers.md)).

In the Aurora live app, Thomas now runs with an animated stride, the ring
and his name tag follow him, and the frame runs at 160–210 fps. Two more
pieces are in progress: the head/hand secondary motion (`8004BF1C`/
`8004C268`) and the slerped body form (`801CA900`).

**Earlier rendering: players are skinned natively and one moves under control in the live app.**
Each frame the players' skinning palettes are rebuilt from their poses and
the current camera (`80162760`: body, attachments, head, hands and held
items, through the frame's palette pool, the locked cache and its DMA).
Actors are placed from their gameplay state (`800D4A6C`/`800D4B0C`). Both
match the original in 401 cases per saved context with every RAM byte and
the locked cache compared (16 deliberate faults caught). The venue and
player-select rings now also rebuild their matrices from the camera. In
the Aurora live app ([native/live](native/live/README.md)), the main stick
drives the controlled player through the verified movement runtime, and
the whole scene follows the camera at about 120 fps. Animation is not
connected yet: the player glides in the saved stance. See
[actor palettes and placement](reconstruction/native_assets/actor_palettes.md).

**Earlier rendering: ground marks, ball shadows, the ball and the field camera are native.**
The turf marks (`800D75EC`), the ball and field-object shadows (`801652BC`),
the ball model (`8015CB60`, with its frustum tests and level of detail) and
the field camera's view (`8017D16C` with handler `800C3508`) match the
original. That is 201 frames each per saved context, plus field-camera
views, with every GX call, pipe write and RAM byte compared and 20 more
deliberate faults caught. The matrix library gained `PSMTXConcat` and the
vector helpers. The scene render now uses the native field camera instead
of a view derived from stored matrices. See
[marks, ball and camera](reconstruction/native_assets/ball_and_marks.md).

**Earlier rendering: the in-match HUD is native, so the scene now matches the game's frame.**
The scoreboard, GameBreaker bars, "1ST DOWN", team logos and turbo meters
come from the in-match uis screens (`8012AB5C` → `801D0BC4`). These are now
native: the node walk, transforms, color stack, image quads with the
game's GX texture-object encoders, and text through the native text
renderer. They match the original in 201 screen frames per saved context
(1,202 HUD runs per context, every GX call, pipe write and RAM byte), with
37 deliberate faults caught. The rendered frame now matches the game's
screenshot apart from ground marks, the ball and some effect quads. See
[HUD](reconstruction/native_assets/hud_draw.md).

**Earlier rendering: player name tags, the in-match text renderer and the HUD camera are native.**
The text layout and glyph quads (`80186B1C`), the name tags (`800DADC0`) and
the screen camera view (`8017D16C`, the first piece of the matrix library:
rotations, sine polynomial, translation) match the original: 1,001 runs per
saved context, every GX call, pipe write and RAM byte, with 18 deliberate
faults caught. In the native scene the "Thomas" tag lands on the selected
player's ring, as in the game. The scoreboard and meters (uis screens)
remain. See [HUD text and name tags](reconstruction/native_assets/hud_draw.md).

**Earlier rendering: the native scene matches the game's frame apart from the HUD and small extras.**
The yellow and blue field lines (`800DA074`) and the red player-select ring
(`800E3F7C`) are now native and verified against the original: 1,904 runs per
saved context across all scene routines. The rendered scene has the sky,
field, shadows, lines, ring and players. The remaining differences are the
HUD, ground marks, the ball and some effect quads. See
[scenery and field draw](reconstruction/native_assets/scenery_draw.md).

**Earlier: the native scene adds the sky and player shadows.** The sky dome
(`801655D8`) and actor drop shadows (`801643F4`) are native, including the
vertex data the game writes straight to the GPU pipe. The original matches in
three saved contexts, as saved and in 300 edited cases each. The rendered
scene now has the sky, field, shadows and players. The rings, lines, ground
marks, ball and HUD remain. See
[scenery and field draw](reconstruction/native_assets/scenery_draw.md).

**Earlier: players render standing on the native EARS field.** The scenery
draw (`801EC8E4`) and the two venue loops that draw the field are now native,
down to the GPU calls, sharing the player draw's render engine. 1,201 runs of
the untouched original per saved context match every call, the render state
and all other RAM, and 14 deliberate faults are caught. The software GPU draws
the field and then the 14 captured players into one image with shared depth,
following the original frame order. Sky, decals, shadows, ball and HUD remain.
See [scenery and field draw](reconstruction/native_assets/scenery_draw.md).

**Earlier: selected players have private bodies bound to fresh skeleton targets.**
All 14 body files/views now join the retained selected-player owners. The original
binder matches 12 body type/variant cases across three saved contexts repeated
twice. The 14-player native composition passes stable-link, late rollback and
cleanup checks. Explicit target 0/variant 0 are fixture choices. Actor/model
instances and frame-dependent skinning matrices still need connecting to the
native renderer; fresh menu-selected players are not yet visible. See
[selected body ownership](reconstruction/native_assets/match_bodies.md).

**Earlier: captured players render natively from the reconstructed draw.** A
small software GPU now executes the native player-draw calls: display lists,
skinning matrices, lighting, mipmapped textures, TEV stages, depth and
blending. All 14 players of three saved contexts render in their own uniforms
and poses, matching the game's screenshots up to snapshot timing. The register
states of a real recorded original frame equal what the native calls encode
(both ways), and 337 slot textures decode identically through two independent
decoders. Fresh menu-selected actors and the field/scene remain. See
[native player rendering](reconstruction/native_assets/player_render.md).

**Earlier: selected faces and hair bind to fresh skeleton resources.** All 14
selected visual owners now stage independent initialized skeleton sources and
bind their face/hair models. Two native processes match the existing verified
stages; 210 temporary allocations release, and a late failure preserves every
existing geometry buffer. These are explicit host target/copy choices. Body
skinning, fresh actor/instance state and matrix palettes still need connecting
to the newly reconstructed player draw. See
[selected skeleton binding](reconstruction/native_assets/match_skeletons.md).

**Earlier: the player draw is reconstructed down to the GPU calls.** Native C
now issues exactly what the original sends to the GPU to draw a player
(`80161E10`): lighting, every model instance, each chunk's material
(OnePass/TwoPass shader stages, textures and palettes), vertex formats and
display lists. 854 runs of the untouched original per saved context, the 14
captured players and 840 controlled variants, match every GX call, the render
state and all other RAM, in three contexts repeated twice; 20 deliberate faults
are each caught. Executing these calls in the native renderer, and producing
fresh actors to feed it, remain. See
[player draw submission](reconstruction/native_assets/player_draw.md).

**Earlier: both selected lineups load fresh visual resources together.** A native
owner retains all 14 players from the actual menu selection, with 350 texture
slots and 630 material instances. Namespaces stay separate; late loading
failures discard staged owners before notifications are published. Two fresh
Windows processes match the existing single-player geometry/material owners.
This consumes the emitted menu selection in a loading check; skeletons, actors,
drawing and the running menu's match recipient still need connecting. See
[selected match resources](reconstruction/native_assets/match_visuals.md).

**Earlier: field confirmation creates a retained native roster.** The actual
menu host now builds the roster directly when the field is confirmed and owns
its source database through menu teardown. Both lineups, formation orders and
field choice remain valid after menu buffers are freed. Three navigation flows
and the Win32 self-test pass; back creates no owner. The host currently validates
and exits. Fresh actors, field loading and playable startup remain unfinished.
See [native match ownership](reconstruction/native_assets/quick_match_session.md).

**Earlier: the corrected picker produces both native GAME lineups.** Choosing
one team on both sides now works: the untouched original loader confirms that
copies receive separate player IDs. All43 mirrored teams match byte for byte,
alongside the 86 existing pair checks, in three saved contexts repeated twice.
The bridge passes 387 menu selections and accepts the corrected 14/14 picker
snapshot; the old 12/14 snapshot still rejects. This removes the default choice's
roster restriction. Fresh actor startup and selected-player drawing remain open.
See [menu roster bridge](reconstruction/native_assets/quick_match_roster.md).

**Earlier: the native picker uses the correct STAT roster.** Its new Win32
selection contains 14/14 players belonging to the selected teams, fixing the
old 12/14 mismatch. Team/field navigation, confirmation, back and notification
checks pass; original fallback roster projections still match. Old captures
are preserved. Mirrored-team copying remains unverified and rejects at the
bridge; fresh actors and playable startup remain unfinished. See
[picker roster correction](reconstruction/menus/stat_picker_rosters.md).

**Earlier: menu selections map to native GAME rows.** A new bridge validates
seven players per side, sets the original lineup bits and preserves both
formation orders through GAME id reassignment. It passes 258 selection cases
and 8,192 original scalar setter comparisons per process. The retained real
picker snapshot still has a source mismatch (12/14 ids belong to its STAT teams)
and selects mirrored teams, an unverified route. These inputs reject; the real
menu is not yet connected to fresh actor startup. See
[menu roster bridge](reconstruction/native_assets/quick_match_roster.md).

**Earlier: complete native match records feed selected visuals.** The assembly
now uses the exact STAT identity mapped by each native GAME row and builds its
full player record, including team/color state. All 508 STAT players pass across
three body variants in two fresh Windows processes. The earlier controlled-
color fixture is replaced. Borrowed source provenance rejects the wrong database;
the original GAME-roster gate still passes. Menu wiring, skeleton/actor startup
and selected-player drawing remain unfinished. See
[selected record handoff](reconstruction/native_assets/selected_player_visual.md).

**Earlier: selected geometry and materials share one owner.** Face/body/hair
resources now join the 25 texture slots and 45 material instances. Two fresh
Windows processes check 1,206 assemblies each and reject cross-owner token
collisions before uploads. These use controlled native colors; actual selected-
team record ingestion, skeleton/actor startup and drawing remain open. See
[selected visual evidence](reconstruction/native_assets/selected_player_visual.md).

**Earlier: one owner for player textures and materials.** Fresh player-record and
roster-name inputs now feed an owned surface that joins all 25 texture slots,
recoloring, material binding/settings and the 45 material instances. Across
three saved contexts repeated twice, 28 builds per process match the separately
original-verified stages; 307 failure/reuse checks preserve output and balance
material allocations. Geometry/actor creation and selected-player drawing still
need connecting. See [surface ownership evidence](reconstruction/native_assets/player_surface.md).

**Earlier: textures hooked onto player materials.** The 45 player materials
(eyes, biceps, shoes, …) now take their textures from the bound slots, as in
the original actor startup. Eight arm materials switch to a two-layer shader
when the player has a tattoo. For all 14 captured players, all 630 material
instances equal the live game's: shader, stage count and every stage's
texture and palette. Running the original sequence on fresh material arrays
matches the native result for 819 players, including controlled variants. See
[material hookup evidence](reconstruction/native_assets/slot_materials.md).

**Earlier: material binding.** Each selected player's 25 textures are now
attached to their named materials (HAIR, HEAD, TORSO, … GLASSES). The game's
texture and palette objects are built exactly as the original does
(`800E2BE8`), and the default settings from actor startup (`80164DD8`) are
applied: LOD bias −1.3, and clamped decals. For all 14 captured players the
350 texture sets equal the live objects, apart from fields the game writes
later while drawing. The actual original code matches 20,475 selected sets
per process, along with synthetic sweeps of formats, sizes and settings. See
[material binding evidence](reconstruction/native_assets/slot_materials.md).

**Earlier: the match's player table.** When a match starts, the original game
copies the two selected teams from its STAT database into a GAME database
with new player ids. Native code now builds that GAME player table from disc
data, the two teams and the lineup, and it equals the captured match's table
byte for byte. The original copy, rerun inside the snapshot, matches the
native table for 87 team pairs. The new-id order has a quirk: players
without a `THCD` row are numbered last. All 14 captured player records now
follow from disc data plus the menu's choices. See
[GAME table evidence](reconstruction/native_assets/game_roster.md).

**Earlier: complete player records.** Native code now reconstructs the rest of
the original per-player record writer (`800DE000`): team colors, the 24-byte
color state used for recoloring, and the home/away side byte (`800DE6C8`).
The team data comes from the disc's template database. For all 14 players in
the captured match, the record rebuilt from the match's player row equals the
live record in all 104 bytes. The actual original routine, run on the game's
own live databases, matches 4,318 calls per process across 44 teams, both
sides and a field sweep, in three contexts twice. The match's player rows
(the GAME database built at match setup) still come from the snapshot. See
[player record evidence](reconstruction/native_assets/player_record.md).

**Earlier: original palette recoloring.** Native code now rebuilds the boot
color table (template DB `CPAL`/`CPHR`) and parses all 129 `pladyncl`
recolor documents. It applies the original eight palette operations after
each slot texture is loaded. For all 14 players in the captured match, every
one of the 350 texture slots now equals the live game's memory byte for byte,
70 of them recolored. The actual original recolor routine matches 4,163
compositions per process plus 3,072 operation sweeps, across three contexts
twice. Team color state still comes from captured records. See
[recolor evidence](reconstruction/native_assets/palette_recolor.md).

**Earlier: owned selected slot textures.** Native code now loads each selected
player's 25 gear/body texture slots into owned, relocated destinations,
following the original boot-to-match pipeline (`800E2958` layout,
`800E2AF0` templates, `800E2CAC`/`800E27F8` loader). The actual original
pipeline matches 819 players per process, including 81,081 cache flushes and
4,163 palette-composition requests, across three contexts twice.

The gear plans also equal the live game's request records for all 14 players
in a captured match. The owned textures equal that match's live destination
memory in 281 of 350 slots. The other 69 differ only in palette data, still
awaiting the original recolor step (`800D9C0C`). See
[slot texture evidence](reconstruction/native_assets/slot_textures.md).

**Earlier: selected gear texture plans.** Native code now reconstructs original
`800DE3B8`: from a selected player's appearance record it joins 12 GEAR rows,
builds `PLATEX`/`PLADYNCLUT` keys, fixed-index and default slots and the slot0
fallback, producing all 25 texture slot index/palette pairs plus ordered color
requests. The actual original routine matches 930 complete plans per process:
24,180 slot writes, 26,538 color requests and 11,160 GEAR searches, across
three contexts twice. GEAR records and player-record fields also match the
original builders (564 and 12,462 comparisons), and full sweeps cover two
helpers. Three faults in an earlier uncommitted draft were corrected.

Team-level record bytes still come from captured contexts, and color/request
effects are recorded rather than executed. See
[gear texture evidence and limits](reconstruction/native_assets/gear_texture.md).
Menus and the reference scene work; texture acquisition for these slots,
materials and fresh actor startup remain before a selected match is playable.

**Earlier: original face image/palette application.** Native code now copies
selected face images and palette into an owned template destination. All402
roster selections match the original complete destination and reapplication,
including ordered cache notifications. Per original process:402 initial files,
402 reapplies and2,412 requests; three contexts repeat twice. GPU/cache effects
remain explicit host services, not verified original rendering.

The selected appearance owner retains1,206 applied destinations across three
explicit body variants. Namespace and rejected-upload guards preserve prior
host storage. The earlier face-texture gate still passes. See
[application evidence and limits](reconstruction/native_assets/texture_apply.md).

Menus and the reference scene work. Remaining texture keys, composition/material
binding, pool activation, descriptors and actor creation remain before a fresh
selected match is playable. Protected originals and private inputs are unchanged.

**Earlier: selected face textures and joined appearance ownership.** All402
regular-roster players now load owned raw/processed face MMAP data and an
inspection plane;15 use the original template fallback. Actual original
formatting, HSH1 lookup and complete request-record comparisons pass1,208
cases per process. All388 distinct processed MMAP files and repeat no-ops match
original relocation across three contexts twice.

A reusable owner joins selected geometry and face texture across1,206 explicit
body-variant assemblies, with prepared skeleton binding and namespace rejection.
This supplies the first selected appearance texture; decoded inspection pixels
are not an original GPU/material comparison. See
[face texture evidence and limits](reconstruction/native_assets/face_texture.md).

Menus and the reference scene work. Remaining texture keys, palettes/material
application, resource-pool activation, descriptors and fresh actor creation
still need connecting before a selected match is playable. Protected originals
are unchanged; assets, decoded pixels and diagnostics remain private on W:.

**Earlier: complete fresh skeleton startup.** Native code now owns all six
original startup resources, including position records, scale writes, paired
mirroring and derived vector words. The actual original80041108 caller matches
36 complete initialized files,36 published references and36 vector words across
three contexts twice; only archive acquisition is supplied. Required constants
are exported privately to W: for native loading without a runtime DOL.

Selected geometry now binds prepared face/hair targets across1,206 assemblies
for402 roster players and three explicit body variants. Earlier hair/headwear
gates still pass. See [complete startup evidence](reconstruction/native_assets/skeleton_startup.md).

Menus and the controllable reference scene work. Texture/material ownership,
resource-pool activation, fresh descriptors and actor creation remain before
selected teams can start a fresh playable match. Protected originals are unchanged.

**Earlier: selected hair binding to a fresh skeleton target.** The owned
selected geometry path now binds all three hair models to freshly loaded
animdata entry101. Across402 roster players and three explicit body variants,
3,618 model bindings and repeat no-ops pass per native process. All831 distinct
selected hair files/views match original binding, including2,493 temporary
allocations, their storage and release order per original process. Comparisons
repeat across three contexts twice; earlier headwear/hair gates still pass.
See [binding evidence and limits](reconstruction/native_assets/hair_postattach.md).

This closes another appearance stage. Menus and the controllable reference scene
work; texture/material ownership, remaining skeleton startup edits, fresh
descriptors and actor creation still need connecting before selected teams can
start a fresh playable match. Protected originals are unchanged.

**Earlier: automatic headwear selection and joined geometry ownership.**
Selected PLAY fields now join the original GEAR table to derive headwear state,
then load the selected hair triplet. All 402 regular-roster players succeed.
A reusable owner joins face/body/hair resources across 1,206 explicit body
variant assemblies. Controlled original comparisons match 1,650 extracted
fields, 402 decisions and all 64 mapping inputs per process, repeated across
three contexts twice. GEAR query services are supplied from fresh,
original-extracted records; full database startup is outside this check.
See [automatic selection evidence](reconstruction/native_assets/headwear_state.md).

This removes the manually supplied headwear state from selected geometry
assembly. Menus and the reference scene work; hair/texture/material binding,
fresh descriptors and actor creation still need connecting before selected
teams can start a fresh playable match. Protected originals are unchanged.

**Earlier: owned selected-player hair resources.** Native code now plans and
loads all three hair-model slots from the local and roster archives, preserving
named lookup fallbacks and special-state branches. Across402 regular-roster
players, two explicitly supplied headwear states load2,412 processed models.
Original planning matches1,608 controlled cases per process, and all1,634
unique used model files/views match original relocation and attachment.
Verification repeats across three saved contexts and fresh Windows processes.
See [hair resource evidence and limits](reconstruction/native_assets/hair_resources.md).

This adds remaining geometry ownership alongside fresh face/body resources.
Headwear-state derivation, hair postattachment, texture/material ownership and
fresh actor descriptors still need connecting. Menus and the controllable
reference scene work; fresh selected-match startup remains unfinished.

**Earlier: fresh skeleton loading and connected face binding.** Native code
now loads skeleton resources directly from `animdata.dat` and binds each
selected face pair to that owned target. Six resources match the original
relocation helper across36 whole-file checks and36 repeated no-ops. The
connected original face acquisition path now runs its optional binding stage:
12 acquired files,12 views and12 pool queries match across three contexts,
twice in fresh Windows processes. The relocation-only gate still passes.

All402 regular-roster players pass native assembly and binding for three
explicit body variants:1,206 assemblies and2,412 bound faces per fresh process,
with no snapshot or original module at runtime. This closes the previously
excluded skeleton/cluster binding stage. Protected originals are unchanged.
See [binding evidence and limits](reconstruction/native_assets/model_postattach.md)
and [fresh skeleton loading](reconstruction/native_assets/skeleton_resource.md).

Hair, textures/materials, game-specific skeleton edits, fresh descriptors and
resource-to-actor binding remain unfinished. Menus and the controllable reference
scene work; these resources are not yet substituted into fresh match actors.
The next connection is remaining appearance resources into checked player
creation, followed by playable selected-match startup.

**Earlier: independently relocated base bodies.** All nine decoded base-body
assets now process through owned native directory dispatch and chunk handlers,
then form their complete processed model views.216 whole-file comparisons,
216 repeat no-op calls and216 resulting views, plus54 joined pooled assets,
match actual original relocation and attachment across three contexts and two
fresh Windows processes each. The existing raw-view gate still passes. No
relocation callbacks are stubbed in this check; protected originals are unchanged.

This supplies checked resource namespace references for the base-body catalog.
Face/hair/texture/material acquisition, fresh descriptors and replacing the
reference resource inputs in player creation remain unfinished, as does fresh
match startup. Untested chunk-handler branches remain outside the verified
scope. See [relocation evidence](reconstruction/native_assets/model_relocate.md).

**Earlier: fresh base-body resource views.** Native code now decodes all nine
verified base-body archive entries and owns their complete raw model views.
No saved actor record is copied into this catalog. Across three contexts and
two fresh Windows processes each,432 full attachments and1,728 direct tag
applications match the actual original attachment/lookup bodies, including
missing, empty and duplicate sections and preserved output bytes. Source
archive and protected original hashes are unchanged.

These views retain raw section offsets. Original relocation, face/hair/texture
and material binding, fresh constructor descriptors and connection to the
player-creation path remain unfinished. Current menus and the controllable
reference scene still work; fresh selected-match startup remains open. See
[fresh resource-view evidence](reconstruction/native_assets/model_view.md).

**Earlier: connected factory and model binding.** Native creation now owns the
fresh object's clearing/header and routes model binding into reconstructed
command headers, model copies, index buffers/remapping and shared-model state.
Root independently rebuilt and passed3,072 joined comparisons through the
actual original factory, constructor, appearance and binding bodies. All actor
bytes, allocation storage, registries, source-view mutations and2,172 ordered
requests agree;2,070 requests are name comparisons, not gameplay frames.
Separate factory and binding gates pass576 and96 comparisons, and the earlier
constructor-only gate still passes1,536. Protected originals are unchanged.

Reference resources and inverse descriptors are still supplied, as are cache,
query, texture/node and renderer operations. Fresh selected-resource pools,
descriptor assembly, resource lifetime and playable match startup remain open.
Existing roster resolution and verified base-body archive binding will be
reused. See [joined evidence](reconstruction/native_assets/player_bound_startup.md)
and [remaining loading boundary](reconstruction/native_assets/player_resource_loading.md).

**Connected player initialization — 2026-10-12:** native code now joins the
model/channel initializer, tail reset and appearance state setup. It matches
1,536 original constructor-through-appearance comparisons across three saved
contexts, fresh and prefilled storage, all 64 resource-presence combinations
and supplied binding fixtures. Complete actor bytes, all allocations, both
registries and 89 ordered service requests agree. The separate appearance
gate passes 48 comparisons. Root independently rebuilt and repeated both gates;
protected original files are unchanged.

This constructs owned state in new storage rather than copying a saved actor.
Resource views and descriptor/catalog inputs still come from reference data;
complex model/texture/node binding remains supplied. Selected-player resource
loading, fresh descriptors and factory/session ownership are the next boundary.
A fresh playable match is not yet created. See
[connected player initialization](reconstruction/native_assets/player_startup.md).

**Connected startup and actor foundation — 2026-10-12:** the native caller,
session continuation and25-field configuration owner are now connected.
54 joined original-body comparisons consume the actual Windows menu lineups
and private player records, retaining normalized settings, roster arrays,
status, cancellation and owner state. Separate session and settings gates
pass234 and34 comparisons. Query responses, table/resource and downstream
services remain supplied; a fresh playable match is not created by those gates.

The native base model initializer now owns actual skeleton/channel allocation
requests and source-derived object/channel writes. It matches384 original-body
comparisons across all64 resource-presence combinations, including every byte
of fresh1A90 storage and all allocated blocks. Resources are bounded views
from reference actors in this checker; binding them to fresh selected-player
assets, complete appearance binding, full actor state and field/ball startup
remain unfinished. See [connected pipeline](reconstruction/menus/match_session.md),
[configuration](reconstruction/menus/match_settings.md) and
[base actor initialization](reconstruction/native_assets/actor_model_init.md).

**Earlier caller boundary — 2026-10-12:** native `801EA480` orchestration now
consumes both menu formations, follows original setup/query/error branches
and reaches an explicit session request. Six fresh original-module comparisons
cover102 controlled cases with matching requests, statuses and complete arrays.
One case resolves both lineups from actual packed player records. Resource,
initial/side query and session owners remain supplied; the session recipient
is captured without execution. This does not create a fresh match yet. See
[startup reconstruction and limits](reconstruction/menus/fresh_match_start_contract.md).

**Latest checked milestone — 2026-10-11:** native team completion now loads
and activates field8. Its source artwork, labels, descriptions, timed fades,
eight-slot browsing, confirmation and shutdown run independently. Six fresh
Windows flows retain both seven-player lineups, both fourteen-word formations
and slot1/key1 through the field commit request. Bounded original selected-input
transfer agrees in three saved contexts at the match-start boundary.
Fresh match creation remains unfinished. Controller completion, logical
devices, no-profile predicate, statistic, archives and transition clock/audio
context are supplied. General unlock policy, full original lifecycle agreement
and original GX rendering remain open. See
[joined flow](reconstruction/menus/quick_game_flow.md),
[field runtime](reconstruction/menus/field_screen_startup.md),
[original transfer](reconstruction/menus/field_commit.md) and
[next startup contract](reconstruction/menus/fresh_match_start_contract.md).

The native team menu now receives
the original forward Quick Game load arguments and browses in both directions.
Six fresh Windows runs select teams 1/1,31/31 or3/3, construct both seven-player
lineups and resolve selected PLAY15,PLAY318 orPLAY39. Captures repeat within
each case. A separate untextured athlete inspection pane is connected; shared
model resources can look identical. Ten legacy live-preview cases and the
two-process sequential replay still pass, with protected original hashes
unchanged. The complete original loaded-startup comparison, original athlete
materials/animation, full original field lifecycle and fresh selected-match startup remain
unfinished. See [native team flow](reconstruction/menus/team_live_preview.md).

The complete original team-screen source now constructs and activates in an
independent Windows probe, then passes1,000 passive updates. Fresh original
DOL/template/team data builds both seven-player lineups, formations and pose
requests and emits14 athlete preview requests. Native consistency checks cover
all32 teams, both sides and five original weighting modes. Original curve, gear
contribution, lineup and pose components have bounded original comparisons;
joined weighting is checked with resolved DB getters/allocation supplied.

Active translated source input now reaches the original transition request
with a delay of 500 clock units through supplied logical devices and an owned
transition object/clock.
The source remove/portrait/dirty-confirm sequence also retains seven players
per side after60 further updates. Its state differs from the exit transition.
The device projection, confirmation helper and transition helper have controlled
original comparisons. Requests remain explicit host effects; physical input
discovery and external effect dispatch are not established.

**Original selected-athlete appearance/animation, full original team/field
lifecycle agreement and a fresh selected match remain unfinished.** Passive startup
is not proof of interactive readiness. The earlier uncontrolled saved-context
weighted-row mismatch remains recorded; its numbers are not startup inputs.
See [team startup evidence](reconstruction/menus/team_screen_startup.md) and
[visible roster input](reconstruction/menus/team_visible_input.md). Native
diagnostic captures now show original player labels/rating bars and the
confirmed Ready/Lineups artwork. The later separate geometry pane described
above is an inspection view, not original rendering fidelity.

The original main Quick Game handoff and controller/team/field coordinator are
now reconstructed and joined in an independent Windows routing verifier.
Across three contexts,960 complete main scheduler updates and96 complete setup
source/VM comparisons match the original, including174 ordered screen requests.
Back from field to teams and teams to controllers follows the source scripts.
Team/field completion notifications are still supplied in this older routing
check. The later native team completion join requests field selection;
the recipient and a freshly constructed selected match remain unfinished.
Source loaded-screen ordering now applies in the native routing verification,
preserving animation/callback owners:4,320 complete original comparisons and21
joined source ordering requests pass. Recipient loading remains separate.
The local controller selector now owns original profile cycling, duplicate
resolution, remapping and confirmation requests. Across three contexts,14,403
cycles,45,000 remap/set/read checks,15,000 commits and11,952 actual screen
callbacks match the original. Profiles and logical handles are supplied.
Local source initialization and device refresh now also match across three
contexts: 21,504 refreshes, 10,752 initializations and 43,008 actual screen
callbacks. Device/profile responses are supplied and requests are captured;
this does not establish Windows controller discovery or visible controller
screen activation, physical input routing, team/field selection or selected-
match startup. Team/field completions remain supplied in routing. The automated
verification script builds the strict Windows checks, runs all three contexts
and regressions, and records baseline hashes/private metadata.
Controller source class construction, 12 creation scripts, one selected activation
script and 160 timed updates per context now match the untouched original across
all three contexts. The checks compare the complete 38,682-byte screen, 456 ordered
class callbacks,all scheduled records, owned controller state and 7,803 device
requests. Native startup/timers run independently from local assets with supplied
profiles/devices. Five translated input events and100 following updates per
context now also match the original dispatcher, full screen, selection state,
scheduler and requests. The delayed source completion and linked5 request feed
the native Quick Game coordinator, requesting team7 without a supplied controller
completion. A new independent controller preview renders six source-driven
captures with original artwork/fonts and supplied profile/device state. Its host
rasterizer is diagnostic; interactive desktop testing, main-window integration,
team/field recipients and selected-match startup remain open.
Team selection now has an independent native reader for32 private team records
and a two-sided three-item carousel. Across contexts14/15/16, all1,216 numeric
and96 text fields per context match original field readers;1,170 carousel
comparisons match original8011CE64 and its no-profile eligibility branch.
The native probe traverses both sides forward/backward through all32 records
without the original module. Starting IDs, local ID sorting, and absent profiles
are explicit boundaries. No-profile team startup now matches144 controlled
original cases, including default windows, final cursors, eligibility counts and
ordered resource requests. Image/text construction matches the complete253,987-byte
source in three contexts:3,558 ordered callbacks and1,164 resource acquisitions/
lookups, with every multiline range checked independently. Native probes run
without the original module. Actor previews, creation/activation scripts,
rendering/portraits, confirmation and field8 handoff remain unfinished. Checks
are automated with `verify_team_selection.py` and `verify_team_start.py`;
original baseline hashes are unchanged. The team source callback bridge now
connects startup and carousel to original8011E9D4 framing:144 startup callback
comparisons and1,170 carousel callback comparisons pass in addition to the
previous direct checks. Independent native mixed screen/stack outputs and six
atomic rejection guards pass. Team label/row outputs and actor selection remain
needed before the full source activation sequence can run.
See [team selection](reconstruction/menus/team_selection.md).
See [controller source screen](reconstruction/menus/controller_screen.md).
See [Quick Game flow](reconstruction/menus/quick_game_flow.md),
[screen ordering](reconstruction/menus/screen_order.md),
[controller selection](reconstruction/menus/controller_selection.md) and
[controller initialization/device refresh](reconstruction/menus/controller_devices.md).

### Earlier Options and storage progress
The independent Windows Options window now constructs the original screen from
local UI assets, executes child creation/activation, scrolls through its original
settings, changes values through source scripts, and completes the checked Back
cleanup/return branch. The main inspection opens it on the source Options request;
the host restores the caller inspection when the child window closes.

Across three reference contexts, 60 creation scripts, one selected activation
script, 101 controls and 2,020 subsequent scheduler updates per context match the
untouched original module. That includes all 19 row selections, value changes,
volume-preview requests and timed Back cleanup. 55 native rendered captures pass;
the actual Windows main-to-Options adapter passes too. Native runtime uses local
assets and Windows, without the original DLL, ModernGekko, Dolphin or RAM snapshots.

**Options is not completely finished:** Load/Save, Remove Team, Undo confirmation,
full card dialog/storage integration, original team/career/profile database storage and audio
playback remain open. Native Options settings persistence is now usable: startup
loads a private W: save, F5 saves and F9 reloads with source screen reconstruction.
The explicit native file format stores22 original settings keys/raw values; it is
not an original card save. Failed/corrupt loads retain current settings. Source
default initialization and303 controls/6,060 updates with those defaults match
the original across three contexts. Disk roundtrip/corruption/failed-replacement
checks and fresh-process restart pass; all55 legacy captures are unchanged.
Owned VM suspension/resumption now handles the actual Back caller with query26=false:
three returned raw words and480 timed updates per context match the original.
The recipient lifecycle and initial slot selection also match450 controlled cases
across three contexts. Storage polling also matches192 supplied-response cases;
its recipient UI connection and actual original card/database I/O remain unfinished.
Independent Windows continuation replay produces six captures, and all55 previous
Options captures remain byte-identical. Interactive inspection still uses supplied
query26=true; launchers now use source defaults or their persisted native settings. Rendering is diagnostic. Full original
manager/dispatcher reactivation and fresh profile startup remain separate. Existing
main transitions and field/player/football inspection are preserved. Full match
startup and complete native gameplay remain unfinished.

The original card owner now executes both its immediate-return and nested-prompt
branches. Across three contexts,36 controlled activations,5,760 scheduled updates
and27 supplied raw-result resumptions match the original module. Original word/
character wrapping also matches3,600 comparisons, clearing a dependency of the
visible card prompts. The native original save question now displays Slot A/B,
wrapped normal/warning text, Save/Continue Without Saving/Cancel, original focus
and fades. Its source navigation and return words match120 controlled runs,
1,176 controls and46,080 updates across three contexts. Windows captures exercise
the native prompt. Save returns the slot; Continue returns-1; Cancel returns-2.
The preview does not write a save. Other device-error messages, subsequent owner
polling, routing negative results into Options/native storage and team/career
saves remain unfinished. Details and field evidence
are in [the menu reconstruction notes](reconstruction/menus/README.md).

### Native gameplay frame integration — 2026-10-10

The optional independent Windows ball inspection now uses the original
800C0FE4 frame sequence for attached and detached updates: full position/
orientation history, checked update services and force/angular clearing.
Across three reference contexts,2,112 complete records, visual poses and
retained environment-query inputs match the full original caller. Environment
lookup remains supplied/recorded, and launch/catch/ground policy and full match
startup are unfinished. Details: [ball frame integration](reconstruction/ball/native_ball_frame.md).

### Earlier menu milestones

The loaded-screen manager and linked-screen request now have owned native
implementations. Across three reference contexts,768 direct activation cases
and1,920 joined VM/queue/activation cases match the original manager state and
ordered callback boundaries. These checks supply loaded records and capture
recipient scripts. The native preview still supports Play It/Features/Back;
Options needs its actual linked recipient and activation scripts connected.
This removes a manager-level dependency without claiming full menu startup.

[Original-menu reconstruction](reconstruction/menus/README.md) now executes the
actual title activation bytecode, its original title callbacks, its fade scheduler
and completion helper in native Windows C. All three reference contexts match
four complete source-script activations and 64 fade/completion updates each.
The independent title inspection now shows that fade using the source lettering.
Its static capture remains byte-identical to the previous baseline.

Full menu activation/navigation is **unfinished**. The title's properties9/8
now execute the source background recipient scripts: logo fade, initial player
art selection, archive readiness waiting, fades and recurring portrait changes.
The background root and child activation scripts execute too. Across three
reference contexts, **19,200 complete two-screen updates** match the original,
including delayed readiness and a busy/pending archive case. The native preview
loads all22 private portrait assets and displays source-driven rotations beside
the logo and lettering. A native interactive title/background mode is available;
it has been checked by captures, not a manual keyboard test.
Property6's recipient, full loaded-screen construction and original random-stream
ownership remain open. The checks supply archive status and random draws;
resource I/O is captured. Host rasterization is diagnostic, not a verified GX
rendering replacement.
Main-menu layout and focus scripts now execute with bounded original services.
All five source layout modes match complete original files; per reference context,
18 sequential selections/deselections also match, including their text colors,
button/state updates and 12 help requests. The independent native main preview
shows the four available mode0 choices and source highlights. Help requests remain
captured. Supplied font pointers/static archives make this a controlled stage
comparison; full main activation and original input routing remain open.
Source direction scripts and indexed focus queues now execute too: per context,
40 consecutive numeric events and52 queued targets match, including wraparound
and disabled-row skipping. Native arrows use that event path; original controller
input production is not reconstructed. Help/sound requests are retained rather
than rendered/played. The existing native field/two-player/ball inspection is
preserved; a full independently initialized match is still unfinished.
Play It and Features now activate through original confirm scripts, timed fades,
source layout changes and Back. Three reference contexts match640 complete
updates each with their saved callback inputs, plus640 each on an explicitly
supplied alternate query branch (3,840 total). The native preview uses Enter and
Backspace with its own timer and shows original submenu labels. Its32 transition
captures pass; all53 previous menu captures are unchanged. Options reaches a
separate screen request and remains unfinished, as do deeper choices and match
launch. These remain supplied-stage comparisons, not a fresh native UI boot.
Start routing, all screen-manager services, original composition and the
main-menu-to-match path still need reconstruction. No new controller mapping
or live UI memory field is claimed. Original assets, builds, captures and logs
remain private on W:. Gameplay builds and the airborne milestone below are preserved.

The [native port plan](PORTING_PLAN.md) sets the boundary between Daywater
research, reviewed game code, and the eventual independent Windows runtime.
The Daywater web UI now has a [roadmap overview](WEB_ROADMAP.md), with current
capabilities and completion criteria for the remaining milestones.

## Where things stand

[Released football now moves through a checked native airborne update](reconstruction/ball/native_ball_flight.md).
All 1,536 complete original detached-update records match across three saved
contexts, including supplied forces, field limits, no-spin orientation and plane
sampling. Optional independent Windows airborne inspection advances the real
football after release and retains ownership switching/reassignment. Movement
and paired contact checks pass. Force, marker and no-spin context are host
inputs; ground-plane stopping is a host inspection choice. Original launch,
spin/wind, bounce/catch, natural tackle ownership and fresh match startup remain
unfinished. Existing fixed-release ownership behavior is preserved.

[The visible football now follows checked native ownership](reconstruction/ball/native_ball_ownership.md).
The complete original assignment wrapper matches 576 full records, actor flags,
context effects and model poses across three saved contexts. In the independent
Windows scene, inspection ownership changes attach the ball to either animated
player, release it and reassign it. Movement/set and paired contact checks pass
with six assignments and two releases each. B is a host inspection control;
release freezes the last visible pose. Free flight, automatic snap/pass/catch,
natural tackle ownership and fresh roster/match startup remain unfinished.

[The native scene now renders an animated attached football](reconstruction/ball/native_ball_render.md).
The original GameBall mesh/CMPR texture follows the first reference player's
left wrist through native movement and contact inspection, sharing scene depth.
Repeated original capture resources and 513 native triangles/UVs are checked;
576 original ordinary render bases match exactly. Both Windows movement/contact
runs pass. Disabling the ball preserves all 26 preceding captures, and both
player exports remain unchanged. This is fixed attachment inspection: automatic
ownership, snap/pass/catch, natural tackles and fresh match startup remain open.
A separate private football inspection launcher is available; existing launchers
are unchanged.

[Complete attached-ball record and visual-model updates now match the original](reconstruction/ball/README.md).
576 complete ball records and 576 visual-model poses match exact original
outputs across three saved contexts, including scale, previous-position and
velocity handling. Native contact and direction/set playback exercise these
updates from decoded animation, preserving all prior captures. Existing capture
evidence already identifies GameBall geometry for the rendering step. This is
internal native state; visible fixed attachment is now available as documented above; possession gameplay remains unfinished.
Original root/history scheduling, natural contacts,
snap/pass/catch and fresh match startup remain unfinished.

[Native ball attachment position and orientation now match the original](reconstruction/ball/README.md).
576 complete actor attachment poses and 576 associated-ball position/quaternion
updates match exact float words across three saved contexts. The native viewer
uses its own animated compact hand matrices and now evaluates these rotations
through contact playback and eight-direction movement/set changes. All Windows
checks pass; 26 matching contact/direction captures are unchanged. This supplies
the rotation needed for a football to follow a player's hand. It is currently
internal diagnostics: no visible ball or possession gameplay yet. Original
root/extra-bone scheduling, ball mesh rendering, ownership application,
snap/pass/catch and fresh match startup remain unfinished.

[Compact hand transforms run from native animation poses](reconstruction/ball/README.md).
The 29-bone evaluator matches 2,784 complete original matrices and 192 joined
wrist positions exactly. It replaces captured matrices for native hand-point
calculation; compact bones remain separate from weighted rendering clusters.

[Native ball release now joins owner lookup and trajectory sampling](reconstruction/ball/README.md).
Bounded state callbacks and both release routines match 867 complete original
ball records; 864 owner-lookup joins and 1,728 time/XY samples match across three
saved contexts. This preserves release history and callback ordering instead
of just clearing an owner field. State choices and clock inputs remain supplied;
full hand/model attachment, snap/pass/catch, ball rendering and fresh match startup
are unfinished. The visible Windows viewer is unchanged by this milestone.

[Source-backed contact playback now reaches the visible native scene](reconstruction/native_assets/native_contact_playback.md).
C recalculates selection and both original actor applications from a checked
private context, plays both finite streams and returns to movement inspection.
Two capture comparisons match both complete actors/manager; six original-module
runs match 294 joined updates and 588 full actor payloads. Both Windows contexts
complete twice. Natural geometry/eligibility, contact root travel and original
control handoff remain unfinished; this is not yet an automatic tackle.
Ball ownership/snap/pass/catch and fresh native match startup are authorized
next milestones and remain incomplete. The native ball association lookup now
matches 48 controlled original results; bounded release is now verified as documented above. Actual attachment still
needs the original actor pose/model services and game-global context.


[Two independent native player instances now share the scene](reconstruction/native_assets/native_two_players.md).
WASD/arrows and IJKL drive separate movement/animation state. Both start, turn,
brake and stop in all eight directions; 590 scene comparisons preserve every
color/depth word against a combined render. Existing one-player pixels remain
identical. These instances share the checked reference appearance and captured
movement seed; original roster assembly, contacts and ball gameplay are still
unfinished. The added two-player launcher leaves prior launchers unchanged.


[Supported animation sets now change in the visible native player](reconstruction/native_assets/native_set_playback.md).
The experimental launcher supports V to switch two checked sets while preserving
phase and blending. Three original contexts match 1,440 joined updates, 39 set
installs and 6,120 numeric pose-list entries. Actual Windows runs preserve eight
recorded movement checkpoints and pass all eight starts/releases/stops. Legacy
inspection pixels remain identical. This is an inspection selection; automatic
gameplay eligibility, other players, contacts, ball gameplay and fresh startup
remain unfinished.


[Both animation-set keys now join native source lookup](reconstruction/actor_target_input/variant_second.md).
Three original contexts match 4,632 second-key decisions, 1,980 request checks,
174 ordinary-phase controls and 24 complete two-key curve payloads. Ordinary
request/phase/additional/index facts are calculated natively. The special
6510C predicate and broader target/search paths remain supplied or unfinished;
the later supported catalog integration above applies checked inspection
requests to the Windows player. Automatic gameplay choices remain unfinished.


[The first animation-set key now resolves natively](reconstruction/actor_target_input/variant_selection.md).
Three source contexts match 9,234 complete key/flag decisions and 144 joined
curve payloads. Required game-service facts are explicit inputs; the second
key and those facts still need native producers. This is a checked selection
component, not automatic set switching in the viewer. The current one-player
Windows build and launcher retain the preceding verified behavior.


[Original directional animation now joins native playback](reconstruction/native_assets/native_directional_playback.md).
The native controller uses the original distinct heading thresholds for clip
selection and blending. Three contexts match 960 boundary cases and 1,440
joined updates, including 5,988 original frame-weight entries. The Windows
experimental path passes movement and stop checks. These are controlled source
stages; full gameplay callback eligibility and changing variants remain open.


[Original movement-curve choice now joins native playback](reconstruction/native_assets/native_curve_choice.md).
Three contexts match 6,318 controlled decision cases and 1,440 joined updates,
including 24 source-identity swaps and 6,570 exact original pose-list entries.
The experimental Windows viewer consumes carried movement inputs and passes
the recorded/eight-direction checks. Upstream variant keys remain fixed;
controlled curve swaps are not evidence for complete original gameplay.


[Eight-direction native movement now works](reconstruction/native_assets/native_directions.md),
including backward/diagonal inputs, starts from rest and release-to-brake stops.
Three original stage comparisons match 192 new complete movement checkpoints;
all earlier 20 moving and 192 braking checkpoints still pass. Windows key-handler
checks exercise eight starts/releases/stops, and inspection pixels stay identical.
[Original timing now drives both chosen idle/run cycles](reconstruction/native_assets/native_idle_stream.md),
including the accepted idle flag callback. Original clip selection, full callback
context routing and transition blending remain unfinished.

[Original locomotion metadata now resolves natively](reconstruction/actor_target_input/locomotion_selection.md):
1,320 row choices, three full curve payloads and 243 speed-boundary index
comparisons match original code. The probable source bank's 4,641 pose frames
also match the original decoder. An additional 972 controlled calls match
original paired-channel weights, smoothing and playback-rate changes, including
entire curve/channel state. [Source-driven paired playback now renders in an
optional native viewer mode](reconstruction/native_assets/native_paired_playback.md):
1,440 joined original-stage updates match, and a real Windows run renders
paired poses through all eight movement directions. [Original phase transfer
now replaces immediate pair restarts](reconstruction/native_assets/native_phase_rebind.md),
with 804 full original-caller stream comparisons including cycle boundaries.
[Bounded outgoing-channel fades now render natively](reconstruction/native_assets/native_retiring_fade.md):
1,440 joined updates and 6,768 original frame-list entries match. Windows movement
checks pass with four carried channels and completed retirement. Upstream
variants remain fixed, and full callback/transition choices remain unfinished. The existing movement
launcher retains its baseline; `W:\Daywater\Start-NFLStreet-PairedPlayer.cmd`
opens the new path.

[Native ball observation and plane sampling](reconstruction/ball/README.md)
now match original accessors and 36 original time/XY samples. This provides a
bounded numerical ball subsystem, not launch/catch/bounce rules or a rendered
ball. Existing paired-actor and contact reconstruction still need connection to
the visible runtime. A captured movement seed is still required; fresh native
initialization and a complete standalone football match remain unfinished.
The launcher is `W:\Daywater\Start-NFLStreet-MovementPlayer.cmd`.

The [live native player viewer](reconstruction/native_assets/native_live_player.md)
now puts one controllable animated player on the independently rendered field
and sky. Windows evaluates and renders poses each update, with cached assets
and field depth. WASD/arrows move and turn; R resets. Two actual Windows input
runs pass, and five checkpoints repeat exactly. The private launcher is
`W:\Daywater\Start-NFLStreet-LivePlayer.cmd`. Speed, bounds and clip choices are
inspection behavior; original locomotion, ball behavior and match startup
still need to be connected. This is not yet a playable football match.

An [independent original-camera field renderer](reconstruction/native_assets/native_field_render.md)
now renders the EARS ground and buildings in Windows C from original assets,
vertex colors and measured material/depth settings. It processes 3,029 checked
field draws and 87 selected texture planes. Two fresh capture profiles and
their native images match exactly. The field remains a static rendering slice. The newer native sky/player
viewer adds live actor poses; effects, live camera updates and a playable match
remain open.

The native scene viewer now assembles all eight venues from six environment
layers each, and places 143 supported objects from 586 original instance
records. Independent Windows checks match the environment geometry exactly
and placed positions within 0.000004 units. The private launcher is
`W:\Daywater\Start-NFLStreet-ScenePreview.cmd`. This is an inspection preview:
47 former matrix omissions now render in stored poses. One fountain record
has no original definition; the original module explicitly skips failed
definition lookups. Several other scene subsystems and
animated actors remain absent, and original render state is unresolved.
The image is visibly sparse; complete original scene assembly is not finished.
See the [scene evidence and limitations](reconstruction/native_assets/README.md).

Two fresh saved-match research runs now produce identical GPU captures,
including 10,638 draw states and ordered memory payloads. Exact position arrays
identify EARS and all five field passes, with original blend/depth modes
documented. Native rendering still uses inspection settings; the nearby
screenshot is not an exact-frame pixel comparison. See the
[rendering reference](reconstruction/native_assets/original_render_capture.md).

| Area | Current evidence | Status |
|---|---|---|
| Daywater research platform | Reads verified score and selected-player position fields; traces their live writers in Dolphin | Useful for discovering and checking game behavior |
| Windows gameplay experiment | Loads the user's game data and a saved match through ModernGekko/Dolphin with a statically recompiled DOL; a controller input path works, slowly | Playable prototype, still emulator dependent |
| Reconstructed game code | A Windows C score path reproduces failed and successful PAT events. A guarded native actor step takes normalized cardinal input vectors and reproduces complete actor-input and movement blocks across four direction sequences, an eight-update run, and four partial-strength sequences. | Verified native slices, far from a whole game |
| Independent native game | Independent loaders, field/sky rendering and a live animated player with host keyboard movement work; original gameplay integration, complete match loop and audio remain absent | Interactive native inspection works; full game not yet running |

The recent paired-single work proves that we can replace selected generated
PowerPC translations with checked C behavior. Those routines are small and
do not by themselves make the game independent of ModernGekko. They should
be treated as a validation method, not as a measure of percent complete.
The [score-event replay](native/score_event_replay.md) is more directly tied
to game behavior: it takes an actual captured event through a native C
score path without Dolphin at replay time. It still uses test adapters and
does not run the match or render anything.

The [movement boundary](reconstruction/kinematics_boundary.md) maps the
position/velocity update at `0x80036B28`. Its
[native replay](reconstruction/kinematics/README.md) matches all 21 words from eleven live
calls. The [cardinal input replay](reconstruction/input_translation/README.md)
matches all 48 actor-input bytes for controlled directions and partial
strengths. It is a bounded replay; game-service calls and update scheduling
are still missing.
The [command-to-movement replay](reconstruction/command_to_movement/README.md)
matches the complete command and movement output blocks for eight live calls.
It calculates the observed bounded turn-heading branch with the original
saved-state turn limit. The newer
[actor-update replay](reconstruction/command_to_movement/actor_update.md)
also calculates acceleration and matches preparation, command, and movement
blocks in three fresh live calls. That earlier replay starts from
game-populated actor input fields; the newer stateful bridge below translates
its accepted vectors and steps repeatedly.
The [consecutive update replay](reconstruction/command_to_movement/update_sequence.md)
now runs three native updates in succession and matches two independent live
sequences. It carries the full movement block forward and applies the one
observed intercall acceleration reset. The
[calculated setup branch](reconstruction/command_to_movement/reset_acceleration.md)
now derives that reset from actor input strength and rating. It matched
separate right and forward runs, including a three-update right run. The
loop also accepts a distinct game-populated actor input block on each update.
It matched fresh live three-update right-to-forward and forward-to-right
transitions, a right-to-left transition, and a two-switch right-forward-right
sequence in Windows. A guarded mode now takes an R/F/L stick path and
computes the four changing heading words, but starts from a saved actor
block. It does not yet implement the full controller translation or schedule
the rest of the match.
The guarded movement path now has a stateful native C interface with one
step per call, separate from the replay CLI. It preserves its prior state
when a guarded step rejects an input. The existing live trace checks still
pass on Windows after the extraction.
It now accepts normalized cardinal input vectors as well. The Windows bridge
matched every byte of the actor input block and every word of the movement
block across four live three-update sequences. The bridge still uses a
saved-match starting state and a measured strength rule; the complete
controller and game scheduling paths remain open.
A separate eight-update live run now matches every actor-input byte and all
21 movement words on every call. It exposed and resolved one floating-point
rounding difference using the original DOL's fused length instruction.
Four fresh partial-strength sequences also match on Windows, including
full-to-half, half-to-full, and a turn with simultaneous strength change.
They required the measured strength cap and original movement speed-cap
branch. A [neutral-input capture attempt](reconstruction/command_to_movement/neutral_capture_limit.md)
found that Thomas's actor is cleared before the next comparable update in
this saved state. Neutral behavior remains unverified.
An [eight-update scheduler trace](reconstruction/command_to_movement/update_schedule.md)
now establishes the input, reset, preparation, and movement call order on
successive dumped frames for this actor and state. Every captured reset and
native actor output matched the live run. A general game scheduler remains
open.
Two fresh [multi-actor order traces](reconstruction/command_to_movement/actor_update_order.md)
show the same nine actor pointers reaching movement preparation in the same
order on two complete dumped frames. Other actors' movement branches have
not yet been checked against the native runtime.
Later [actor-order captures](reconstruction/command_to_movement/actor_order_dynamic.md)
show that the slots reaching preparation change during the match. A native
scheduler needs to reproduce actor eligibility as well as call order.
The [second actor replay](reconstruction/command_to_movement/second_actor_replay.md)
matches Stewart at observed roster slot 7 through preparation, command,
reset, and movement. Two fresh right/forward runs each matched three
consecutive Windows native updates. This actor's input blocks were captured
from the game. A newer [target-position input reconstruction](reconstruction/actor_target_input/README.md)
now computes Stewart's later input blocks from captured target positions.
The native Windows loop matched three full updates under right input and
three under forward input. The game logic selecting those targets remains
open; the native controller-to-input path remains checked only for Thomas's
observed branch.
Two later [selector handoff captures](reconstruction/actor_target_input/README.md#upstream-target-handoff)
now show the original target-output function feeding Stewart's input function
without a pointer or byte change. In both tested calls, it received Thomas
at roster slot 8 and returned his position with Y 2.0 lower. This narrows
the target-source boundary, but does not establish how Thomas was selected
or how other selector branches behave.
The [paired replay](reconstruction/command_to_movement/paired_actor_replay.md)
then advances Stewart and Thomas together for three consecutive frames in
each of two input runs, matching both actors' movement outputs. It forms
Stewart's later targets from Thomas's prior native position. A guarded
[native C target composer](reconstruction/actor_target_input/README.md#bounded-native-vector-composition)
now matches all six original selector outputs in those runs. The Windows
actor loop uses that C function for Stewart's later targets; both paired
replays still match every captured actor and movement output block.
An [integrated Windows pair loop](reconstruction/command_to_movement/paired_actor_replay.md)
now advances both actors and composes the targets inside one executable.
It matches both three-frame reference runs from initial saved-match states.
Six fresh [choice-call handoffs](reconstruction/actor_target_input/README.md#other-actor-pointer-source)
now show the upstream function returning the same Thomas pointer later
passed into Stewart's target calculation. The hash-checked DOL maps that
function through a record lookup. A private original MEM1 snapshot and
six live calls now verify the record handle `0x00010101` resolves to
roster slot 8. The bounded native pair loop checks this handle and still
matches both three-frame runs. The rule changing the manager's record
index across match states remains open.
Two more [live internal traces](reconstruction/actor_target_input/README.md#live-record-and-lookup-arguments)
now stop at the record and handle lookup boundaries themselves. Across
three right and three forward calls, they confirm the selected record,
handle argument, and returned Thomas pointer at each step. The index and
handle stayed fixed in this saved match; the circumstances of their writes
remain unknown.
The [selection-index write boundary](reconstruction/actor_target_input/README.md#selection-index-write-boundary)
identifies a direct DOL store and two callers. Write watchpoints observed
no index change in separate 25-second right and forward runs from the
saved match. A state transition that executes the store is still needed
to establish when the game switches selected records.
Eight hash-checked [first-down RAM snapshots](reconstruction/actor_target_input/README.md#first-down-record-handle-transitions)
show a different transition: the record index remains 0 while its handle
changes from empty to values resolving to Urlacher, Stewart, then Thomas.
The native C decoder matches each roster entry's name bytes. The live
writer and whether target-choice calls use each intermediate handle are
not yet established.
Two fresh [first-down choice calls](reconstruction/actor_target_input/README.md#first-down-live-choice-calls)
now verify that Stewart's target path reads the final Thomas handle after
the snap under both right and forward input. The earlier transient handles
in the RAM series were not seen at this call boundary.
The [snapshot-start Windows replay](reconstruction/command_to_movement/snapshot_start.md)
now loads a private RAM image captured at Stewart's preparation boundary,
derives both actors and the selected handle, and matches a fresh
three-frame right-input run. This removes trace-supplied initial actor
blocks for that fixed-state continuation; native boot and full-match
services remain open.

## Direction

Fresh [record-write and actor-handoff captures](reconstruction/actor_target_input/record_handoff.md)
now verify the copy call causing the empty-to-Urlacher-to-Stewart-to-Thomas
first-down changes. The two later changes follow a record lookup for the
current actor and an incoming actor argument. A bounded Windows C replay
finds the record and carries both changes forward, matching two fresh
right/forward traces. It accepts captured events; the full assignment
wrapper and event generation remain unimplemented.

A newer [descriptor-to-event Windows replay](reconstruction/actor_target_input/descriptor_event.md)
now constructs the event and derives its incoming actor from captured
actor descriptors and roster headers. It matches six full event blocks,
flags/input-mode outputs, and four later handoffs across two fresh runs.
The executable no longer receives the expected incoming actor at handoff.
It still uses captured descriptors, the initial Urlacher record, and the
observed ordering of build/handoff actions.

The [readiness-signal and native polling milestone](reconstruction/actor_target_input/event_signals.md)
now identifies command callbacks setting the event advance/ready flag bits.
Windows C matches ten command effects and eight stage-three gate decisions
across two fresh runs: four waits and four handoffs. It also reproduces a
conditional old-actor flag cleanup discovered during those comparisons.
The poll decides whether to hand off from its inputs; command emission,
earlier event stages and the rest of the actor transfer remain unimplemented.

The [native command-timing milestone](reconstruction/actor_target_input/command_timing.md)
now removes captured emission times for the measured readiness branch.
Windows C matches 30 complete timing updates across two fresh right/forward
runs, including both readiness commands, 26 command-free updates, and six
consecutive carried updates per run. It advances from supplied stream state,
program bytes and elapsed time. The two captures cover the first later
handoff; stream initialization/replacement, other callbacks and full event
processing still come from the reference.

The [stream setup and early-event slice](reconstruction/actor_target_input/stream_setup_early_events.md)
now reconstructs table lookup, full interpreter initialization, and the
observed early-stage decisions and steering in Windows C. Two fresh runs
match six complete setups/lookups, 12 early event updates and two pairs of
animation requests, each yielding the next stream key. Ten contact-service
updates are explicitly unsupported and not counted as matches.
That earlier event slice still rejects the contact service branch. The new
[contact selection and completion component](reconstruction/actor_target_input/contact_selection.md)
constructs parameters and eligibility, scans all supplied candidate assets,
chooses the paired animation, advances RNG state, and reproduces the owned
event-completion fields. Three fresh runs match 15 selections (12 failures,
three successes), both paired requests and three stage-three transitions.
The selected asset and expected requests are no longer supplied to this
component. Used alone, that selector/completion verifier reports animation
changes separately. The newer paired application below reconstructs those
changes for the tested contact; a combined match loop remains open.

The [animation component](reconstruction/actor_target_input/animation_channels.md)
now predicts channel/manager allocations and reproduces clip initialization,
manager preparation and blend updates in Windows C. Two fresh runs pass 26
complete block comparisons. Callback metadata, transition matching and source
bank resolution were outside that component. The newer
[callback and transition reconstruction](reconstruction/actor_target_input/callback_metadata_transitions.md)
now joins links, requests, allocation, metadata, transitions and completion
in a single native paired contact application. Two fresh runs match both
complete actor objects and the selection manager with zero byte differences.
The source tables remain providers, and the observed transition branch has
no eligible row. Other callback/transition families remain unsupported.

The [asset-loading and spawning extension](reconstruction/actor_target_input/asset_loading_transition_spawning.md)
now loads the tested bank metadata/programs from original extracted disc bytes,
derives their source tables natively, and starts two different transition clips.
Two fresh controlled original-game experiments match six full actor checkpoints
each and both clip durations in Windows C. Natural eligibility of those rows,
natural match asset selection, frame decoding/rendering and native match
startup remain open.

The [native asset bootstrap](reconstruction/native_assets/README.md) now reads
TERF directories and reconstructs LZH1 compression from the original DOL.
The independent Windows unpacker processes all 8,688 entries in 47 DAT
archives, including 5,903 compressed entries with exact-size/checksum checks.
All plain entries and 39 compressed entries also match an independent Python
comparison. Native C decodes the first texture plane in 2,672 MMAP assets and
draws the original loading image in a real Windows GDI window. Startup owns
a catalog of 14 animation banks and its registry without captured RAM.
The separate independent Windows model viewer now draws original static
meshes with embedded OnePass textures, including the Bronx first field pass
(561 triangles and all 12 material textures). The full model scan accepts
125 static models and explicitly rejects 4,045 unsupported model entries.
Fourteen native mesh dumps, including three field meshes, match an independent
raw-file reader exactly across 10,582 triangles, positions, UVs and material
selectors. A real Windows field paint passes. This remains an inspection
renderer with its own camera/lighting, not playable match startup. Skinned
actors, animation frames, scene assembly, original asset-selection rules and
fresh match state remain to be reconstructed. Original loader/model/MMAP
semantics are probable; no new live Dolphin integration is claimed.

The next substantial step is joining the
complete bounded contact update with stream setup, timing,
event processing and movement in one native segment. Wider descriptor
production, changing actor eligibility, native startup and full-match
services remain open.
Continue using Daywater to verify game-specific facts while keeping runtime
code outside generic Daywater.

Keep ISO files, extracted assets, savestates, generated whole-DOL code,
Windows builds, and logs on private W: storage. Record each claim's
confidence and test procedure. Update this page when a milestone changes
what the native program can do or materially changes the route to the goal.
