#!/usr/bin/env python3
"""Build the camera projection oracle check and run it on all saved contexts."""

import json
import re
from pathlib import Path

from verify_controller_checks import BASELINES, PRIVATE, digest, private, run, windows

EXPECTED = re.compile(r"Camera projection builders: 1203 node cases .* match all RAM\.")
SOURCES = (
    "camera_projection.c",
    "camera_projection.h",
    "camera_projection_original_check.c",
    "build_windows_camera_projection_original_check.sh",
    "verify_camera_projection.py",
)


def main() -> None:
    here = Path(__file__).resolve().parent
    out = private(PRIVATE / "research-temp/nflstreet-camera-projection-20261014")
    out.mkdir(parents=True, exist_ok=True)
    exe = out / "camera-projection-original-check.exe"
    report = out / "verification.json"
    sources = {name: digest(here / name) for name in SOURCES}
    traces: dict[str, str] = {}
    try:
        run(["bash", str(here / "build_windows_camera_projection_original_check.sh"), str(out)], out / "build.log")
        for context in (14, 15, 16):
            log = out / f"original-{context}.log"
            output = run(
                [
                    str(exe),
                    windows(PRIVATE / f"research-temp/position-neutral-{context}.mem1"),
                    windows(BASELINES["original_dll"][0]),
                ],
                log,
            )
            normalized = output.replace("\r\n", "\n").strip()
            if not EXPECTED.fullmatch(normalized):
                raise RuntimeError(f"Camera projection oracle comparison failed in context {context}: {normalized}")
            traces[str(context)] = str(digest(log))
        record = {"status": "passed", "source_sha256": sources, "trace_sha256": traces, "cases_per_context": 1203}
    except Exception as exc:
        record = {"status": "failed", "source_sha256": sources, "error": str(exc)}
        report.write_text(json.dumps(record, indent=2) + "\n")
        raise
    report.write_text(json.dumps(record, indent=2) + "\n")
    print("Original camera projection builders pass; private report " + str(report))


if __name__ == "__main__":
    main()
