#!/usr/bin/env python3
"""Check ball updates against the original module, including every planted mutant."""

import argparse
import json
import re
import shutil
import subprocess
from pathlib import Path

from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

SOURCES = (
    "ball_update.c",
    "ball_update.h",
    "ball_update_original_check.c",
    "ball_update_mutants.json",
    "ball_update.md",
    "../ball/ball_frame.c",
    "ball_update_motion.c",
    "../ball/ball_attachment.c",
    "../ball/ball_orientation.c",
    "../ball/ball_state.c",
    "matrix_stack.c",
    "draw_engine.c",
    "gekko_quaternion/quaternion_kernels.c",
    "build_windows_ball_update_original_check.sh",
    "verify_ball_update.py",
)
BOUNDARY = (
    "800C0FE4 attached and bounded detached no-spin/no-wind branch; real association resolution, "
    "matrix scratch and original spatial lookup, no hooks. 800C09D8 pose publication and 801A3008 "
    "rotation stores checked separately. Matrix translation is an explicit host connection. "
    "State dispatcher, contact/bounce, force producers, natural ownership and other rotation modes remain unmodeled."
)
SUMMARY = re.compile(
    r"Original ball update: 401 cases x 4 updates; 964 attached, 640 detached; "
    r"800C0FE4 \+ 800C09D8 \+ 801A3008 match all RAM after each phase; no hooks; digest [0-9a-f]{8}\."
)


def build(here: Path, out: Path) -> Path:
    out.mkdir(parents=True, exist_ok=True)
    run(["bash", str(here / "build_windows_ball_update_original_check.sh"), str(out)], out / "build.log")
    return out / "ball-update-original-check.exe"


def mutants(here: Path, out: Path, sources: dict[str, str], context: Path) -> list[dict[str, object]]:
    root = here.parent
    edits = json.loads((here / "ball_update_mutants.json").read_text())
    results = []
    names = set()
    for edit in edits:
        if set(edit) != {"name", "file", "old", "new"} or "\n" in edit["old"] or "\n" in edit["new"]:
            raise RuntimeError("Expected a one-line {name,file,old,new} mutant")
        if edit["name"] in names or not re.fullmatch(r"[a-z0-9-]+", edit["name"]):
            raise RuntimeError("Invalid or duplicate mutant name")
        names.add(edit["name"])
        destination = out / "mutants" / edit["name"]
        tree = destination / "source"
        if tree.exists():
            shutil.rmtree(tree)
        for name in sources:
            if name.startswith("ModernGekko/"):
                continue
            target = tree / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(root / name, target)
        target = (tree / "native_assets" / edit["file"]).resolve()
        if not target.is_relative_to(tree.resolve()) or not target.is_file():
            raise RuntimeError("Mutant must edit a declared local source")
        text = target.read_text()
        if text.count(edit["old"]) != 1 or edit["old"] == edit["new"]:
            raise RuntimeError(f"Nonunique or ineffective mutant {edit['name']}")
        target.write_text(text.replace(edit["old"], edit["new"]))
        exe = build(tree / "native_assets", destination)
        log = destination / "original-14.log"
        with log.open("w") as stream:
            result = subprocess.run(
                [str(exe), windows(context), windows(BASELINES["original_dll"][0])],
                stdout=stream,
                stderr=subprocess.STDOUT,
                check=False,
                timeout=300,
            )
        # A loader failure, crash, timeout or compiler error is never a caught mutant.
        text = log.read_text()
        if result.returncode not in (6, 7, 8) or "RAM differs case " not in text:
            raise RuntimeError(f"Mutant {edit['name']} was not caught by a RAM comparison: {text}")
        results.append({"name": edit["name"], "caught": True, "log_sha256": digest(log)})
    if len(results) < 5:
        raise RuntimeError("Too few mutants")
    return results


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-only", action="store_true", help="Compile only; never reports verification passed")
    args = parser.parse_args()
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / "research-temp/nflstreet-ball-20261015")
    out.mkdir(parents=True, exist_ok=True)
    report = out / "verification.json"
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY}
    report.write_text(json.dumps(record, indent=2) + "\n")
    entries = tuple("native_assets/" + name for name in SOURCES)
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        contexts = {i: PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in (14, 15, 16)}
        inputs = {str(path): digest(path) for path in contexts.values()}
        exe = build(here, out)
        logs = {}
        mutation_results: list[dict[str, object]] = []
        if not args.build_only:
            for i, context in contexts.items():
                traces = []
                for repeat in range(2):
                    log = out / f"original-{i}-{repeat}.log"
                    text = run([str(exe), windows(context), windows(BASELINES["original_dll"][0])], log)
                    text = text.replace("\r\n", "\n").strip()
                    if not SUMMARY.fullmatch(text):
                        raise RuntimeError(f"Original ball comparison failed in context {i}: {text}")
                    traces.append(digest(log))
                    print(f"context {i}, fresh run {repeat + 1}: {text}", flush=True)
                if traces[0] != traces[1]:
                    raise RuntimeError("Fresh original ball traces differ")
                logs[str(i)] = traces
            mutation_results = mutants(here, out, sources, contexts[14])
            record["mutants"] = mutation_results
        if before != baseline_hashes() or any(digest(Path(path)) != sha for path, sha in inputs.items()):
            raise RuntimeError("Protected originals or private inputs changed")
        if sources != source_hashes(root, entries):
            raise RuntimeError("Source changed during verification")
        record.update(
            status="compiled_only" if args.build_only else "passed",
            baseline=before,
            input_sha256=inputs,
            source_sha256=sources,
            original_log_sha256=logs,
            executable_sha256=digest(exe),
            cases_per_context=401,
            consecutive_updates=4,
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    if args.build_only:
        print("Ball update compiled; original execution and mutants pending.")
    else:
        print(f"Ball update mutants: {len(mutation_results)}/{len(mutation_results)} caught.")
        print("Original ball updates pass; private report " + str(report))


if __name__ == "__main__":
    main()
