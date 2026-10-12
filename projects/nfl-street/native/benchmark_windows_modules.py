"""Run paired fixed-frame Windows trials for the baseline and candidate DLL."""

from __future__ import annotations

import argparse
import json
import statistics
import subprocess
import sys
from pathlib import Path

from profile_windows_native import BUNDLE, HARNESS, ROOT, parse_output, sha1


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--frames", type=int, default=600)
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--max-seconds", type=int, default=180)
    parser.add_argument("--baseline-module", type=Path,
                        default=BUNDLE / "gGNNE69_recomp.dll")
    parser.add_argument("--candidate-module", type=Path,
                        default=ROOT / "gGNNE69-recomp-odd-fastpath.dll")
    args = parser.parse_args()
    if args.output.exists():
        parser.error(f"output already exists: {args.output}")
    if args.frames < 30 or args.repeats < 1 or args.max_seconds < 30:
        parser.error("frames must be >=30, repeats >=1, max-seconds >=30")
    runner = BUNDLE / "moderngekko-run.exe"
    inputs = (runner, HARNESS, args.baseline_module, args.candidate_module,
              BUNDLE / "game", BUNDLE / "GNNE69.iso", BUNDLE / "match.sav", BUNDLE / "user")
    for path in inputs:
        if not path.exists():
            parser.error(f"missing input: {path}")
    args.output.mkdir(parents=True)
    summary: dict[str, object] = {
        "frames_requested": args.frames,
        "repeats": args.repeats,
        "runner_sha1": sha1(runner),
        "baseline_module_sha1": sha1(args.baseline_module),
        "candidate_module_sha1": sha1(args.candidate_module),
        "savestate_sha1": sha1(BUNDLE / "match.sav"),
        "runs": [],
    }
    runs: list[dict[str, object]] = summary["runs"]  # type: ignore[assignment]
    for round_index in range(args.repeats):
        order = ("baseline", "candidate") if round_index % 2 == 0 else ("candidate", "baseline")
        for variant in order:
            module = args.baseline_module if variant == "baseline" else args.candidate_module
            command = [
                sys.executable, str(HARNESS),
                "--runner", str(runner), "--module", str(module),
                "--game", str(BUNDLE / "game"), "--iso", str(BUNDLE / "GNNE69.iso"),
                "--user-dir", str(BUNDLE / "user"),
                "--load-state", str(BUNDLE / "match.sav"),
                "--graphics", "Null", "--frames", str(args.frames),
                "--minimum-start-frame", "16000", "--seconds", str(args.max_seconds),
            ]
            print(f"round={round_index + 1} variant={variant} starting", flush=True)
            completed = subprocess.run(command, text=True, errors="replace",
                                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            log_name = f"round-{round_index + 1:02d}-{variant}.log"
            (args.output / log_name).write_text(completed.stdout, encoding="utf-8")
            if completed.returncode != 0:
                raise RuntimeError(f"{log_name} exited {completed.returncode}")
            result = parse_output(completed.stdout, False)
            if (result["frame_start"] < 16000 or
                    result["frame_counter_resets"] != 0 or
                    result["frame_delta"] < args.frames or
                    result["fallback_steps"] != 0 or
                    result["failed_chunks"] != 0):
                raise RuntimeError(f"{log_name} failed replay checks: {result}")
            result["candidate_first_hit"] = "[nflstreet-fastpath] first-hit" in completed.stdout
            if variant == "candidate" and not result["candidate_first_hit"]:
                raise RuntimeError(f"{log_name} did not execute the guarded candidate")
            result.update({"round": round_index + 1, "variant": variant, "log": log_name})
            runs.append(result)
            (args.output / "summary.json").write_text(
                json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
            print(f"round={round_index + 1} variant={variant} "
                  f"fps={result['observed_fps']:.2f} frames={result['frame_delta']}", flush=True)
    summary["median_fps"] = {
        variant: statistics.median(float(run["observed_fps"])
                                   for run in runs if run["variant"] == variant)
        for variant in ("baseline", "candidate")
    }
    (args.output / "summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({"output": str(args.output), "median_fps": summary["median_fps"]},
                     indent=2), flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
