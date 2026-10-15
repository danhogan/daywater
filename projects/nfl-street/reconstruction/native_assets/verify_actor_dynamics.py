#!/usr/bin/env python3
"""Original-module gate for hand/head dynamics and independently compiled mutants.

This runs Windows executables. It is intended for the owner to run after
compilation; a compiled binary alone never constitutes oracle evidence.
"""

import argparse
import json
import re
import subprocess
from pathlib import Path

from build_actor_dynamics_mutants import OUTPUT, compile_mutant, definitions
from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, run, windows

HERE = Path(__file__).resolve().parent
SOURCES = (
    "actor_dynamics.c",
    "actor_dynamics.h",
    "actor_dynamics_math.c",
    "actor_dynamics_math.h",
    "actor_dynamics_check.h",
    "actor_dynamics_original_check.c",
    "actor_dynamics_hand_original_check.c",
    "actor_dynamics_head_original_check.c",
    "actor_dynamics_mutants.json",
    "build_windows_actor_dynamics_original_check.sh",
    "build_windows_actor_dynamics_hand_original_check.sh",
    "build_actor_dynamics_mutants.py",
    "verify_actor_dynamics.py",
    "actor_dynamics.md",
    "matrix_stack.c",
    "matrix_stack.h",
    "draw_engine.c",
    "draw_engine.h",
    "player_draw.h",
    "scenery_draw.h",
    "marks_draw.h",
    "check_fixture.h",
    "venue_fixture.h",
)
SUMMARY = re.compile(
    r"Original actor dynamics: 67424 hand and 33712 head updates; 14 actors x 301 cases x 2 enabled flags "
    r"x 4 consecutive updates; 101136 all-RAM comparisons outside the oracle CPU stack; digest ([0-9a-f]{8})\."
)


def sources() -> dict[str, str]:
    return {name: digest(HERE / name) for name in SOURCES}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--no-build", action="store_true", help="Use the already compiled baseline and mutant binaries")
    args = parser.parse_args()
    OUTPUT.mkdir(parents=True, exist_ok=True)
    report = OUTPUT / "verification.json"
    record: dict[str, object] = {"status": "running", "cases": 301, "updates": 4, "actors": 14, "enabled_flags": [0, 1]}
    report.write_text(json.dumps(record, indent=2) + "\n")
    try:
        before = baseline_hashes()
        source_hashes = sources()
        contexts = {i: PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in (14, 15, 16)}
        inputs = {str(path): digest(path) for path in contexts.values()}
        if not args.no_build:
            run(
                ["bash", str(HERE / "build_windows_actor_dynamics_original_check.sh"), str(OUTPUT)],
                OUTPUT / "gate-build.log",
            )
        exe = OUTPUT / "actor-dynamics-original-check.exe"
        executable = digest(exe)
        logs: dict[str, list[str]] = {}
        summaries: dict[str, str] = {}
        dll = windows(BASELINES["original_dll"][0])
        for index, context in contexts.items():
            traces = []
            for repeat in range(2):
                log = OUTPUT / f"original-{index}-{repeat}.log"
                output = run([str(exe), windows(context), dll], log)
                match = SUMMARY.search(output)
                if match is None or len(SUMMARY.findall(output)) != 1:
                    raise RuntimeError(f"Context {index} did not complete the required comparison counts")
                traces.append(digest(log))
                summaries[str(index)] = match[0]
            if traces[0] != traces[1]:
                raise RuntimeError(f"Context {index} is nondeterministic across fresh processes")
            logs[str(index)] = traces
            print(f"Context {index}: {summaries[str(index)]}", flush=True)
        if digest(exe) != executable:
            raise RuntimeError("Baseline executable changed during verification")
        mutants: dict[str, object] = {}
        record["mutants"] = mutants
        for mutant in definitions():
            mutant_exe = OUTPUT / "mutants" / mutant["name"] / "actor-dynamics-original-check.exe"
            if not args.no_build:
                mutant_exe = compile_mutant(mutant)
            changed = mutant_exe.parent / mutant["file"]
            expected_source = (HERE / mutant["file"]).read_text().replace(mutant["old"], mutant["new"], 1)
            if changed.read_text() != expected_source:
                raise RuntimeError(f"Mutant {mutant['name']} does not contain exactly the requested edit")
            log = mutant_exe.parent / "original-14.log"
            with log.open("w") as stream:
                result = subprocess.run(
                    [str(mutant_exe), windows(contexts[14]), dll, "17", "4"],
                    stdout=stream,
                    stderr=subprocess.STDOUT,
                    check=False,
                )
            # A crash, load failure, native rejection or timeout is NOT a kill.
            # Require the actual byte comparator to detect unequal results.
            if result.returncode != 6 or "RAM differs " not in log.read_text():
                raise RuntimeError(f"Mutant {mutant['name']} was not caught by the RAM comparator; see {log}")
            mutants[mutant["name"]] = {"status": "caught", "log_sha256": digest(log), "exe_sha256": digest(mutant_exe)}
            print(f"Mutant caught: {mutant['name']}", flush=True)
        if before != baseline_hashes() or inputs != {str(path): digest(path) for path in contexts.values()}:
            raise RuntimeError("Protected originals or private inputs changed")
        if source_hashes != sources():
            raise RuntimeError("Source changed during verification")
        record.update(
            status="passed",
            baseline=before,
            source_sha256=source_hashes,
            input_sha256=inputs,
            executable_sha256=executable,
            original_logs=logs,
            summaries=summaries,
            mutants=mutants,
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print(f"Actor dynamics pass; 3 contexts x 2 processes, 12 mutants caught; private report {report}")


if __name__ == "__main__":
    main()
