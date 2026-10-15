#!/usr/bin/env python3
"""Integration gate for field-camera entry 14; runs Windows probes and rejects every planted mutant."""

import json
import os
import re
import subprocess
from pathlib import Path

from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

SOURCES = (
    "field_overlay.c",
    "field_overlay.h",
    "field_overlay_oracle.h",
    "field_overlay_original_check.c",
    "field_overlay_mutants.json",
    "build_windows_field_overlay_original_check.sh",
    "verify_field_overlay.py",
    "field_overlay.md",
    "cloud_draw.h",
    "scenery_draw.c",
    "scenery_draw.h",
    "scenery_shared.h",
    "marks_draw.c",
    "marks_draw.h",
    "matrix_stack.c",
    "matrix_stack.h",
    "draw_engine.c",
    "draw_engine.h",
    "player_draw.h",
    "gekko_float.h",
    "gx_oracle.h",
    "check_fixture.h",
    "venue_fixture.h",
)
SUMMARY = re.compile(
    r"Original field overlay: 601 runs of 800E5CA0; saved forced-on (\d+) calls; "
    r"(\d+) GX calls and pipe writes \((\d+) writes, (\d+) quads, (\d+) depth reads, (\d+) updates\); "
    r"all RAM matches; digest ([0-9a-f]{8})\."
)
BOUNDARY = (
    "800E5CA0 -> 800D1118(0): projection and matrix/fog bracketing, active-record visibility and textured quads. "
    "All original game routines execute; SDK setters recorded and compared, GXGetProjectionv and C_MTXOrtho "
    "execute, GXPeekZ receives a supplied hardware response. All RAM outside the oracle stack compared. "
    "Saved state with enable forced on plus 600 controlled edits per context. No live GPU/pixel claim; "
    "clipped mode-1 output locals and configuration-query side effects are outside the modeled contract."
)


def main() -> None:
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / "research-temp/nflstreet-frame14-20261015")
    out.mkdir(parents=True, exist_ok=True)
    report = out / "verification.json"
    entries = tuple("native_assets/" + name for name in SOURCES)
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY}
    report.write_text(json.dumps(record, indent=2) + "\n")
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        contexts = {i: PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in (14, 15, 16)}
        inputs = {str(path): digest(path) for path in contexts.values()}
        build = here / "build_windows_field_overlay_original_check.sh"
        run(["bash", str(build), str(out)], out / "build.log")
        exe = out / "field-overlay-original-check.exe"
        logs: dict[str, list[str]] = {}
        summaries = {}
        for i, context in contexts.items():
            hashes = []
            for repeat in range(2):
                log = out / f"original-{i}-{repeat}.log"
                text = run([str(exe), windows(context), windows(BASELINES["original_dll"][0])], log)
                match = SUMMARY.fullmatch(text.replace("\r\n", "\n").strip())
                if not match or any(int(match[j]) == 0 for j in range(1, 7)):
                    raise RuntimeError(f"Original field overlay or branch coverage failed in context {i}")
                summaries[str(i)] = text.strip()
                hashes.append(digest(log))
            if hashes[0] != hashes[1]:
                raise RuntimeError(f"Fresh field overlay runs differ in context {i}")
            logs[str(i)] = hashes
            print(summaries[str(i)], flush=True)
        mutants = json.loads((here / "field_overlay_mutants.json").read_text())
        caught = []
        for mutant in mutants:
            name = mutant["name"]
            if not re.fullmatch(r"[a-z0-9_-]+", name) or mutant["file"] != "field_overlay.c":
                raise RuntimeError("Unexpected mutant path/name")
            source = (here / mutant["file"]).read_text()
            if source.count(mutant["old"]) != 1 or "\n" in mutant["old"] or "\n" in mutant["new"]:
                raise RuntimeError(f"Mutant {name} is not a unique one-line edit")
            scratch = private(out / "mutants" / name)
            scratch.mkdir(parents=True, exist_ok=True)
            changed = scratch / mutant["file"]
            changed.write_text(source.replace(mutant["old"], mutant["new"], 1))
            with (scratch / "build.log").open("w") as output:
                subprocess.run(
                    ["bash", str(build), str(scratch)],
                    env={**os.environ, "FIELD_OVERLAY_SOURCE": str(changed)},
                    stdout=output,
                    stderr=subprocess.STDOUT,
                    check=True,
                )
            detections = []
            for i, context in contexts.items():
                log = scratch / f"original-{i}.log"
                with log.open("w") as output:
                    result = subprocess.run(
                        [str(scratch / exe.name), windows(context), windows(BASELINES["original_dll"][0])],
                        stdout=output,
                        stderr=subprocess.STDOUT,
                        check=False,
                    )
                # A crash, fixture rejection, or missing summary is not proof of detection.
                text = log.read_text()
                if result.returncode not in (6, 7) or not re.search(
                    r"GX (?:call|pipe write)|extended GX call|RAM differs", text
                ):
                    raise RuntimeError(f"Mutant {name} was not caught by a comparison in context {i}")
                detections.append({"context": i, "exit": result.returncode, "log_sha256": digest(log)})
            caught.append({"name": name, "source_sha256": digest(changed), "detections": detections})
            print(f"Mutant caught: {name} (contexts 14/15/16)", flush=True)
        if before != baseline_hashes() or any(digest(Path(path)) != sha for path, sha in inputs.items()):
            raise RuntimeError("Protected originals or inputs changed")
        if sources != source_hashes(root, entries):
            raise RuntimeError("Sources changed during verification")
        record.update(
            status="passed",
            baseline=before,
            input_sha256=inputs,
            source_sha256=sources,
            original_log_sha256=logs,
            summaries=summaries,
            executable_sha256=digest(exe),
            cases_per_context=601,
            fresh_runs_per_context=2,
            mutants=caught,
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print(f"Original field overlay passes; {len(caught)} mutants caught; private report {report}")


if __name__ == "__main__":
    main()
