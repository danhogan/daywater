#!/usr/bin/env python3
"""Verify the native scenery draw and EARS venue loops against the original in three saved contexts."""

import json
from pathlib import Path

from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

VENUE = {
    14: "EARS venue as saved (2517 GX calls, 388 display lists, 20 instances drawn, 29 depth ranges, 20 fog calls); "
    "300 edited venue loops (1089613 GX calls, 175904 display lists, 5598 instances drawn); 900 single instances "
    "(316992 GX calls, 52864 display lists, 735 drawn, 54 frustum-culled, 620 fog calls); sky and shadows as saved "
    "(865 GX calls, 16 primitives, 808 pipe writes) and 300 edited (208401 GX calls, 3721 primitives, "
    "192258 pipe writes); digest 234a4d48.",
    15: "EARS venue as saved (2497 GX calls, 388 display lists, 20 instances drawn, 29 depth ranges, 0 fog calls); "
    "300 edited venue loops (1084933 GX calls, 175904 display lists, 5598 instances drawn); 900 single instances "
    "(316514 GX calls, 52864 display lists, 735 drawn, 54 frustum-culled, 142 fog calls); sky and shadows as saved "
    "(867 GX calls, 16 primitives, 808 pipe writes) and 300 edited (208777 GX calls, 3721 primitives, "
    "192258 pipe writes); digest 8b7f3adf.",
    16: "EARS venue as saved (2517 GX calls, 388 display lists, 20 instances drawn, 29 depth ranges, 20 fog calls); "
    "300 edited venue loops (1089613 GX calls, 175904 display lists, 5598 instances drawn); 900 single instances "
    "(316992 GX calls, 52864 display lists, 735 drawn, 54 frustum-culled, 620 fog calls); sky and shadows as saved "
    "(865 GX calls, 16 primitives, 808 pipe writes) and 300 edited (208401 GX calls, 3721 primitives, "
    "192258 pipe writes); digest d0abef02.",
}
NATIVE = {context: "Scenery draw: " + text for context, text in VENUE.items()}
ORIGINAL = {
    context: "Original scenery draw: 1502 runs of 801EC8E4, 80165DC4, 8015B670, 801655D8 and 801643F4 match every "
    f"GX call ({calls} calls), the render state and all RAM outside the oracle stack."
    for context, calls in ((14, 1618388), (15, 1613588), (16, 1618388))
}
SOURCES = (
    "scenery_draw.c",
    "scenery_draw.h",
    "draw_engine.c",
    "draw_engine.h",
    "scenery_draw_check.c",
    "scenery_draw_original_check.c",
    "build_windows_scenery_draw_check.sh",
    "build_windows_scenery_draw_original_check.sh",
    "verify_scenery_draw.py",
)
BOUNDARY = (
    "Native 801EC8E4 scenery instance draw (frustum test 801CB3EC/801CB430/801CB708 with 801CAE78 corners and "
    "801CB05C clip codes, shader override, matrices, blend/alpha/depth presets and ranges, fog switch 801EDDF8/"
    "801EDD74, channel controls, chunks or group variant through the shared 801C7xxx engine) and the EARS venue loops "
    "80165DC4 (main field) and 8015B670/8015B564 (field passes with 801ECD98 ambient and 801ECD54 depth writes). "
    "Compared as GX SDK calls, render-state globals and all RAM against the original module with the matrix-stack "
    "updates (801893C8, 8018941C, 80189BB0, 8018949C, 80189ED8) and the unmodeled callees 800D75EC, 801652BC, "
    "8015A5CC and 8015A7C4 returning at once: instances keep their stored model-view matrices. The sky 801655D8 and "
    "actor shadows 801643F4 run unhooked (immediate vertex data compared as GX pipe writes). Venue objects are "
    "the ones the original scene traversal draws in the saved match."
)


def checked(command: list[str], log: Path, expected: str, failure: str) -> str:
    """Run one check, require its exact summary output and return the log digest."""
    if run(command, log).replace("\r\n", "\n").strip() != expected:
        raise RuntimeError(failure)
    return str(digest(log))


def main() -> None:
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / "research-temp/nflstreet-scenery-draw-20261013")
    out.mkdir(parents=True, exist_ok=True)
    report = out / "verification.json"
    entries = tuple("native_assets/" + name for name in SOURCES)
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY}
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        contexts = {i: PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in (14, 15, 16)}
        inputs = {str(path): digest(path) for path in contexts.values()}
        for name in ("scenery_draw", "scenery_draw_original"):
            run(["bash", str(here / f"build_windows_{name}_check.sh"), str(out)], out / f"{name}-build.log")
        plain = out / "scenery-draw-check.exe"
        original = out / "scenery-draw-original-check.exe"
        native_logs = [
            checked(
                [str(plain), windows(contexts[14])],
                out / f"native-{repeat}.log",
                NATIVE[14],
                "Incomplete native scenery draw coverage",
            )
            for repeat in range(2)
        ]
        if native_logs[0] != native_logs[1]:
            raise RuntimeError("Fresh native scenery draw traces differ")
        original_logs: dict[str, list[str]] = {}
        for i, context in contexts.items():
            traces = [
                checked(
                    [str(original), windows(context), windows(BASELINES["original_dll"][0])],
                    out / f"original-{i}-{repeat}.log",
                    NATIVE[i] + "\n" + ORIGINAL[i],
                    f"Original scenery draw comparison failed in context {i}",
                )
                for repeat in range(2)
            ]
            if traces[0] != traces[1]:
                raise RuntimeError("Fresh original scenery draw traces differ")
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
            venue_instances=20,
            controlled_loops_per_context=300,
            controlled_instances_per_context=900,
            original_runs_per_process=1502,
            sky_shadow_cases_per_context=301,
            original_gx_calls={"14": 1618388, "15": 1613588, "16": 1618388},
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print("Original scenery draw passes; private report " + str(report))


if __name__ == "__main__":
    main()
