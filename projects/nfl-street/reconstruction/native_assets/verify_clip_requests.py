#!/usr/bin/env python3
"""Compare animation clip requests with the original module; optionally check the planted mutants."""

import argparse
import json
import re
import shutil
import subprocess
import tempfile
from pathlib import Path

from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

SOURCES = (
    "clip_requests.c",
    "clip_requests.h",
    "clip_requests_original_check.c",
    "build_windows_clip_requests_original_check.sh",
    "verify_clip_requests.py",
    "clip_requests_mutants.json",
    "clip_requests.md",
    "gekko_float.h",
    "draw_engine.h",
)
BOUNDARY = (
    "8017413C, 80177EF4, 80178780 and direct start 80173E78, including bank/group catalogs, allocation, "
    "retirement, transitions, blend initialization and program initialization/scans. Native and original use "
    "identical prepared MEM1; all RAM outside 80F38000..80F47000 and callback argument/operand traces compared. "
    "Only external gameplay group/event callbacks return at entry; the fixture event provider consumes operands "
    "through original 801777A0 in the oracle. Movement state handlers and actual gameplay callbacks "
    "are not reconstructed."
)
SUMMARY = re.compile(
    r"Original clip requests: actors=14 cases=301 replace=4214 group=4214 transition=4214 start=4214; "
    r"all RAM outside oracle stack and callbacks match; rejected=(\d+) events=(\d+) scans=(\d+) "
    r"groups=(\d+) operands=(\d+); digest=([0-9a-f]{8})\.\n"
    r"Coverage: types=(\d+)/(\d+)/(\d+)/(\d+) fallback=(\d+) synthetic=(\d+); dispatch((?: \d+){8})\."
)


def validate(text: str) -> None:
    match = SUMMARY.fullmatch(text.replace("\r\n", "\n").strip())
    if not match or any(int(match[i]) == 0 for i in (*range(1, 6), *range(7, 13))):
        raise RuntimeError("Clip request comparison lacks its complete success/coverage record: " + text)


def build(here: Path, out: Path, log: Path) -> Path:
    # Windows may still have an earlier check executable open. Never link over
    # it: each build gets a new retained directory under the private output.
    directory = Path(tempfile.mkdtemp(prefix="build-", dir=out))
    run(["bash", str(here / "build_windows_clip_requests_original_check.sh"), str(directory)], log)
    return directory / "clip-requests-original-check.exe"


def mutants(here: Path, out: Path, sources: dict[str, str], context: Path) -> list[dict[str, object]]:
    results: list[dict[str, object]] = []
    edits = json.loads((here / "clip_requests_mutants.json").read_text())
    for edit in edits:
        # Keep scratch repository files inside this worktree. Binaries and logs stay on W:.
        with tempfile.TemporaryDirectory(prefix=".clip-request-mutant-", dir=here) as directory:
            scratch = Path(directory)
            for entry in sources:
                if entry.startswith("ModernGekko/"):
                    continue
                target = scratch / entry
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(here.parent / entry, target)
            if edit["file"] != "projects/nfl-street/reconstruction/native_assets/clip_requests.c":
                raise RuntimeError("Unexpected mutation target")
            target = scratch / "native_assets/clip_requests.c"
            text = target.read_text()
            if "\n" in edit["old"] or "\n" in edit["new"] or text.count(edit["old"]) != 1:
                raise RuntimeError("Mutation must identify exactly one single-line edit: " + edit["name"])
            target.write_text(text.replace(edit["old"], edit["new"]))
            destination = out / "mutants" / edit["name"]
            destination.mkdir(parents=True, exist_ok=True)
            exe = build(scratch / "native_assets", destination, destination / "build.log")
            command = [str(exe), windows(context), windows(BASELINES["original_dll"][0])]
            proc = subprocess.run(command, capture_output=True, text=True, check=False)
            log = destination / "original-14.log"
            log.write_text(proc.stdout + proc.stderr)
            # Compilation, access violations, unsupported fixtures, timeouts or lost
            # coverage are failures of mutation testing, never a caught mutation.
            if proc.returncode != 7 or "mismatch mode=" not in proc.stderr:
                raise RuntimeError(
                    f"Mutant {edit['name']} was not caught by a native/original mismatch: {proc.returncode}"
                )
            results.append(
                {
                    "name": edit["name"],
                    "status": "caught",
                    "returncode": proc.returncode,
                    "log_sha256": digest(log),
                    "executable_path": str(exe),
                }
            )
    return results


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mutants", action="store_true", help="Also compile/run every deliberate one-line fault")
    args = parser.parse_args()
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / "research-temp/nflstreet-clips-20261015")
    out.mkdir(parents=True, exist_ok=True)
    report = out / "verification.json"
    entries = tuple("native_assets/" + name for name in SOURCES)
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY}
    report.write_text(json.dumps(record, indent=2) + "\n")
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        contexts = {i: private(PRIVATE / f"research-temp/position-neutral-{i}.mem1") for i in (14, 15, 16)}
        inputs = {str(path): digest(path) for path in contexts.values()}
        exe = build(here, out, out / "build.log")
        record["executable_path"] = str(exe)
        logs: dict[str, list[str]] = {}
        summaries: dict[str, str] = {}
        for i, context in contexts.items():
            traces = []
            for repeat in range(2):
                log = out / f"original-{i}-{repeat}.log"
                text = run([str(exe), windows(context), windows(BASELINES["original_dll"][0])], log)
                validate(text)
                traces.append(str(digest(log)))
                summaries[str(i)] = text.replace("\r\n", "\n").strip()
            if traces[0] != traces[1]:
                raise RuntimeError("Fresh original clip request traces differ")
            logs[str(i)] = traces
        mutation_results = mutants(here, out, sources, contexts[14]) if args.mutants else []
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
            summaries=summaries,
            executable_sha256=digest(exe),
            cases_per_actor_per_routine=301,
            actors=14,
            fresh_processes_per_context=2,
            mutations=mutation_results,
            mutation_status="passed" if args.mutants else "not_run",
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print("Original clip requests pass; private report " + str(report))
    if args.mutants:
        print(f"Clip request mutants: {len(mutation_results)}/{len(mutation_results)} caught.")


if __name__ == "__main__":
    main()
