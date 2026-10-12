# Original rendering reference — 2026-10-08

The subsequent [independent field renderer](native_field_render.md) now applies
a bounded subset of these measured settings to native EARS geometry. This
document records the original capture; full rendering remains unfinished.

## Result

Two fresh research runs restored the supplied GNNE69 revision-0 saved match
and captured the same full GPU frame. Defined initial GPU state, command bytes,
ordered RAM update metadata/payloads, and all 10,638 decoded draw states match
exactly. Confidence: **verified for this saved match and research build**.

The independent native viewer still assembles stored venue geometry. It does
not yet reproduce these original rendering settings or run a playable match.

The research host uses the existing local ModernGekko/Dolphin build, JIT mode,
Vulkan, and an isolated user profile. The inspector links the existing Dolphin
opcode decoder as a research dependency. Neither is part of the independent
native game. The normal host source and reference DLL were not modified.
Reference DLL SHA1 remains `dd521cc38b0db6bca98a0bb261570a2be5e6e0ed`.
DOL SHA1: `46e3de854998a0f66c9be8c72a1164d22cd29eb4`.

## What the original frame actually does

| Observation | Repeated result |
|---|---:|
| Primitive draw commands | 10,638 |
| Commands in full frame | 37,763 |
| GPU command bytes | 456,281 |
| Ordered RAM updates | 8,438 |
| Draws with one TEV stage | 9,929 |
| Draws with three TEV stages | 709 |
| Draws with blending enabled | 4,540 |
| Draws with blending disabled | 6,098 |

These are draw commands, not counts of game objects. Captured state includes
BP registers, CP vertex descriptors/formats and array bases/strides, selected
position matrices, projection, and XF registers at each primitive command.
RAM updates precede the relevant command, following the original FIFO player.

`match_fifo_models.py` matches whole declared original TMdl position arrays
against their bytes in captured draw RAM. It finds 1,431 matching draws in
the environment/object/static catalogs. The field is **EARS**, identified by
exact bytes rather than its screenshot. All six EARS layers are present:

| Original geometry | Unique matched draws | Recorded depth state |
|---|---:|---|
| EARS_rField | 1,048 | test on, LEQUAL, write on |
| EARS_FieldPass1 | 190 | test on, LEQUAL, write on |
| EARS_FieldPass2 | 14 | test on, LESS, write off |
| EARS_FieldPass3 | 27 | test on, LESS, write off |
| EARS_FieldPass4 | 1 | test on, LESS, write off |
| EARS_FieldPass5 | 15 | test on, ALWAYS, write off |

All matched EARS draws have one TEV stage and source-alpha blending
(`src_alpha`, `1-src_alpha`). Most use stage-0 color `0x18f8af` and alpha
`0x18f2f0`: texture multiplied by raster color/alpha, with scale 2 and clamp.
This describes captured configuration; exact integer rounding, vertex lighting,
sampling and final pixel behavior have not been reconstructed. Some base-field
draws use a raster-only configuration. Matrix and projection match across layers.

Byte identity verifies geometry sources, not specific instances or materials.
Shared/duplicate arrays retain all matching sources, so source counts can
exceed unique draw counts. Actor arrays and the three-stage shader meaning
remain unresolved.

## Public knowledge used

- [Dolphin FIFO Player overview](https://github.com/dolphin-emu/dolphin/wiki/FIFO-Player-Overview): recording, inspection and limitations.
- [FIFO recorder](https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/Core/FifoPlayer/FifoRecorder.cpp) and local `FifoDataFile.cpp`/`FifoPlayer.cpp`: format and memory replay. Use the completion callback; this local build's `IsRecordingDone()` can become true before all frames arrive.
- [BP register definitions](https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoCommon/BPMemory.h), [pixel shader generator](https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoCommon/PixelShaderGen.cpp), [vertex shader generator](https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoCommon/VertexShaderGen.cpp): register and combiner semantics. Interpretation was checked against the actual local build's definitions.
- [devkitPro GX header](https://github.com/devkitPro/libogc/blob/master/gc/ogc/gx.h): public GX API definitions.

Public sources explain GameCube semantics. Game settings come from our capture.

## Reproduction and private evidence

Private root: `W:\Daywater\research-temp\nflstreet-render-capture-20261008`.
`capture.cmd` and `repeat.cmd` launch isolated saved-state captures.
Each DFF contains an initial partial boundary (45 bytes, no draws), followed
by one full rendered frame. Defined-content SHA256:
`94248a0d6c8d26918e35797de4df0b0f567bf0549e3bea19d719ad019e9b4481`.
Reserved padding is excluded.

`prepare_fifo_capture.py --output <private W directory>` generates private
host source and build response files from the existing local build.
`--inspector` generates the standalone decoder. Its bounded DFF v6 reader
avoids emulator memory initialization. Truncated and invalid-version files
were exercised and rejected. Inspector arguments: `<capture.dff> <state.json>`.

```bash
python3 projects/nfl-street/reconstruction/native_assets/verify_fifo_capture.py /mnt/w/Daywater/research-temp/nflstreet-render-capture-20261008
python3 projects/nfl-street/reconstruction/native_assets/match_fifo_models.py /mnt/w/Daywater/research-temp/nflstreet-render-capture-20261008
```

`verification.json`, `model-matches.json`, per-draw JSON, screenshots, builds,
logs and rejection checks stay private. Native EARS output is `native-ears.bmp`
and `.png`; `comparison.html` shows both images.

## Limits and next rendering milestone

The original screenshot was requested after recording and shows a nearby
frame, **not** a guaranteed image of the captured frame. Native EARS uses an
overview camera, inspection shading and stored poses. Visual comparison shows
the substantial fidelity gap; it is not a pixel acceptance test.

Next: reconstruct original color attributes and material/texture state for
matched EARS draws, apply the captured camera and per-pass depth/blend modes in
a bounded independent renderer, and obtain an exact-frame rendered reference.
Capturing settings does not finish animation, scene assembly or native startup.
