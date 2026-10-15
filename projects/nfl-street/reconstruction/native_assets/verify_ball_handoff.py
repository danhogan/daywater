#!/usr/bin/env python3
"""Check ownership services against original MEM1, then compile and catch every declared mutant."""

import argparse
import json
import re
import shutil
import subprocess
from pathlib import Path

from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

SOURCES = (
    "ball_handoff.c",
    "ball_handoff.h",
    "ball_handoff_original_check.c",
    "ball_handoff_mutants.json",
    "ball_handoff.md",
    "../ball/ball_ownership.c",
    "../ball/ball_lifecycle.c",
    "../ball/ball_attachment.c",
    "../ball/ball_orientation.c",
    "matrix_stack.c",
    "draw_engine.c",
    "gekko_quaternion/quaternion_kernels.c",
    "build_windows_ball_handoff_original_check.sh",
    "verify_ball_handoff.py",
)
BOUNDARY = (
    "No hooks: full 800BE2B4 and 800815B4; bounded 800C25FC, 800BE204/800BE264 and 8007BEE8. "
    "All MEM1 except reserved oracle stack compared after each operation. Existing original-layout "
    "actor/model/ball inputs. Event consumers only: natural catch, handoff/pitch, fumble and pickup "
    "eligibility, full event/scoring services and gameplay scheduling remain unmodeled."
)
SUMMARY = re.compile(
    r"Original ball handoff: 401 cases x 4 updates; 7484 all-RAM comparisons; 1068 bounded events; "
    r"assignment, both releases, 800815B4 and history match; no hooks; digest [0-9a-f]{8}\."
)


def build(here: Path, out: Path) -> Path:
    out.mkdir(parents=True, exist_ok=True)
    run(["bash", str(here / "build_windows_ball_handoff_original_check.sh"), str(out)], out / "build.log")
    return out / "ball-handoff-original-check.exe"


def mutants(here: Path, out: Path, sources: dict[str, str], context: Path, build_only: bool) -> list[dict[str, object]]:
    root = here.parent
    edits = json.loads((here / "ball_handoff_mutants.json").read_text())
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
        if build_only:
            results.append({"name": edit["name"], "status": "compiled_only", "executable_sha256": digest(exe)})
            continue
        log = destination / "original-14.log"
        with log.open("w") as stream:
            result = subprocess.run(
                [str(exe), windows(context), windows(BASELINES["original_dll"][0])],
                stdout=stream,
                stderr=subprocess.STDOUT,
                check=False,
                timeout=300,
            )
        text = log.read_text()
        # Rejection, crash, loader failure and build failure never count.
        if result.returncode != 6 or "RAM differs case " not in text:
            raise RuntimeError(f"Mutant {edit['name']} was not caught by a RAM comparison: {text}")
        results.append({"name": edit["name"], "caught": True, "log_sha256": digest(log)})
        print(f"mutant {edit['name']}: caught by all-RAM comparison", flush=True)
    if len(results) < 5:
        raise RuntimeError("Too few mutants")
    return results


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--build-only", action="store_true", help="Compile original check and all mutants without execution"
    )
    args = parser.parse_args()
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / "research-temp/nflstreet-ballplay-20261015")
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
        if not args.build_only:
            for i, context in contexts.items():
                traces = []
                for repeat in range(2):
                    log = out / f"original-{i}-{repeat}.log"
                    text = run([str(exe), windows(context), windows(BASELINES["original_dll"][0])], log)
                    text = text.replace("\r\n", "\n").strip()
                    if not SUMMARY.fullmatch(text):
                        raise RuntimeError(f"Original ownership comparison failed in context {i}: {text}")
                    traces.append(digest(log))
                    print(f"context {i}, fresh run {repeat + 1}: {text}", flush=True)
                if traces[0] != traces[1]:
                    raise RuntimeError("Fresh original ownership traces differ")
                logs[str(i)] = traces
        record["mutants"] = mutants(here, out, sources, contexts[14], args.build_only)
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
            comparisons_per_context_per_run=7484,
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    if args.build_only:
        print("Ball handoff and 10 mutants compiled; original execution and mutation results pending.")
    else:
        print("Ball handoff mutants: 10/10 caught.")
        print("Original ball handoff services pass; private report " + str(report))


if __name__ == "__main__":
    main()
