#!/usr/bin/env python3
"""Verify the native HUD text, name tags and screen camera view against the original in three saved contexts."""

import json
from pathlib import Path

from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

HUD = {
    14: "name tags as saved (472 GX calls, 12 glyph quads, 432 pipe writes); 200 edited name-tag frames (126 "
    "drawing, 393586 GX calls, 10212 glyph quads, 367632 pipe writes); 400 direct texts (349 drawing, 537705 GX "
    "calls, 14494 glyph quads, 521784 pipe writes); 400 screen camera views; digest 36d965d9.",
    15: "name tags as saved (470 GX calls, 12 glyph quads, 432 pipe writes); 200 edited name-tag frames (126 "
    "drawing, 392784 GX calls, 10212 glyph quads, 367632 pipe writes); 400 direct texts (349 drawing, 537705 GX "
    "calls, 14494 glyph quads, 521784 pipe writes); 400 screen camera views; digest 95244216.",
    16: "name tags as saved (472 GX calls, 12 glyph quads, 432 pipe writes); 200 edited name-tag frames (126 "
    "drawing, 393586 GX calls, 10212 glyph quads, 367632 pipe writes); 400 direct texts (349 drawing, 537705 GX "
    "calls, 14494 glyph quads, 521784 pipe writes); 400 screen camera views; digest c297ee5b.",
}
NATIVE = {context: "HUD draw: " + text for context, text in HUD.items()}
ORIGINAL = {
    context: "Original HUD draw: 1001 runs of 800DADC0, 80186B1C and 8017D16C match every GX call and pipe write "
    f"({calls}), the render state and all RAM outside the oracle stack."
    for context, calls in ((14, 931763), (15, 930959), (16, 931763))
}
SOURCES = (
    "hud_draw.c",
    "hud_draw.h",
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
    "The scoreboard, meters and other uis screens are not modeled."
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
            original_runs_per_process=1001,
            mutants_caught=18,
            original_gx_calls={"14": 931763, "15": 930959, "16": 931763},
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print("Original HUD draw passes; private report " + str(report))


if __name__ == "__main__":
    main()
