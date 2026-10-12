# Native asset loading and Windows bootstrap

This is an independent C asset pipeline for GNNE69 revision 0. It does not
link Dolphin, ModernGekko, or generated PowerPC code. Locally extracted DAT
files are its inputs. Daywater remains the generic research workbench.

**This starts an asset preview, not a playable native match.** It loads its
own animation-bank catalog, unpacks original textures, and displays one
through Win32/GDI. It does not inherit emulator RAM or a captured registry.
The separate model preview draws static 3D meshes and their embedded textures.
The scene preview combines the six environment layers for all eight venues
and places supported objects from their original instance records. Complete
scene rendering, animated actors and match initialization are still missing.

## Results on 2026-10-08

| Check | Result | Interpretation |
| --- | --- | --- |
| Original DAT extraction | 47 archives, 903,809,920 bytes | FST from the hash-checked ISO, private W: storage |
| Windows archive unpacking | 8,688 entries, 952,653,276 decoded bytes; zero failures | All directory entries in the extracted DAT corpus |
| Codec coverage | 2,785 plain; 5,903 LZH1 | Every compressed output passed exact-size and embedded Adler checksum checks |
| Independent reference checks | All 2,785 plain entries; 39 compressed entries | Separate Python decoder uses counts/tables read from the original DOL for loaddata/plamodel |
| Native texture decode | 2,672 first texture planes, 51,669,568 pixels; zero failures | Includes CI4, CI8, IA8, RGB5A3, RGBA8 and CMPR assets; no live pixel comparison |
| Owned animation startup | 14 direct banks, 6,909,568 arena bytes | Discovered at archive entry boundaries; own registry, no RAM-header search |
| Animation duration scan | 2,497 accepted; 162 unsupported | Existing bounded program scanner; unsupported programs remain visible |
| Windows graphics | One successful GDI paint, zero failures | Actual Windows window draws the original 640×448 loading screen and closes automatically in render-check mode |
| Static model coverage | 125 accepted models, 48,866 triangles; 4,045 unsupported TMdl entries | Full decoded corpus scan; unsupported matrix/skinned layouts are rejected |
| Independent geometry comparison | 14 models, 10,582 triangles, exact serialized vertex/UV/material matches | Separate raw-file Python reader versus native Windows output, including three field meshes |
| Textured field preview | Bronx first field pass: 16 draws, 561 triangles, 12/12 material texture names resolved | Real Windows GDI paint succeeds; this is one mesh, not a complete assembled scene |
| Environment assembly | Eight venues, 48 layers, 60,066 triangles | Exact independent combined geometry; original render state remains unresolved |
| Original object placement | 586 instance records: 143 rendered, 442 nonvisual, one absent definition | Includes stored cluster poses for 47 formerly omitted instances; independent placement difference at most 0.000003815 units |

The preview's common-bank clip 75 duration is 52. Common-bank clip 84 is
24.9375; that is a different bank/clip identity from the prior transition
experiment whose duration was 124. Clip numbers alone are not asset identity.

## Evidence and confidence

Game: NFL Street, GameCube, GNNE69 revision 0.
ISO SHA1: `813c4511ba3135059a13de424555647e8252254b`.
Original DOL SHA1: `46e3de854998a0f66c9be8c72a1164d22cd29eb4`.
These checks are **offline** and native Windows checks. No new live Dolphin
decoder, texture, or match-startup integration is claimed.

### TERF directory — probable original semantics

Original `0x801A5A0C` reads the TERF header and walks length-prefixed chunks;
`0x801A3E44` reads DIR1 rows and optional COMP decoded sizes. DIR1 rows start
at chunk+8. Offsets are relative to the DATA **chunk header**, including its
eight bytes, rather than file start or DATA payload. Entry count is the
big-endian u16 at file+14. DIR1 and COMP rows have eight bytes each. COMP is
codec ID then decoded size. Unknown chunks can be traversed but hashes/name
lookup is implemented separately for the supported HSH1 algorithm. Duplicate required chunks and out-of-range
entries fail before output is published.

