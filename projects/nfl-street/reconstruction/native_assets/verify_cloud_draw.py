#!/usr/bin/env python3
"""Verify the native field-camera node entry 800E398C (projection, sky, clouds) against the original."""

import json
from pathlib import Path

from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

ORACLE = (
    "Original sky node: saved frame 9430 GX calls and pipe writes (9296 pipe writes); 401 runs of 800E398C match "
    "3105756 GX calls and pipe writes (3063824 pipe writes, 714 projection changes), all RAM; 330 cloud passes drew "
    "23491 cloud strips; digest {}."
)
EXPECTED = {14: ORACLE.format("d621b617"), 15: ORACLE.format("1893cf17"), 16: ORACLE.format("c2c9bdf8")}
RENDERED = {
    14: "Scene with the sky alone: 6421 venue GX calls and writes, 19938 in all; 752 display lists, 10033 primitives, "
    "30644 triangles (10991 culled, 3101 clipped), 1286524 shaded pixels; digest 65ef5976.\nScene with the sky node: "
    "15528 venue GX calls and writes, 29045 in all; 752 display lists, 10113 primitives, 31206 triangles ("
    "10991 culled, 3579 clipped), 1286969 shaded pixels; digest 65ef5976.",
    15: "Scene with the sky alone: 6416 venue GX calls and writes, 19929 in all; 752 display lists, 10034 primitives, "
    "30646 triangles (10987 culled, 3096 clipped), 1289055 shaded pixels; digest c8817024.\nScene with the sky node: "
    "15523 venue GX calls and writes, 29036 in all; 752 display lists, 10114 primitives, 31208 triangles ("
    "10987 culled, 3574 clipped), 1289500 shaded pixels; digest c8817024.",
    16: "Scene with the sky alone: 6471 venue GX calls and writes, 19980 in all; 752 display lists, 10011 primitives, "
    "30589 triangles (10961 culled, 3097 clipped), 1285091 shaded pixels; digest 2e5381c6.\nScene with the sky node: "
    "15578 venue GX calls and writes, 29087 in all; 752 display lists, 10091 primitives, 31151 triangles ("
    "10961 culled, 3571 clipped), 1285535 shaded pixels; digest 2e5381c6.",
}
SOURCES = (
    "cloud_draw.c",
    "cloud_draw.h",
    "cloud_draw_original_check.c",
    "cloud_render_check.c",
    "scenery_draw.c",
    "marks_draw.c",
    "matrix_stack.c",
    "draw_engine.c",
    "crowd_draw.c",
    "cluster_palette.c",
    "model_view.c",
    "player_draw.c",
    "gx_render.c",
    "hud_draw.c",
    "uis_draw.c",
    "ball_draw.c",
    "scene_frame.c",
    "scene_frame.h",
    "scene_fixture.h",
    "light_rig.c",
    "light_rig.h",
    "light_rig_fixture.h",
    "camera_slots.c",
    "camera_slots.h",
    "name_tag_project.c",
    "name_tag_project.h",
    "marker_ram.h",
    "actor_palettes.c",
    "gx_texobj.c",
    "texture.c",
    "build_windows_cloud_draw_original_check.sh",
    "build_windows_cloud_render_check.sh",
    "verify_cloud_draw.py",
)
BOUNDARY = (
    "Native 800E398C, the field camera scene node's entry 1: the enable bytes r13-71E3/-71E4, GXSetZMode, "
    "GXGetProjectionv (GX shadow +420..+438) with words 2, 4 and 5 replaced by 802536CC through GXSetProjectionv, "
    "the sky 801655D8, the clouds 800D5B3C -> 8015F83C (sky color record 8015F164/8015F0E8/801DE858, colors "
    "8015F3F0, stack push, x quarter turn 80189854, position matrix, vertex/channel/TEV/blend state) -> 8015F450 per "
    "record (immediate quad strips), then the projection and z mode restored. Compared call by call, pipe write by "
    "pipe write and as all RAM against the original module, with GXSetProjectionv recorded as its seven argument "
    "words and GXGetProjectionv executing. GXSetProjectionv is emitted as op NFLSTREET_GX_OPS (not yet in "
    "player_draw.h's table). The render check is an inspection render (warm-up frame for persistent GX state), not "
    "a pixel claim."
)


def main() -> None:
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / "research-temp/nflstreet-cloud-draw-20261014")
    out.mkdir(parents=True, exist_ok=True)
    report = out / "verification.json"
    entries = tuple("native_assets/" + name for name in SOURCES)
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY}
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        contexts = {i: PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in (14, 15, 16)}
        inputs = {str(path): digest(path) for path in contexts.values()}
        run(["bash", str(here / "build_windows_cloud_draw_original_check.sh"), str(out)], out / "build.log")
        run(["bash", str(here / "build_windows_cloud_render_check.sh"), str(out)], out / "render-build.log")
        exe = out / "cloud-draw-original-check.exe"
        render = out / "cloud-render-check.exe"
        logs: dict[str, list[str]] = {}
        images: dict[str, list[str]] = {}
        for i, context in contexts.items():
            traces = []
            for repeat in range(2):
                log = out / f"original-{i}-{repeat}.log"
                text = run([str(exe), windows(context), windows(BASELINES["original_dll"][0])], log)
                if text.replace("\r\n", "\n").strip() != EXPECTED[i]:
                    raise RuntimeError(f"Original sky node comparison failed in context {i}")
                traces.append(str(digest(log)))
            if traces[0] != traces[1]:
                raise RuntimeError("Fresh original sky node traces differ")
            logs[str(i)] = traces
            sky, clouds = out / f"scene-sky-{i}.bmp", out / f"scene-clouds-{i}.bmp"
            text = run([str(render), windows(context), windows(sky), windows(clouds)], out / f"render-{i}.log")
            if text.replace("\r\n", "\n").strip() != RENDERED[i]:
                raise RuntimeError(f"Sky node render differs in context {i}")
            images[str(i)] = [str(digest(sky)), str(digest(clouds))]
        if before != baseline_hashes() or any(digest(Path(path)) != sha for path, sha in inputs.items()):
            raise RuntimeError("Protected originals or private inputs changed")
        if sources != source_hashes(root, entries):
            raise RuntimeError("Source changed during verification")
        record.update(
            status="passed",
            baseline=before,
            input_sha256=inputs,
            source_sha256=sources,
            original_log_sha256=logs,
            image_sha256=images,
            executable_sha256=digest(exe),
            cases_per_context=401,
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print("Original sky node and cloud draw passes; private report " + str(report))


if __name__ == "__main__":
    main()
