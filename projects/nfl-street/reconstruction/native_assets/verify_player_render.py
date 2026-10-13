#!/usr/bin/env python3
"""Render the captured players from the verified draw submission and check its GPU-state and texture evidence."""

import json
from pathlib import Path

from verify_controller_checks import PRIVATE, baseline_hashes, digest, private, run, windows
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
    14: "(3658 GX calls) then 14 actors (4955 GX calls); 731 display lists, 26594 triangles (9957 culled, "
    "3077 clipped), 330 decoded textures, 957811 shaded pixels; digest ecc1092b.",
    15: "(3626 GX calls) then 14 actors (4955 GX calls); 731 display lists, 26594 triangles (9925 culled, "
    "3072 clipped), 330 decoded textures, 961967 shaded pixels; digest bec5c51a.",
    16: "(3658 GX calls) then 14 actors (4947 GX calls); 731 display lists, 26535 triangles (9910 culled, "
    "3072 clipped), 328 decoded textures, 957156 shaded pixels; digest 79bc5b7e.",
}
SOURCES = (
    "gx_render.c",
    "gx_render.h",
    "player_render_check.c",
    "player_draw.c",
    "player_draw.h",
    "scenery_draw.c",
    "scenery_draw.h",
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
    "exact original pixels, ground marks, ball, effect quads, further models, HUD and fresh selected actors are not "
    "claimed."
)


def main() -> None:
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / "research-temp/nflstreet-player-render-20261013")
    out.mkdir(parents=True, exist_ok=True)
    report = out / "verification.json"
    draw_report = private(PRIVATE / "research-temp/nflstreet-player-draw-20261013/verification.json")
    scenery_report = private(PRIVATE / "research-temp/nflstreet-scenery-draw-20261013/verification.json")
    entries = tuple("native_assets/" + name for name in SOURCES)
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY}
    try:
        for path, names in ((draw_report, DRAW_SOURCES), (scenery_report, SCENERY_SOURCES)):
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
                "Scene render: sky, EARS field passes, shadows, lines, rings and main field "
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