The entire corpus traverses successfully and all plain entries compare
byte-for-byte with the corresponding original DATA ranges. Directory behavior
has not been compared by calling the live original loader.

### Codec 5 / LZH1 — probable original semantics

Original initializer `0x802398A8` registers codec 5 at descriptor
`0x8029B5B4`, whose identifier is `LZH1`. Decoder setup `0x8023CCA4` selects
`0x8023E75C`. That routine reads one control bit, then two code-length tables,
using `0x8023F448` and `0x8023F4FC`; `0x8023E838` decodes literals, end markers,
lengths and overlapping backward copies. `0x8023EE2C` supplies MSB-first bits.

Each block begins with control bit zero followed by 285 literal/length code
lengths and 30 distance code lengths, four bits per length. Canonical codes
are assigned in symbol order. Literal 256 ends a block. Symbols 257–284
select the original length table; the final length is **227**, not standard
DEFLATE's 258. Control bit one ends the stream and carries a 32-bit checksum.
Original `0x8023FA30` implements the Adler accumulators modulo 65521.

The original tables are at `0x8026A1E0` (length extra bits), `0x8026A1FC`
(length bases), `0x8026A238` (distance extra bits), and `0x8026A256` (distance
bases). Counts are read at `0x8026A1D0` and `0x8026A1DC` by the independent
verifier. These are **DOL evidence locations**, not verified game-state fields.

