#!/usr/bin/env python3
"""Check locomotion selection and stateful predicates against the original, including every planted fault."""

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
    "locomotion_predicates.c",
    "locomotion_predicates.h",
    "locomotion_predicates_original_check.c",
    "build_windows_locomotion_predicates_original_check.sh",
    "verify_locomotion_predicates.py",
    "locomotion_predicates_mutants.json",
    "locomotion_predicates.md",
    "gekko_float.h",
    "draw_engine.h",
    "verify_player_startup.py",
    "../menus/verify_controller_checks.py",
)
BOUNDARY = (
    "800150E8 and its eleven dispatch entries, 8006510C and its controller/team/status predicates, "
    "80014644/80061158, 800146CC/8001454C/8004F214, 80014750/80014898/8005335C, "
    "801EE1C0(0) and all three 80188628 generators. Original predicates and helpers execute normally. "
    "Only 80177EF4 and gameplay event services 80076C48/80076E18 return at their original entry, "
    "recording arguments. All RAM outside 80F38000..80F47000 and ordered service calls compared. "
    "Downstream clip manager/callback execution and gameplay event service internals are not modeled."
)
SUMMARY = re.compile(
    r"Original locomotion predicates: actors=14 cases=401 select=5614 gameplay=401 suppression=401; "
    r"all RAM outside oracle stack and service arguments match; digest=([0-9a-f]{8})\.\n"
    r"Coverage: gameplay=(\d+)/(\d+) suppression=(\d+)/(\d+) requests=(\d+) effects=(\d+)/(\d+) "
    r"rng=(\d+)/(\d+)/(\d+) predicate_calls=(\d+) random_calls=(\d+) turn_calls=(\d+); "
    r"selectors((?: \d+){11})\."
)


def validate(text: str) -> None:
    match = SUMMARY.fullmatch(text.replace("\r\n", "\n").strip())
    if not match or any(int(match[i]) == 0 for i in range(2, 15)):
        raise RuntimeError("Incomplete original locomotion comparison/coverage record: " + text)
    if any(int(count) == 0 for count in match[15].split()):
        raise RuntimeError("A movement selector was not exercised")
    if sum(map(int, match[15].split())) != 5614:
        raise RuntimeError("Wrong movement selector comparison count")
    if int(match[2]) + int(match[3]) != 401 or int(match[4]) + int(match[5]) != 401:
        raise RuntimeError("Wrong independent predicate comparison count")


def build(here: Path, out: Path, log: Path) -> Path:
    # Windows can retain an old executable mapping. Every link uses a fresh
    # destination; never fall back to a previously built executable.
    directory = Path(tempfile.mkdtemp(prefix="build-", dir=out))
    run(["bash", str(here / "build_windows_locomotion_predicates_original_check.sh"), str(directory)], log)
    return directory / "locomotion-predicates-original-check.exe"


