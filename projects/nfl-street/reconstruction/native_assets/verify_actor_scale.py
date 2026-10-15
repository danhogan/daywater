#!/usr/bin/env python3
"""Build/check actor scaling against the original; --build-only never executes Windows code."""

import argparse
import json
import os
import re
import subprocess
import tempfile
from pathlib import Path

from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows

SOURCES = (
    "actor_scale.c",
    "actor_scale.h",
    "actor_scale_original_check.c",
    "actor_scale_mutants.json",
    "build_windows_actor_scale_original_check.sh",
    "verify_actor_scale.py",
    "actor_scale.md",
    "draw_engine.c",
    "draw_engine.h",
    "model_view.c",
    "model_view.h",
    "matrix_stack.c",
    "matrix_stack.h",
    "player_draw.h",
    "scenery_draw.h",
    "marks_draw.h",
    "check_fixture.h",
    "venue_fixture.h",
)
SUMMARY = re.compile(
    r"Original actor scale: 16842 batches over 14 actors x 401 cases x 3 modes \(mark/vector/paired\); "
    r"enabled (\d+) disabled (\d+) owner-hit (\d+) owner-miss (\d+) wide-index (\d+); "
    r"all RAM outside oracle stack and every produced vector match; digest [0-9a-f]{8}\."
)
BOUNDARY = (
    "Unhooked 800DFBA8/800DFB34 and their sole callee 800DF1EC, mark-only/vector-only/paired, "
    "all 14 saved visual actors in 401 cases per context, all RAM outside oracle stack and produced vectors. "
    "Finite binary32 scale inputs, unsigned full-byte indices, relocated supplied scale objects. "
    "No scale-object creation/update or full draw/palette integration is claimed."
)


def build(here: Path, out: Path, source: Path | None = None) -> Path:
    out.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    if source is not None:
        env["DAYWATER_ACTOR_SCALE_SOURCE"] = str(source)
    else:
        env.pop("DAYWATER_ACTOR_SCALE_SOURCE", None)
    with (out / "build.log").open("w") as log:
        subprocess.run(
            ["bash", str(here / "build_windows_actor_scale_original_check.sh"), str(out)],
            stdout=log,
            stderr=subprocess.STDOUT,
            env=env,
            check=True,
        )
    return out / "actor-scale-original-check.exe"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-only", action="store_true")
    parser.add_argument("--mutants", action="store_true", help="Also build/check each exact one-line mutant.")
    args = parser.parse_args()
    here = Path(__file__).resolve().parent
    out = private(PRIVATE / "research-temp/nflstreet-actor-scale-20261014")
    out.mkdir(parents=True, exist_ok=True)
    report = out / ("build-verification.json" if args.build_only else "verification.json")
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY}
    report.write_text(json.dumps(record, indent=2) + "\n")
    try:
        before = baseline_hashes()
        sources = {name: digest(here / name) for name in SOURCES}
        contexts = {i: PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in (14, 15, 16)}
        inputs = {str(path): digest(path) for path in contexts.values()}
        exe = build(here, out)
        binaries = {"baseline": digest(exe)}
        logs: dict[str, list[str]] = {}
        if not args.build_only:
            for i, context in contexts.items():
                traces = []
                for repeat in range(2):
                    log = out / f"original-{i}-{repeat}.log"
                    text = run([str(exe), windows(context), windows(BASELINES["original_dll"][0])], log)
                    match = SUMMARY.fullmatch(text.strip())
                    if not match:
                        raise RuntimeError(f"Missing complete original comparison in context {i}")
                    enabled, disabled, hits, misses, wide = map(int, match.groups())
                    if min(enabled, disabled, hits, misses, wide) <= 0 or enabled + disabled != 5614:
                        raise RuntimeError(f"Missing branch coverage in context {i}")
                    if hits + misses != enabled or wide > enabled:
                        raise RuntimeError(f"Inconsistent coverage in context {i}")
                    traces.append(digest(log))
                if traces[0] != traces[1]:
                    raise RuntimeError(f"Fresh process traces differ in context {i}")
                logs[str(i)] = traces
        mutants: dict[str, object] = {}
        if args.mutants:
            original = (here / "actor_scale.c").read_text()
            for mutant in json.loads((here / "actor_scale_mutants.json").read_text()):
                name, old, new = (mutant[key] for key in ("name", "old", "new"))
                if original.count(old) != 1 or "\n" in old or "\n" in new:
                    raise RuntimeError(f"Mutant {name} must be a unique one-line replacement")
                # Repository source scratch stays inside this worktree and is removed after building.
                with tempfile.TemporaryDirectory(prefix=".actor-scale-mutant-", dir=here) as scratch:
                    source = Path(scratch) / "actor_scale.c"
                    source.write_text(original.replace(old, new))
                    mutant_exe = build(here, out / "mutants" / name, source)
                binaries[name] = digest(mutant_exe)
                if args.build_only:
                    mutants[name] = "compiled; execution pending"
                    continue
                caught = []
                for i, context in contexts.items():
                    log = mutant_exe.parent / f"original-{i}.log"
                    with log.open("w") as stream:
                        result = subprocess.run(
                            [str(mutant_exe), windows(context), windows(BASELINES["original_dll"][0])],
                            stdout=stream,
                            stderr=subprocess.STDOUT,
                            check=False,
                        )
                    text = log.read_text()
                    if result.returncode != 7 or "RAM differs at " not in text:
                        raise RuntimeError(f"Mutant {name} was not caught by RAM comparison in context {i}")
                    caught.append(i)
                mutants[name] = {"caught_in_contexts": caught}
        if before != baseline_hashes() or any(digest(Path(path)) != sha for path, sha in inputs.items()):
            raise RuntimeError("Protected originals or private inputs changed")
        if sources != {name: digest(here / name) for name in SOURCES}:
            raise RuntimeError("Source changed during verification")
        record.update(
            status="compiled-not-run" if args.build_only else "passed",
            baseline=before,
            input_sha256=inputs,
            source_sha256=sources,
            executable_sha256=binaries,
            original_log_sha256=logs,
            mutants=mutants,
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    if args.build_only:
        print(f"Actor scale compiled; Windows checks and mutation execution pending; private report {report}")
    else:
        print(f"Original actor scale passes all three contexts; private report {report}")
        if args.mutants:
            print(f"Actor scale mutants caught: {len(mutants)} in all three contexts")


if __name__ == "__main__":
    main()
