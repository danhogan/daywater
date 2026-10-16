#!/usr/bin/env python3
"""Verify the native crowd draw (800D2F20 -> 80168234/80168E08) against the original in three saved contexts."""

import json
from pathlib import Path

from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

EXPECTED = {
    14: "Original crowd: 18045 runs of 800D2F20 over 45 entries x 401 cases match 184573 GX calls "
    "(4702 display lists), all RAM and the locked cache; 3106 model draws skinned their palettes, "
    "7610 model and copy draws completed; digest 37db9d9e.",
    15: "Original crowd: 18045 runs of 800D2F20 over 45 entries x 401 cases match 160528 GX calls "
    "(2903 display lists), all RAM and the locked cache; 2463 model draws skinned their palettes, "
    "7610 model and copy draws completed; digest 13a6abf4.",
    16: "Original crowd: 18045 runs of 800D2F20 over 45 entries x 401 cases match 184573 GX calls "
    "(4702 display lists), all RAM and the locked cache; 3106 model draws skinned their palettes, "
    "7610 model and copy draws completed; digest 9fba097b.",
}
RENDERED = {
    14: "Scene without crowd: 14952 venue GX calls, 32337 in all; 732 display lists, 28564 triangles (9965 culled, 3597"
    " clipped), 1288584 shaded pixels; digest 90962aa4.\nScene with crowd: 15551 venue GX calls, 32936 in all; 752 "
    "display lists, 31456 triangles (11002 culled, 3597 clipped), 1289043 shaded pixels; digest 7a414a64.",
    15: "Scene without crowd: 14949 venue GX calls, 30944 in all; 732 display lists, 28460 triangles (9948 culled, 3592"
    " clipped), 1290660 shaded pixels; digest 79e21c4c.\nScene with crowd: 15548 venue GX calls, 31543 in all; 752 "
    "display lists, 31352 triangles (10991 culled, 3592 clipped), 1291127 shaded pixels; digest 7e42426e.",
    16: "Scene without crowd: 15002 venue GX calls, 31774 in all; 732 display lists, 28493 triangles (9939 culled, 3589"
    " clipped), 1287325 shaded pixels; digest c53cf63f.\nScene with crowd: 15601 venue GX calls, 32373 in all; 752 "
    "display lists, 31385 triangles (10980 culled, 3589 clipped), 1287788 shaded pixels; digest a9be1328.",
}
SOURCES = (
    "crowd_draw.c",
    "crowd_draw.h",
    "crowd_draw_original_check.c",
    "crowd_render_check.c",
    "scenery_draw.c",
    "marks_draw.c",
    "cluster_palette.c",
    "model_view.c",
    "matrix_stack.c",
    "draw_engine.c",
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
    "camera_projection.c",
    "camera_projection.h",
    "effects_draw.c",
    "effects_draw.h",
    "actor_scale.c",
    "actor_scale.h",
    "actor_palettes.c",
    "gekko_quaternion/quaternion_kernels.c",
    "gekko_quaternion/quaternion_kernels.h",
    "gekko_quaternion/actor_slerp.c",
    "gekko_quaternion/actor_slerp.h",
    "actor_dynamics.c",
    "actor_dynamics.h",
    "actor_dynamics_math.c",
    "actor_dynamics_math.h",
    "gx_texobj.c",
    "texture.c",
    "build_windows_crowd_draw_original_check.sh",
    "build_windows_crowd_render_check.sh",
    "verify_crowd_draw.py",
    "cloud_draw.c",
    "cloud_draw.h",
    "field_overlay.c",
    "field_overlay.h",
    "player_draw.h",
    "gx_render.h",
    "gekko_float.h",
)
BOUNDARY = (
    "Native 800D2F20 for the 45 entries of the saved scene node (EARS props through hook 800D2860, garbage cans and "
    "the ten spectators through 80168234, their copies through 80168E08): lights, presets, model and model-view "
    "matrices (801897B8 quaternion), the 801EC298 frustum test, the 8016870C/801687FC hide test, CPU skinning "
    "(801EAC78, 801CABC4, 801C83A0/801C7FB4 into the 801C6344 ring), GPU skinning and rigid draws (801EC140). "
    "Compared call by call and as all RAM and the locked cache against the original module with the game's GQR "
    "values (8015668C), the OS cache routines returning and the locked-cache DMA 801F0DD4 served from the oracle's "
    "locked cache. Not modeled (rejected natively): the +13C distance fade, 801C6BF4 override allocation, other "
    "hooks. The render check is an inspection render, not a pixel claim."
)


def main() -> None:
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / "research-temp/nflstreet-crowd-draw-20261014")
    out.mkdir(parents=True, exist_ok=True)
    report = out / "verification.json"
    entries = tuple("native_assets/" + name for name in SOURCES)
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY}
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        contexts = {i: PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in (14, 15, 16)}
        inputs = {str(path): digest(path) for path in contexts.values()}
        run(["bash", str(here / "build_windows_crowd_draw_original_check.sh"), str(out)], out / "build.log")
        run(["bash", str(here / "build_windows_crowd_render_check.sh"), str(out)], out / "render-build.log")
        exe = out / "crowd-draw-original-check.exe"
        render = out / "crowd-render-check.exe"
        logs: dict[str, list[str]] = {}
        images: dict[str, list[str]] = {}
        for i, context in contexts.items():
            traces = []
            for repeat in range(2):
                log = out / f"original-{i}-{repeat}.log"
                text = run([str(exe), windows(context), windows(BASELINES["original_dll"][0])], log)
                if text.replace("\r\n", "\n").strip() != EXPECTED[i]:
                    raise RuntimeError(f"Original crowd comparison failed in context {i}")
                traces.append(str(digest(log)))
            if traces[0] != traces[1]:
                raise RuntimeError("Fresh original crowd traces differ")
            logs[str(i)] = traces
            plain, crowd = out / f"scene-nocrowd-{i}.bmp", out / f"scene-crowd-{i}.bmp"
            text = run([str(render), windows(context), windows(plain), windows(crowd)], out / f"render-{i}.log")
            if text.replace("\r\n", "\n").strip() != RENDERED[i]:
                raise RuntimeError(f"Crowd render differs in context {i}")
            images[str(i)] = [str(digest(plain)), str(digest(crowd))]
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
            entries=45,
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print("Original crowd draw passes; private report " + str(report))


if __name__ == "__main__":
    main()