def mutants(
    here: Path, out: Path, sources: dict[str, str], contexts: dict[int, Path], build_only: bool
) -> list[dict[str, object]]:
    results: list[dict[str, object]] = []
    edits = json.loads((here / "locomotion_predicates_mutants.json").read_text())
    if not isinstance(edits, list) or not 5 <= len(edits) <= 10:
        raise RuntimeError("Expected five to ten independent one-line mutations")
    names = [edit["name"] for edit in edits]
    if len(names) != len(set(names)) or any(not re.fullmatch(r"[a-z0-9_-]+", name) for name in names):
        raise RuntimeError("Mutation names must be unique safe path components")
    for edit in edits:
        with tempfile.TemporaryDirectory(prefix=".locomotion-mutant-", dir=here) as directory:
            scratch = Path(directory)
            for entry in sources:
                if entry.startswith("ModernGekko/"):
                    continue
                target = scratch / entry
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(here.parent / entry, target)
            if edit["file"] != "projects/nfl-street/reconstruction/native_assets/locomotion_predicates.c":
                raise RuntimeError("Unexpected mutation target")
            target = scratch / "native_assets/locomotion_predicates.c"
            text = target.read_text()
            if "\n" in edit["old"] or "\n" in edit["new"] or text.count(edit["old"]) != 1:
                raise RuntimeError("Mutation must identify exactly one single-line edit: " + edit["name"])
            target.write_text(text.replace(edit["old"], edit["new"]))
            destination = out / "mutants" / edit["name"]
            destination.mkdir(parents=True, exist_ok=True)
            exe = build(scratch / "native_assets", destination, destination / "build.log")
            result: dict[str, object] = {
                "name": edit["name"],
                "status": "compiled",
                "executable_path": str(exe),
                "executable_sha256": digest(exe),
            }
            if not build_only:
                # Any one real RAM/argument/result mismatch must catch the fault.
                # Crashes, build errors, rejection, lost coverage or timeouts do
                # not count. Try the other saved contexts only if it survived.
                caught = False
                for context, path in contexts.items():
                    command = [str(exe), windows(path), windows(BASELINES["original_dll"][0])]
                    proc = subprocess.run(command, capture_output=True, text=True, check=False, timeout=900)
                    log = destination / f"original-{context}.log"
                    log.write_text(proc.stdout + proc.stderr)
                    if proc.returncode == 7 and "comparison mismatch mode=" in proc.stderr:
                        result.update(status="caught", context=context, returncode=7, log_sha256=digest(log))
                        caught = True
                        break
                    if proc.returncode != 0:
                        raise RuntimeError(
                            f"Mutant {edit['name']} failed without a comparison mismatch: {proc.returncode}"
                        )
                    validate(proc.stdout)
                if not caught:
                    raise RuntimeError(f"Mutant {edit['name']} survived all three saved contexts")
            results.append(result)
    return results


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--build-only", action="store_true", help="Compile base and mutants without running Windows code"
    )
    args = parser.parse_args()
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / "research-temp/nflstreet-locopred-20261015")
    out.mkdir(parents=True, exist_ok=True)
    report = out / ("compilation.json" if args.build_only else "verification.json")
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
        if not args.build_only:
            for i, context in contexts.items():
                traces = []
                for repeat in range(2):
                    log = out / f"original-{i}-{repeat}.log"
                    text = run([str(exe), windows(context), windows(BASELINES["original_dll"][0])], log)
                    validate(text)
                    traces.append(str(digest(log)))
                    summaries[str(i)] = text.replace("\r\n", "\n").strip()
                if traces[0] != traces[1]:
                    raise RuntimeError("Fresh original locomotion traces differ")
                logs[str(i)] = traces
        mutation_results = mutants(here, out, sources, contexts, args.build_only)
        if before != baseline_hashes() or any(digest(Path(path)) != sha for path, sha in inputs.items()):
            raise RuntimeError("Protected originals or private inputs changed")
        if sources != source_hashes(root, entries):
            raise RuntimeError("Source changed during verification")
        record.update(
            status="compiled" if args.build_only else "passed",
            baseline=before,
            input_sha256=inputs,
            source_sha256=sources,
            original_log_sha256=logs,
            summaries=summaries,
            executable_sha256=digest(exe),
            selector_cases_per_actor=401,
            independent_cases_per_predicate=401,
            actors=14,
            fresh_processes_per_context=0 if args.build_only else 2,
            mutations=mutation_results,
            mutation_status="compiled" if args.build_only else "passed",
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    if args.build_only:
        print(
            f"Locomotion base and {len(mutation_results)} mutants compile; Windows execution not run; report {report}"
        )
    else:
        print(
            "Original locomotion predicates pass in two fresh runs per context 14/15/16; private report " + str(report)
        )
        print(f"Locomotion predicate mutants: {len(mutation_results)}/{len(mutation_results)} caught.")


if __name__ == "__main__":
    main()
