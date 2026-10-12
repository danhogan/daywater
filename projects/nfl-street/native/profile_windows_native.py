"""Run paired offline timing trials of the local Windows static recompilation.

Run with Windows Python. Inputs and logs remain on private W: storage.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import statistics
import subprocess
import sys
from pathlib import Path


ROOT = Path(r"W:\Daywater\research-temp")
BUNDLE = ROOT / "nflstreet-native-windows-local"
NORMAL = BUNDLE / "moderngekko-run.exe"
TIMED = (Path(r"W:\Daywater\tools\ModernGekko-Template\lib\ModernGekko") /
         "build-win64/moderngekko-run-timed-sites.exe")
HARNESS = ROOT / "run_windows_headless_bundle.py"
SITE = re.compile(
    r"\[staticrecomp\] timed-site pc=([0-9a-f]{8}) samples=(\d+) "
    r"total_ns=(\d+) max_ns=(\d+)")


def sha1(path: Path) -> str:
    digest = hashlib.sha1()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(8 * 1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def parse_output(output: str, timed: bool) -> dict[str, object]:
    benchmark = re.search(r"frame_benchmark=\{([^}]*)\}", output)
    shutdown = re.search(r"\[staticrecomp\] shutdown: ([^\r\n]*)", output)
    if not benchmark or not shutdown:
        raise RuntimeError("missing frame benchmark or static-core shutdown")
    fields = dict(re.findall(r"(\w+)=([^,}]+)", benchmark.group(1)))
    shutdown_fields = dict(re.findall(r"(\w+)=(\d+)", shutdown.group(1)))
    result: dict[str, object] = {
        "frame_start": int(fields["frame_start"]),
        "frame_delta": int(fields["frame_delta"]),
        "wall_seconds": float(fields["wall_seconds"]),
        "observed_fps": float(fields["observed_fps"]),
        "frame_counter_resets": int(fields.get("frame_counter_resets", "0")),
        "native_dispatches": int(shutdown_fields["native"]),
        "fallback_steps": int(shutdown_fields["fallback"]),
        "failed_chunks": int(shutdown_fields["smc_failed"]),
    }
    sites = [
        {"pc": match.group(1), "samples": int(match.group(2)),
         "total_ns": int(match.group(3)), "max_ns": int(match.group(4))}
        for match in SITE.finditer(output)
    ]
    if timed:
        header = re.search(r"\[staticrecomp\] timed-sites samples=(\d+) sites=(\d+)", output)
        if not header or not sites:
            raise RuntimeError("timed runner produced no guest-PC timings")
        result["timed_samples"] = int(header.group(1))
        result["timed_site_count"] = int(header.group(2))
        result["top_sites"] = sites
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--frames", type=int, default=600)
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--max-seconds", type=int, default=180)
    parser.add_argument("--harness", type=Path, default=HARNESS)
    parser.add_argument("--normal-runner", type=Path, default=NORMAL)
    parser.add_argument("--timed-runner", type=Path, default=TIMED)
    args = parser.parse_args()
    if args.frames < 30 or args.repeats < 1 or args.max_seconds < 30:
        parser.error("frames must be >=30, repeats >=1, max-seconds >=30")
    if args.output.exists():
        parser.error(f"output already exists: {args.output}")
    inputs = (args.harness, args.normal_runner, args.timed_runner,
              BUNDLE / "gGNNE69_recomp.dll", BUNDLE / "game", BUNDLE / "GNNE69.iso",
              BUNDLE / "match.sav", BUNDLE / "user")
    for path in inputs:
        if not path.exists():
            parser.error(f"missing input: {path}")
    args.output.mkdir(parents=True)
    summary: dict[str, object] = {
        "frames_requested": args.frames,
        "repeats": args.repeats,
        "normal_runner_sha1": sha1(args.normal_runner),
        "timed_runner_sha1": sha1(args.timed_runner),
        "module_sha1": sha1(BUNDLE / "gGNNE69_recomp.dll"),
        "savestate_sha1": sha1(BUNDLE / "match.sav"),
        "runs": [],
    }
    runs: list[dict[str, object]] = summary["runs"]  # type: ignore[assignment]
    for round_index in range(args.repeats):
        order = ("normal", "timed") if round_index % 2 == 0 else ("timed", "normal")
        for variant in order:
            runner = args.timed_runner if variant == "timed" else args.normal_runner
            command = [
                sys.executable, str(args.harness),
                "--runner", str(runner),
                "--module", str(BUNDLE / "gGNNE69_recomp.dll"),
                "--game", str(BUNDLE / "game"),
                "--iso", str(BUNDLE / "GNNE69.iso"),
                "--user-dir", str(BUNDLE / "user"),
                "--load-state", str(BUNDLE / "match.sav"),
                "--graphics", "Null", "--frames", str(args.frames),
                "--minimum-start-frame", "16000", "--seconds", str(args.max_seconds),
            ]
            if variant == "timed":
                command.append("--timed-sites")
            print(f"round={round_index + 1} variant={variant} starting", flush=True)
            completed = subprocess.run(command, text=True, errors="replace",
                                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            log_name = f"round-{round_index + 1:02d}-{variant}.log"
            (args.output / log_name).write_text(completed.stdout, encoding="utf-8")
            if completed.returncode != 0:
                raise RuntimeError(f"{log_name} exited {completed.returncode}")
            result = parse_output(completed.stdout, variant == "timed")
            if (result["frame_start"] < 16000 or
                    result["frame_counter_resets"] != 0 or
                    result["frame_delta"] < args.frames or
                    result["fallback_steps"] != 0 or
                    result["failed_chunks"] != 0):
                raise RuntimeError(f"{log_name} failed replay checks: {result}")
            result.update({"round": round_index + 1, "variant": variant, "log": log_name})
            runs.append(result)
            (args.output / "summary.json").write_text(
                json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
            print(f"round={round_index + 1} variant={variant} "
                  f"fps={result['observed_fps']:.2f} frames={result['frame_delta']}", flush=True)
    medians = {
        variant: statistics.median(
            float(run["observed_fps"]) for run in runs if run["variant"] == variant)
        for variant in ("normal", "timed")
    }
    site_totals: dict[str, dict[str, int]] = {}
    for run in runs:
        for site in run.get("top_sites", []):
            item = site_totals.setdefault(site["pc"], {"samples": 0, "total_ns": 0})
            item["samples"] += site["samples"]
            item["total_ns"] += site["total_ns"]
    summary["median_fps"] = medians
    summary["timed_to_normal_fps_ratio"] = medians["timed"] / medians["normal"]
    summary["top_sites_from_reported_32_per_run"] = [
        {"pc": pc, **totals}
        for pc, totals in sorted(site_totals.items(),
                                 key=lambda entry: entry[1]["total_ns"], reverse=True)[:32]
    ]
    (args.output / "summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({"output": str(args.output), "median_fps": medians,
                      "top_sites": summary["top_sites_from_reported_32_per_run"][:5]},
                     indent=2), flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
