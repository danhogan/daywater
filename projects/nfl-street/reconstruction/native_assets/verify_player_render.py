#!/usr/bin/env python3
"""Render the captured players from the verified draw submission and check its GPU-state and texture evidence."""

import json
from pathlib import Path

from verify_controller_checks import PRIVATE, baseline_hashes, digest, private, run, windows
from verify_hud_draw import SOURCES as HUD_SOURCES
from verify_player_draw import SOURCES as DRAW_SOURCES
from verify_player_startup import source_hashes
from verify_scenery_draw import SOURCES as SCENERY_SOURCES

TEXTURES = (
    "Texture decode: 337 captured slot textures (4221952 pixels) decode identically from their GX texture/TLUT "
    "objects and their MMAP files."
)
STATE = (
    "Captured GPU state: 2/2 recorded one-stage player draw states are produced by the native calls; 2/2 native "
    "one-stage states were recorded."
)
RENDER = {
    14: "4955 GX calls, 340 display lists, 5480 primitives, 19631 triangles (9955 culled, 0 clipped), 254 decoded "
    "textures, 30685 shaded pixels, 22295 covered; digest fd5f2385.",
    15: "4955 GX calls, 340 display lists, 5480 primitives, 19631 triangles (9924 culled, 0 clipped), 254 decoded "
    "textures, 30180 shaded pixels, 21973 covered; digest db7ee221.",
    16: "4947 GX calls, 340 display lists, 5456 primitives, 19572 triangles (9909 culled, 0 clipped), 252 decoded "
    "textures, 30139 shaded pixels, 21608 covered; digest ac372641.",
}
SCENE = {
    14: "(5822 GX calls) then 14 actors (4955 GX calls) and the HUD (8562 GX calls); 732 display lists, 27752"
    " triangles (9954 culled, 3101 clipped), 346 decoded textures, 1286004 shaded pixels; digest "
    "a9ea2f0d.",
    15: "(5817 GX calls) then 14 actors (4955 GX calls) and the HUD (8558 GX calls); 732 display lists, 27754"
    " triangles (9944 culled, 3096 clipped), 345 decoded textures, 1288588 shaded pixels; digest "
    "14fd198a.",
    16: "(5872 GX calls) then 14 actors (4947 GX calls) and the HUD (8562 GX calls); 732 display lists, 27697"
    " triangles (9920 culled, 3097 clipped), 343 decoded textures, 1284616 shaded pixels; digest "
    "984ca239.",
}
SOURCES = (
    "gx_render.c",
    "gx_render.h",
    "player_render_check.c",
    "player_draw.c",
    "player_draw.h",
    "scenery_draw.c",
    "scenery_draw.h",
    "scenery_shared.h",
    "hud_draw.c",
    "hud_draw.h",
    "matrix_stack.c",
    "matrix_stack.h",
    "uis_draw.c",
    "uis_draw.h",
    "ball_draw.c",
    "ball_draw.h",
    "scene_frame.c",
    "actor_palettes.c",
    "actor_palettes.h",
    "cluster_palette.c",
    "cluster_palette.h",
    "model_view.c",
    "model_view.h",
    "scene_frame.h",
    "scene_fixture.h",
    "marks_draw.c",
    "marks_draw.h",
    "gx_texobj.c",
    "gx_texobj.h",
    "hud_shared.h",
    "venue_fixture.h",
    "hud_fixture.h",
    "draw_engine.c",
    "draw_engine.h",
    "texture.c",
    "texture.h",
    "build_windows_player_render_check.sh",
    "verify_player_render.py",
)
BOUNDARY = (
    "Software GX execution of the verified native 80161E10 submission for the 14 captured players of three saved "
    "contexts, alone and after the verified EARS field passes and main field (8015B670, 80165DC4) in one target: "
    "display lists with indexed matrix loads, transform, one-channel lighting, clipping, culling, mip "
    "sampling, integer TEV, alpha compare, depth and blending. Frame state comes from the saved GX shadow "
    "(projection, viewport, color mode, konst selections) and the 801C769C frame start (depth mode, texgens). "
    "Checked: every recorded one-stage player draw state of the 2026-10-08 original frame capture equals the SDK "
    "encoding of the native calls and vice versa; all non-empty captured slot textures decode identically through "
    "their GX objects and through the independent MMAP reader; renders repeat exactly. Inspection images only: "
    "exact original pixels, effect quads, further models and fresh selected actors are not claimed. The field "
    "view comes from the native field camera (8017D16C on node 806A1CA8, copied to stack slot 3 as 801DFAB0 does); "
    "in contexts 14 and 16 it is the camera after the drawn frame, which differs from the stored instance "
    "matrices by about 0.03 units. The name tags and in-match screens draw under the HUD camera's verified "
    "screen view, executed as each call is issued (the screens re-initialize shared texture objects per image)."
)


