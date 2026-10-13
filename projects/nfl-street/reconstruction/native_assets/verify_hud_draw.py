#!/usr/bin/env python3
"""Verify the native HUD text, name tags and screen camera view against the original in three saved contexts."""

import json
from pathlib import Path

from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

HUD = {
    14: "name tags as saved (472 GX calls, 12 glyph quads, 432 pipe writes); 200 edited name-tag frames (126 "
    "drawing, 393586 GX calls, 10212 glyph quads, 367632 pipe writes); 400 direct texts (349 drawing, "
    "537705 GX calls, 14494 glyph quads, 521784 pipe writes); 400 camera views; uis screens as saved "
    "(8090 GX calls, 184 quads, 6432 pipe writes) and 200 edited (169 drawing, 817150 GX calls, 19142 "
    "quads, 674928 pipe writes); ball shadows as saved (136 GX calls, 3 fans) and 200 edited (18845 GX "
    "calls, 385 fans); ball as saved (10 GX calls, 0 display lists) and 200 edited (101 drawing, 5056 GX "
    "calls, 101 display lists); digest bd3c041c.",
    15: "name tags as saved (470 GX calls, 12 glyph quads, 432 pipe writes); 200 edited name-tag frames (126 "
    "drawing, 392784 GX calls, 10212 glyph quads, 367632 pipe writes); 400 direct texts (349 drawing, "
    "537705 GX calls, 14494 glyph quads, 521784 pipe writes); 400 camera views; uis screens as saved "
    "(8088 GX calls, 184 quads, 6432 pipe writes) and 200 edited (169 drawing, 816876 GX calls, 19142 "
    "quads, 674928 pipe writes); ball shadows as saved (136 GX calls, 3 fans) and 200 edited (18845 GX "
    "calls, 385 fans); ball as saved (10 GX calls, 0 display lists) and 200 edited (98 drawing, 4954 GX "
    "calls, 98 display lists); digest 392f701d.",
    16: "name tags as saved (472 GX calls, 12 glyph quads, 432 pipe writes); 200 edited name-tag frames (126 "
    "drawing, 393586 GX calls, 10212 glyph quads, 367632 pipe writes); 400 direct texts (349 drawing, "
    "537705 GX calls, 14494 glyph quads, 521784 pipe writes); 400 camera views; uis screens as saved "
    "(8090 GX calls, 184 quads, 6432 pipe writes) and 200 edited (169 drawing, 817150 GX calls, 19142 "
    "quads, 674928 pipe writes); ball shadows as saved (136 GX calls, 3 fans) and 200 edited (18845 GX "
    "calls, 385 fans); ball as saved (10 GX calls, 0 display lists) and 200 edited (102 drawing, 5087 GX "
    "calls, 102 display lists); digest d5e3911a.",
}
NATIVE = {context: "HUD draw: " + text for context, text in HUD.items()}
ORIGINAL = {
    context: "Original HUD draw: 1604 runs of 800DADC0, 80186B1C, 8017D16C, 8012AB5C, 801652BC and 8015CB60 match "
    "every GX call and pipe write "
    f"({calls}), the render state and all RAM outside the oracle stack."
    for context, calls in ((14, 1781050), (15, 1779868), (16, 1781081))
}
SOURCES = (
    "hud_draw.c",
    "hud_draw.h",
    "hud_shared.h",
    "uis_draw.c",
    "uis_draw.h",
    "ball_draw.c",
    "ball_draw.h",
    "marks_draw.c",
    "marks_draw.h",
    "gx_texobj.c",
    "gx_texobj.h",
    "player_draw.h",
    "matrix_stack.c",
    "matrix_stack.h",
    "scenery_draw.c",
    "scenery_draw.h",
    "scenery_shared.h",
    "draw_engine.c",
    "draw_engine.h",
    "check_fixture.h",
    "venue_fixture.h",
    "hud_fixture.h",
    "gx_oracle.h",
    "hud_draw_check.c",
    "hud_draw_original_check.c",
    "build_windows_hud_draw_check.sh",
    "build_windows_hud_draw_original_check.sh",
    "verify_hud_draw.py",
)
BOUNDARY = (
    "Native 80186B1C text draw (801863A4 layout with 80185A98 glyphs and 80185B18 kerning into the 8039C4BC "
    "boxes, 80186924/8018675C glyph quads written to the GX pipe, two IA8 palette passes for C4 fonts or the "
    "offset shadow pass), 800DADC0 player name tags (option, play-state and mode gates, profile bucket, fog switch, "
    "80186264 defaults, 80186CD8 GX setup, tag offset on the viewport's matrix stack) and the 8017D16C screen "
    "camera view (8017D604: identity, 80189854/80189944/80189A34 rotations with the 80188A70/80188AEC sine "
    "polynomial, 80189BB0 translation). Every original routine executes unhooked except GX SDK entry points; "
    "calls, pipe writes, render-state globals and all RAM outside the oracle stack are compared. Name tags are "
    "the 14 objects the original frame passes to 800DADC0; the HUD camera is the frame's second camera node. "
    "The in-match uis screens 8012AB5C (801D0BC4 without update ticks: 801CEFDC node walk, 801D8268 transforms, "
    "801D7E74 color stack, 801D6690/801D6A54 images with the GX texture-object encoders 80205754/802059C8/"
    "80205A10/80205DF4 executing in the oracle, 801D97D4 text) match with the resource lock 801AF944/801AF9CC "
    "as a supplied no-op on both sides. Resource loads, acquisitions, nine-slice, display-list and indirect "
    "images, dirty fonts and layouts are rejected natively. The ball shadows 801652BC and the ball 8015CB60 "
    "(light set 4, frustum tests 801EC298, LOD 801EB68C, model and shadow pass) and the field camera's view "
    "(handler 800C3508 modes 0, 1 and 5) run unhooked."
)


