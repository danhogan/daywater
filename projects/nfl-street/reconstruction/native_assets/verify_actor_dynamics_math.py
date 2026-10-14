#!/usr/bin/env python3
"""Run the verified scalar and matrix leaves needed by actor dynamics."""

import json
from pathlib import Path

from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

EXPECTED = (
    "Dynamics math leaves: 20003 80188D98 atan, 20003 8004BED8 clamp, 20003 vector-kernel, "
    "and 20003 matrix-kernel cases match bit-exactly."
)
SOURCES = (
    "actor_dynamics_math.c",
    "actor_dynamics_math.h",
    "actor_dynamics_math_original_check.c",
    "build_windows_actor_dynamics_math_original_check.sh",
    "verify_actor_dynamics_math.py",
    "actor_dynamics_math.md",
)


def main() -> None:
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / "research-temp/nflstreet-actor-dynamics-20261014")
    out.mkdir(parents=True, exist_ok=True)
    report = out / "math-verification.json"
    entries = tuple("native_assets/" + name for name in SOURCES)
    record: dict[str, object] = {"status": "running"}
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        contexts = {i: PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in (14, 15, 16)}
        inputs = {str(path): digest(path) for path in contexts.values()}
        run(
            ["bash", str(here / "build_windows_actor_dynamics_math_original_check.sh"), str(out)],
            out / "math-build.log",
        )
        exe = out / "actor-dynamics-math-original-check.exe"
        logs: dict[str, list[str]] = {}
        for index, context in contexts.items():
            traces = []
            for repeat in range(2):
                log = out / f"math-{index}-{repeat}.log"
                text = run([str(exe), windows(context), windows(BASELINES["original_dll"][0])], log)
                if text.replace("\r\n", "\n").strip() != EXPECTED:
                    raise RuntimeError(f"Dynamics math mismatch in context {index}")
                traces.append(str(digest(log)))
            if traces[0] != traces[1]:
                raise RuntimeError(f"Nondeterministic dynamics math trace in context {index}")
            logs[str(index)] = traces
        if before != baseline_hashes() or any(digest(Path(path)) != value for path, value in inputs.items()):
            raise RuntimeError("Protected original or private input changed")
        if sources != source_hashes(root, entries):
            raise RuntimeError("Source changed during verification")
        record.update(status="passed", baseline=before, input_sha256=inputs, source_sha256=sources, logs=logs)
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print("Actor dynamics math leaves pass; private report " + str(report))


if __name__ == "__main__":
    main()
