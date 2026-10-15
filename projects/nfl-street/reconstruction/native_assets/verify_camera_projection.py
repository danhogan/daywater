#!/usr/bin/env python3
"""Verify the native camera slot builders 801DF8C4 and 801DF938 against the original in three saved contexts."""

import json
from pathlib import Path

from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

EXPECTED = "Camera projection builders: 1203 node cases (saved plus 400 edited per node) match all RAM."
SOURCES = (
    "camera_projection.c",
    "camera_projection.h",
    "camera_slots.h",
    "marker_ram.h",
    "matrix_stack.c",
    "matrix_stack.h",
    "draw_engine.c",
    "draw_engine.h",
    "check_fixture.h",
    "venue_fixture.h",
    "camera_projection_original_check.c",
    "build_windows_camera_projection_original_check.sh",
    "verify_camera_projection.py",
)


def main() -> None:
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / "research-temp/nflstreet-camera-projection-20261014")
    out.mkdir(parents=True, exist_ok=True)
    report = out / "verification.json"
    entries = tuple("native_assets/" + name for name in SOURCES)
    record: dict[str, object] = {"status": "running"}
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        contexts = {i: PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in (14, 15, 16)}
        inputs = {str(path): digest(path) for path in contexts.values()}
        run(["bash", str(here / "build_windows_camera_projection_original_check.sh"), str(out)], out / "build.log")
        exe = out / "camera-projection-original-check.exe"
        logs: dict[str, list[str]] = {}
        for i, context in contexts.items():
            traces = []
            for repeat in range(2):
                log = out / f"original-{i}-{repeat}.log"
                text = run([str(exe), windows(context), windows(BASELINES["original_dll"][0])], log)
                if text.replace("\r\n", "\n").strip() != EXPECTED:
                    raise RuntimeError(f"Original camera projection comparison failed in context {i}")
                traces.append(str(digest(log)))
            if traces[0] != traces[1]:
                raise RuntimeError("Fresh original camera projection traces differ")
            logs[str(i)] = traces
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
            executable_sha256=digest(exe),
            cases_per_context=1203,
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print("Original camera projection builders pass; private report " + str(report))


if __name__ == "__main__":
    main()
