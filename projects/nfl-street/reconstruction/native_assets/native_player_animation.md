# First independent player animation preview

NFL Street, GameCube GNNE69 revision 0; DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`.

The Windows C preview now decodes an original clip, evaluates body and
template-head cluster matrices, and renders moving poses from original mesh
and texture assets. It does not link Dolphin, ModernGekko, or generated
PowerPC code. Daywater remains separate.

## What works

- Direct common animation bank: private `animdata` entry 103.
- Clip 75: 27 original frame references, including a repeated first pose.
  Its gameplay meaning is not established; this is not labeled an idle/run clip.
- Native compressed rotation decoder: all 18,752 bank frames, 78 components
  each, exactly equal to the original module's decoder and reconstruction.
- Body: 75 model bones and 128 weighted cluster transforms.
- Head: original BaseHead template, its own cluster chain and shared named
  animation bones. Hair attaches to the body's full-weight neck transform.
- Original body/head/hair geometry and textures: 2,092 triangles, 13 texture
  planes, as in the earlier player preview.
- Two independent Windows renders: all 27 BMPs repeat byte for byte;
  26 distinct images, consistent with the repeated source pose.
- Native Windows timer playback: 29 paints and 29 ticks across a full loop.

Private outputs and launchers are on W:. Open
`W:\Daywater\Start-NFLStreet-AnimationPreview.cmd` for looping Windows
playback, or inspect
`W:\Daywater\research-temp\nflstreet-animation-20261008\native-player-animation.gif`.

**The viewer preloads frames rendered by native code.** It does not evaluate
and render a live actor on each timer tick. Playback uses a chosen 67 ms
inspection interval, without an original-game timing claim. Root travel,
interpolation, event/callback execution, clip transitions, keyboard movement,
and native match startup remain unfinished. It is not yet a playable game.

## Evidence and confidence

### Rotation decoding — verified against original module

`pose_original_check.c` is a research-only oracle, separate from the native
preview. It dispatches the unchanged original module, ABI 3 / CPU ABI 4:

- `0x8015D0DC`: packed frame header size, 10 plus optional six-byte blocks.
- `0x80151ABC`: MSB-first bit extraction.
- `0x8015D104`, non-scattered branch: ordered signed quantized components.
- `0x801667CC`: signed base + arithmetic-shifted residual × scale.

The standalone decoder compares every reconstructed signed 16-bit component
with the original routines. This is controlled original-module execution,
not a new live Dolphin capture. Bank header/frame-table/compression-descriptor
fields are big endian. Descriptor +0 is component count; +4/+8 identify the
base/scale s16 arrays, +20 the two-byte component descriptors. Their second
byte supplies bit width. The non-scattered branch preserves component order;
the first descriptor byte must not be applied as a normal pose permutation.
The initial incorrect permutation was corrected before retaining the preview.

Native bounds checks reject truncated headers, invalid bit widths, insufficient
output capacity, and out-of-range frame indices without changing output.

### Weighted cluster evaluation — probable; approximate trigonometry

Original `0x801CA4E8` interpolates each bone's stored signed angles toward the
animation angles with its Clst u16 weight / 4096, using an arithmetic shift
before constructing and concatenating the local matrix. This interpolates
angles, not completed matrices. Parent references are cluster indices.

The original first palette matrix copies the renderer's **current matrix**
through `0x80189ED8`, `0x8018939C` and `0x8018937C`. The standalone inspection
evaluator deliberately supplies identity as that starting transform.

The oracle loads the hash-checked DOL, initializes the original sine polynomial
with `0x801888F0`, sets the original small-data registers, enables floating
point / paired-single execution, and supplies an identity current-matrix stack.
Scratch RAM identities in the oracle are harness allocations, not live-game
memory findings. It bypasses only `0x801F0BD0` hardware cache flushing.

For one controlled perturbed 75-bone pose, all 128 native matrices are compared
with the original evaluator. The maximum float difference is approximately
`0.000241041183`; the comparison tolerance is `0.001`. Native code uses host
trigonometry; original code uses a truncated paired-single polynomial.
**These matrices are not byte-identical to the original.** Full animation and
rendered pixels have not been compared with a fresh original match.

### Compact skeleton and component assembly — probable/candidate

Original `SKL1` entry 95 names the compact skeleton. Its named translations
match the corresponding body model translations byte for byte in the checked
asset. Normal decoded components populate the first 26 compact bones and are
resolved to the model's names. Additional model bones retain stored angles.
The template head uses its own corresponding skeleton and cluster chain.
The two observed hair assets have identity-only Clst and a `neckhi` root;
native attachment to the body neck is an inspection implementation. Original
hair dynamics and appearance selection are not reconstructed.

Every referenced vertex's position and normal matrix selection is overwritten
from native matrices before rendering. Captured pose matrices are not used by
the moving preview. The existing NAP1 profile still supplies draw selection,
projection/viewport, material/light state and dynamic texture palettes.
The inspection camera recenters the mesh for each frame. Head personalization
and the unresolved secondary skin texture retain the earlier limitations.
Overall original actor playback remains **candidate**.

## Public research checked

[EA OTools](https://github.com/FIFAM-Mods/otools) is a useful nearby open source
model/skeleton project, but its documented inputs are ELF-based `.O` files
and its README marks animation unsupported. It is not a decoder for these
TMdl/SKL1 animation-bank inputs and was not incorporated. Searches for the
specific NFL Street animation formats did not locate a reusable matching
decoder. Existing local GX/Dolphin references continue to supply documented
matrix-index and rendering semantics; they are not native runtime dependencies.

## Reproduce

```bash
bash projects/nfl-street/reconstruction/build_windows_animation_preview.sh
```

Windows renderer arguments:

```text
animation-preview.exe <unpacked-root> <NAP1-profile> <direct-bank.bin>
                      <SKL1.bin> 75 <existing-private-frame-directory> 27
animation-window.exe <private-frame-directory> 27 67 [--render-check]
```

`pose_checks.c` links only `pose.c` and libm. The research-only
`pose_original_check.c` additionally requires the existing ModernGekko CPU/module
ABI headers and takes the original DLL, direct bank, optional model and DOL.
All generated artifacts, oracle logs, inputs and images must stay private on W:.

Next: native time interpolation and cached actor posing/rendering during
playback, then an identified locomotion clip and movement input. Original
callback/transition research can be connected after the rendering path can
advance a live actor.