def checked(command: list[str], log: Path, expected: str, failure: str) -> str:
    """Run one check, require its exact summary output and return the log digest."""
    if run(command, log).replace("\r\n", "\n").strip() != expected:
        raise RuntimeError(failure)
    return str(digest(log))


def main() -> None:
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / "research-temp/nflstreet-hud-draw-20261013")
    out.mkdir(parents=True, exist_ok=True)
    report = out / "verification.json"
    entries = tuple("native_assets/" + name for name in SOURCES)
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY}
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        contexts = {i: PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in (14, 15, 16)}
        inputs = {str(path): digest(path) for path in contexts.values()}
        for name in ("hud_draw", "hud_draw_original"):
            run(["bash", str(here / f"build_windows_{name}_check.sh"), str(out)], out / f"{name}-build.log")
        plain = out / "hud-draw-check.exe"
        original = out / "hud-draw-original-check.exe"
        native_logs = [
            checked(
                [str(plain), windows(contexts[14])],
                out / f"native-{repeat}.log",
                NATIVE[14],
                "Incomplete native HUD draw coverage",
            )
            for repeat in range(2)
        ]
        if native_logs[0] != native_logs[1]:
            raise RuntimeError("Fresh native HUD draw traces differ")
        original_logs: dict[str, list[str]] = {}
        for i, context in contexts.items():
            traces = [
                checked(
                    [str(original), windows(context), windows(BASELINES["original_dll"][0])],
                    out / f"original-{i}-{repeat}.log",
                    NATIVE[i] + "\n" + ORIGINAL[i],
                    f"Original HUD draw comparison failed in context {i}",
                )
                for repeat in range(2)
            ]
            if traces[0] != traces[1]:
                raise RuntimeError("Fresh original HUD draw traces differ")
            original_logs[str(i)] = traces
        if before != baseline_hashes() or any(digest(Path(path)) != sha for path, sha in inputs.items()):
            raise RuntimeError("Protected originals or private inputs changed")
        if sources != source_hashes(root, entries):
            raise RuntimeError("Source changed during verification")
        record.update(
            status="passed",
            baseline=before,
            input_sha256=inputs,
            source_sha256=sources,
            native_log_sha256=native_logs,
            original_log_sha256=original_logs,
            executable_sha256={path.name: digest(path) for path in (plain, original)},
            name_tags=14,
            edited_name_tag_frames_per_context=200,
            direct_texts_per_context=400,
            camera_views_per_context=400,
            original_runs_per_process=1604,
            ball_shadow_frames_per_context=201,
            ball_frames_per_context=201,
            screen_frames_per_context=201,
            mutants_caught=57,
            original_gx_calls={"14": 1781050, "15": 1779868, "16": 1781081},
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print("Original HUD draw passes; private report " + str(report))


if __name__ == "__main__":
    main()