The C decoder was reconstructed from these instructions. An EA open source
[Huffman decoder](https://github.com/electronicarts/CnC_Generals_Zero_Hour/blob/main/GeneralsMD/Code/Libraries/Source/Compression/EAC/huffdecode.cpp)
was investigated but was not the codec and was not incorporated.

All 5,903 compressed entries pass their exact decoded sizes and embedded
checksums. The separate Python reference reproduces all bytes of 10 compressed
loading screens and 29 player-model entries. This establishes strong offline
evidence; fresh live-original decoder comparison remains required for a
`verified` claim about full original-loader behavior.

### MMAP layout and texture encoding — probable/candidate

Version-two MMAP header offsets +24/+28 identify texture/palette descriptors. In the
first texture descriptor: width/height at +0/+2, GX format at +4, byte length
at +8, pixel offset at +12. Palette descriptor: format at +2, length at +4,
pixel offset at +8. Multi-image assets and alternate palettes exist.

Layout confidence is **probable**: 2,672 independent assets decode within
bounds and the loading image is visually recognizable. Original reader
`0x801C5610` confirms version-two image descriptors of 16 bytes and palette
descriptors of 12 bytes. Original helpers `0x801C588C`, `0x801C589C` and
`0x801C58C0` index named records, fixed-width names and image descriptors.
Live pixel comparison is still needed. Formats present in
the checked first planes are 3, 5, 6, 8, 9, 14. Other supported GX formats
(0, 1, 2, 4, 10) remain **candidate** for this game corpus.

GX tile, palette, channel expansion and CMPR interpolation semantics were
checked against the existing local Dolphin source files
`TextureDecoder_Common.cpp`, `TextureDecoder_Util.h` and `LookUpTables.h`.
They are research references, not linked runtime dependencies. The C decoder
is an independent bounded implementation. It uses bit replication rather
than normalized division when expanding channels, and CMPR's 3/8 blend.
The scanner checks the first image/palette only. The model renderer additionally
selects base image planes and palettes by static material names. Header u16
offsets +12/+14/+16 are name/image/palette counts; +20/+32 identify 12-byte
name records and 16-byte names. A static name record has frame count one at
+2, image selector at +4 and palette selector at +8. These selector meanings
are **probable**, supported by original indexing routines and asset correlation.
Animated names, mip chains and version-zero/one descriptors remain unsupported.

### Static TMdl geometry and materials — probable original semantics

Original `0x801C5F68` indexes 16-byte model section records; `0x801C63A0`
relocates geometry arrays and draw lists. The geometry section contains draw
and vertex-attribute tables. Draw records are 24 bytes: list offset +0,
32-byte list block count +4, material selector +6, first attribute +14,
attribute count +16, matrix-index flag +17. Original `0x801C7F18` shifts
the list block count left five before submitting it to the original GX routine.
The static loaders reject the matrix-index flag. The separate stored-pose
loader below reads indexed matrices and applies their transforms.

Attribute records are 12 bytes: array offset +0, element count +4,
GX attribute +6, stride +7, components +8, storage type +9, fraction bits
+10 and index mode +11. Original `0x801CCCC0` passes these fields to
the GX vertex-format/array routines. Position and first texture coordinate
arrays are reconstructed; normal/color indices are consumed and bounded but
their values are not used for inspection shading. Index modes 8/16 bits,
VAT-zero triangles, strips, fans and quads are supported. Direct attributes,
matrix/skinned draws and command/state families outside this subset fail.
GX encoding constants were checked against the
[libogc GX header](https://github.com/devkitPro/libogc/blob/master/gc/ogc/gx.h).

Original `0x801CA1D8` relocates 48-byte material records: shader name at +4,
texture count at +8 and texture names starting at +12. The native renderer
links static single-texture OnePass materials to their embedded MMAP names.
Other shaders use an explicit inspection fallback; original TEV shading,
external texture resolution and scene placement are not reconstructed.

The CPU renderer owns its depth buffer, orthographic inspection camera,
Z-up inspection basis and lighting. It uses nearest repeated texture sampling
and an inspection alpha cutoff of 128. These are chosen viewer behaviors,
not claims about the original camera or material render state. Geometry
comparisons are exact offline comparisons of original asset positions, UVs,
material selectors and triangle ordering; **no live original pixel match**
is claimed. Acceptance of 125 models does not prove complete game rendering.

### Animation startup — probable catalog, not match initialization

Direct context-one banks are TERF entries 103–114 and 121–122. All 14 pass
the existing native core relocation routine. Registry identities belong to
an owned arena starting at opaque identity `0x10000000`; this is an internal
choice, not an original RAM address. This registry is constructed independently
and does not assert the original registry allocation order or live addresses.

Entries 121 and 122 both have tag 12. They are retained as separate catalog
entries. Original match selection/lifetime rules must choose the appropriate
variant. Skeleton/manifest entries, cross-reference banks and the original
frame-payload relocation hook remain outside this startup path. Full frame
decoding, posing and skinning remain open. The animation duration scanner's
162 unsupported programs do not silently become successful programs.

## Build and reproduce

From the repository root, with private output directories on W::

```bash
python projects/nfl-street/tools/extract_native_assets.py \
  --iso cache/isos/813c4511ba3135059a13de424555647e8252254b.iso \
  --output /mnt/w/Daywater/private/nflstreet/GNNE69-r0/assets
bash projects/nfl-street/reconstruction/build_windows_asset_unpack.sh
bash projects/nfl-street/reconstruction/build_windows_asset_startup.sh
bash projects/nfl-street/reconstruction/build_windows_model_startup.sh
```

Windows preview (omit `--render-check` to keep its window open):

```text
W:\Daywater\research-temp\nflstreet-asset-startup.exe W:\Daywater\private\nflstreet\GNNE69-r0\assets\animdata.dat W:\Daywater\private\nflstreet\GNNE69-r0\assets\loaddata.dat W:\Daywater\research-temp\nflstreet-preview.bmp --render-check
```

The native unpacker takes an archive and an existing output directory. It
writes numbered private `.bin` files; decoded proprietary data stays on W:.
`verify_archive.py` takes `--dol`, `--assets`, `--unpacked` and `--report`.
`asset_checks.c` contains synthetic offline corruption/bounds checks and runs
on Linux and Windows. `texture_scan.c` takes a private text file listing
unpacked host paths, decodes first MMAP planes, and reports coverage.

Static field preview (left/right rotates the inspection camera):

```text
W:\Daywater\research-temp\nflstreet-model-startup.exe W:\Daywater\private\nflstreet\GNNE69-r0\assets\envrnmt.dat 4 W:\Daywater\research-temp\nflstreet-native-field.bmp
```

`W:\Daywater\Start-NFLStreet-ModelPreview.cmd` launches that viewer.
`model_checks.c` checks malformed geometry, unsupported matrix/state commands
and synthetic raster output. `asset_checks.c` also checks named image/palette
selection and rejection of ambiguous or animated names. `verify_models.py`
compares native diagnostic meshes with an independent reader of raw TMdl data;
all diagnostic outputs must stay on private W: storage.

Native startup inputs need no ISO, Dolphin, ModernGekko, savestate or captured
RAM registry. The build toolchain and all private binaries remain on W:.

## Private evidence

- `W:\Daywater\private\nflstreet\GNNE69-r0\assets\asset-manifest.json`
- `W:\Daywater\private\nflstreet\GNNE69-r0\unpacked\unpack-report.json`
- `W:\Daywater\research-temp\nflstreet-native-archive-verification-20261008.json`
- `W:\Daywater\research-temp\nflstreet-texture-scan-20261008.log`
- `W:\Daywater\research-temp\nflstreet-native-startup-render-20261008.log`
- `W:\Daywater\research-temp\native-terf-directory-20261008.asm`
- `W:\Daywater\research-temp\native-codec-dispatch-20261008.asm`
- `W:\Daywater\research-temp\native-lzh1-decoder-20261008.asm`
- `W:\Daywater\research-temp\native-lzh1-tree-20261008.asm`
- `W:\Daywater\research-temp\nflstreet-model-scan-20261008.log`
- `W:\Daywater\research-temp\nflstreet-windows-model-verification-20261008\verification.json`
- `W:\Daywater\research-temp\nflstreet-native-field-render-20261008.log`
- `W:\Daywater\research-temp\native-model-geometry-relocation-20261008.asm`
- `W:\Daywater\research-temp\native-model-attributes-20261008.asm`
- `W:\Daywater\research-temp\native-model-draw-20261008.asm`
- `W:\Daywater\research-temp\native-model-materials-20261008.asm`
- `W:\Daywater\research-temp\native-mmap-layout-20261008.asm`
- `W:\Daywater\research-temp\nflstreet-scene-verification-20261008\verification.json`
- `W:\Daywater\research-temp\nflstreet-scene-props-verification-20261008\verification.json`
- `W:\Daywater\research-temp\nflstreet-scene-package-identities-20261008.json`
- `W:\Daywater\research-temp\native-scene-array-relocation-20261008.asm`
- `W:\Daywater\research-temp\native-scene-object-instantiation-20261008.asm`
- `W:\Daywater\research-temp\nflstreet-scene-cluster-verification-20261008\verification.json`
- `W:\Daywater\research-temp\native-cluster-stored-pose-20261008.asm`
- `W:\Daywater\research-temp\native-short-angle-sine-20261008.asm`
- `W:\Daywater\research-temp\native-object-missing-definition-20261008.asm`
- `W:\Daywater\research-temp\native-object-definition-lookup-20261008.asm`

## Native scene preview — candidate assembly

`W:\Daywater\Start-NFLStreet-ScenePreview.cmd` offers all eight venues.
Arrow keys orbit the inspection camera; plus/minus zoom. This viewer runs
without Dolphin, ModernGekko, an ISO, a savestate or captured RAM.

```text
W:\Daywater\research-temp\nflstreet-scene-startup.exe W:\Daywater\private\nflstreet\GNNE69-r0\assets\envrnmt.dat Bron W:\Daywater\research-temp\nflstreet-bronx-props.bmp --props
```

`--props` loads sibling `objdefs.dat` and `objmodel.dat`; omitting it retains
the six-layer geometry preview. `--headless` exports an image, while
`--render-check` exercises an actual Windows paint and closes. An optional
private mesh dump supports independent comparisons.

Original `0x80026C34`/`0x80026CCC` select venue resources. The main mesh and
five field-pass meshes share the inspected asset coordinates. Version-105
environment relocation at `0x80108320` defines seventeen packed arrays.
`0x800D31F8` consumes object, trigger and crowd records: 36-byte records hold
three position floats, three angle floats and a u16 index into a 24-byte
name table. `environment.c` reproduces the bounds and name selection;
it does not interpret the BSP tail, physics or behavior flags.

Names resolve through original HSH1 algorithm 1: `0x801A8088` →
`0x801A8B20` → `0x801AA884`. The uppercase polynomial hash uses the seed
stored in each archive. XML `object.name` is not the resource identity.
Supported render references are read from the original `render.tideRef`.
Some original definitions have malformed unrelated XML; both readers parse
only the required render tag and skip comments.

Rotation follows the fixed-angle conversion and quaternion formula in
`0x800D3A70`, `0x801A3794` and `0x801A3008`. Host trigonometry approximates
the original lookup. `verify_scene_props.py` independently checks all eight
Windows outputs against original records and reports every omission. Original
asset identity/layout is **probable**; complete scene fidelity remains
**candidate**. No live original transform or pixel comparison was performed.

| Venue prefix | Records | Rendered | Nonvisual | Unsupported |
| --- | ---: | ---: | ---: | ---: |
| Beac | 81 | 16 | 65 | 0 |
| Bron | 65 | 38 | 27 | 0 |
| EARS | 55 | 22 | 33 | 0 |
| Junk | 66 | 7 | 59 | 0 |
| LEGEDAY | 62 | 0 | 62 | 0 |
| Lati | 92 | 20 | 71 | 1 |
| Roof | 75 | 15 | 60 | 0 |
| Ware | 90 | 25 | 65 | 0 |

This covers three original record groups, not every scene subsystem. Separate
flying-object records, special vehicles, effects and players remain outside
the assembly. Rendered spectators use asset poses, not animation poses.
Matrix draws now use `nflstreet_mesh_open_bind_pose`; the strict static APIs
retain their earlier rejection. This processes all 47 omitted matrix-driven
instances, adding 6,824 triangles, for 83,351 scene triangles overall.

### Stored cluster pose — probable layout, candidate render fidelity

Original `0x801CA334` relocates the Clst table. `0x801CA4E8` copies the current
renderer matrix into palette entry zero, then concatenates each referenced bone's local transform
with its parent **cluster** transform. Clst rows are four bytes: bone u8,
parent palette u8, interpolation weight u16. The weight applies when an
alternate animation pose is supplied; the viewer supplies none. Skel contains
count plus pointers to 16-byte names, 16-byte translation/parent records,
six-byte signed angle triples and twelve-byte scales. This bounded loader
supports unit scales, with host trigonometry approximating `0x80188BB0`.
The stored-pose inspection chooses identity as the initial renderer matrix.
The subsequent [native clip preview](native_player_animation.md) now decodes
one bank's rotations, applies cluster weights, and renders moving poses;
live actor playback and full original fidelity remain open.

Original `0x801C7F18` enables a direct position-matrix selector before vertex
attributes. The new loader consumes that selector, resolves indexed position
matrix loads (GX command 0x20), and transforms each vertex by the selected
cluster palette matrix. Indexed normal loads (0x28) are bounded and consumed;
inspection lighting still uses triangle normals. Unknown commands, unloaded
selectors, invalid palette/parent references, cycles, nonunit scales and
nonfinite translations fail. Partially overwritten matrix mappings are
invalidated. Matrix selectors are never dropped to draw untransformed points.

`verify_cluster_pose.py` independently reads the original hierarchy using
homogeneous matrix multiplication. `verify_scene_props.py` compares all eight
native scene outputs, including every former matrix omission. Base geometry,
material selectors and UVs match exactly; reconstructed placed positions
differ by at most 0.000003815 units. Referenced bone translations also match
the external SKL1 skeletons by original names. Synthetic Linux and Windows
checks exercise translated/rotated poses and malformed input rejection.
The original strict 14-mesh comparison still passes unchanged.

The remaining `LATI_FOUNTAIN` record has no HSH1 match in the original
definition archive. `0x800E8CC8` returns -1 on that lookup failure;
`0x800D2F60` explicitly logs "NOT ADDED" and skips object creation. The viewer
reports the missing definition and skips it too. This is probable original
skip behavior from module inspection, not a live observation or a substitute
fountain asset.

This is stored-pose reconstruction, not running deformation callbacks or
animation. External skeleton remapping, animated alternate poses, bird/fan
motion, physics, collision, shadows and original material state remain open.
No original live pose or pixel comparison was performed.

Scene-only inspection permits color indices outside their declared color
array count when the addressed span remains inside the original model file.
Positions and UVs retain strict bounds. The renderer uses opaque inspection
state and chosen shading/camera; original blending, vertex colors, TEV state,
draw scheduling and material families are unresolved. The current image is
visibly sparse and does not reproduce the original complete scene.

Nine QKL packages were extracted privately. Original loader `0x801A9C74`
supports FILS/DTLS traversal: each DTLS record is u16 kind, u16 file selector,
u32 entry, u32 DATA-relative offset. Across eight game packages, 1,205 kind-1
resources match decoded DAT bytes exactly; 48 `igdata` resources differ and
remain unexplained. Each venue package confirms its six environment meshes
and version-105 instance blob. QKL runtime selection is not implemented.

The baseline ModernGekko DLL remains SHA1
`dd521cc38b0db6bca98a0bb261570a2be5e6e0ed`. No original runtime, source or
saved match was modified by these offline experiments.

## Remaining toward the full requested milestone

The [independent original-camera field renderer](native_field_render.md) now
reconstructs 3,029 EARS draws with original vertex colors, 87 selected texture
planes, ambient lighting, perspective, alpha blending and field-pass depth
modes. It runs in Windows C without emulator libraries, ISO or saved RAM.
Draw order/camera/texture selection come from a private measured profile;
complete scene behavior and exact pixel matching remain open.

The [original rendering capture](original_render_capture.md) now supplies a
repeated saved-match GPU reference: all 10,638 draws, command bytes, initial
GPU state and ordered memory updates match across two fresh research runs.
Exact position arrays identify EARS and its five field passes, including
distinct depth-write modes. The native viewer does not apply them yet.

1. Original manifest traversal and match-dependent bank selection; native
   frame decompression/posing and materials.
2. Matrix/skinned TMdl geometry, original materials, complete scene assembly and
   camera; extend the independent static mesh renderer to animated actors.
3. Fresh roster, field, actor, ball, event and match initialization; connect
   the previously checked simulation slices in the correct update order.
4. Input, audio, UI behavior, collision, rules and a full playable match.

The present result completes DAT container/decompression coverage for this
revision and establishes actual native texture startup. **Full asset
interpretation, original game rendering, and native match startup are not complete.**


## Live controllable player

The [live Windows viewer](native_live_player.md) now caches original assets and
field color/depth, evaluates interpolated native poses and renders a controllable
player each update. WASD/arrows move; R resets; Escape closes. Use the private
`W:\Daywater\Start-NFLStreet-LivePlayer.cmd` launcher. Keyboard speed, clip
selection and placement are inspection rules; original locomotion and native
match startup remain unfinished. See that document for checks and confidence.

The [reconstructed movement mode](native_movement_player.md) now joins checked
acceleration, speed and turning code to rendered player positions. Five Windows
runs match all 20 original recorded movement checkpoints. Its private launcher
is `W:\Daywater\Start-NFLStreet-MovementPlayer.cmd`; M switches to preserved
inspection movement. Native neutral/braking and fresh actor startup remain open.

[Native neutral/braking](native_braking.md) now slows the player to a held stop
on key release. Four original caller comparisons match 192 complete state
checkpoints, with corresponding real Windows rendering checks. The movement
launcher uses the updated braking build; general rest-to-direction transitions
and original animation selection remain open.