def main() -> None:
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / "research-temp/nflstreet-player-render-20261013")
    out.mkdir(parents=True, exist_ok=True)
    report = out / "verification.json"
    draw_report = private(PRIVATE / "research-temp/nflstreet-player-draw-20261013/verification.json")
    scenery_report = private(PRIVATE / "research-temp/nflstreet-scenery-draw-20261013/verification.json")
    hud_report = private(PRIVATE / "research-temp/nflstreet-hud-draw-20261013/verification.json")
    entries = tuple("native_assets/" + name for name in SOURCES)
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY}
    try:
        for path, names in (
            (draw_report, DRAW_SOURCES),
            (scenery_report, SCENERY_SOURCES),
            (hud_report, HUD_SOURCES),
        ):
            reference = json.loads(path.read_text())
            required = tuple("native_assets/" + name for name in names)
            if reference["status"] != "passed" or source_hashes(root, required) != reference["source_sha256"]:
                raise RuntimeError(f"{path} is not passed for the current sources")
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        contexts = {i: PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in (14, 15, 16)}
        profile = PRIVATE / "research-temp/nflstreet-render-capture-20261008/reference-player-profile.bin"
        inputs = {str(path): digest(path) for path in (*contexts.values(), profile)}
        run(["bash", str(here / "build_windows_player_render_check.sh"), str(out)], out / "player_render-build.log")
        exe = out / "player-render-check.exe"
        logs: dict[str, list[str]] = {}
        images: dict[str, str] = {}
        for i, context in contexts.items():
            image = out / f"players-{i}.bmp"
            scene = out / f"scene-{i}.bmp"
            expected = (
                f"Player render: 14 actors, {RENDER[i]}\n"
                "Scene render: field camera, sky, EARS field passes with ground marks and ball shadows, shadows, "
                "lines, rings, main field and ball "
                f"{SCENE[i]}\n{TEXTURES}\n{STATE}"
            )
            traces = []
            for repeat in range(2):
                log = out / f"render-{i}-{repeat}.log"
                text = run([str(exe), windows(context), windows(image), windows(scene), windows(profile)], log)
                if text.replace("\r\n", "\n").strip() != expected:
                    raise RuntimeError(f"Player render check failed in context {i}")
                traces.append(str(digest(log)))
            if traces[0] != traces[1]:
                raise RuntimeError("Fresh player render traces differ")
            logs[str(i)] = traces
            images[image.name] = str(digest(image))
            images[scene.name] = str(digest(scene))
        if before != baseline_hashes() or any(digest(Path(path)) != sha for path, sha in inputs.items()):
            raise RuntimeError("Protected originals or private inputs changed")
        if sources != source_hashes(root, entries):
            raise RuntimeError("Source changed during verification")
        record.update(
            status="passed",
            baseline=before,
            input_sha256=inputs,
            source_sha256=sources,
            draw_report_sha256=digest(draw_report),
            scenery_report_sha256=digest(scenery_report),
            hud_report_sha256=digest(hud_report),
            log_sha256=logs,
            image_sha256=images,
            executable_sha256=digest(exe),
            players_per_context=14,
            cross_decoded_textures=337,
            captured_states_matched=2,
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print("Native captured-player render passes; private report " + str(report))


if __name__ == "__main__":
    main()
