"""Capture a frame-counted MEM1 checkpoint from a private Windows NFL Street bundle.

Run with Windows Python. Raw memory stays under --output on W:.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time

from profile_windows_native import BUNDLE, sha1


def fields(path: Path) -> dict[str, str]:
    try:
        return dict(line.split("=", 1) for line in path.read_text(encoding="utf-8").splitlines()
                    if "=" in line)
    except OSError:
        return {}


def wait_for(test, process: subprocess.Popen[str], timeout: float, description: str) -> None:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if test():
            return
        if process.poll() is not None:
            raise RuntimeError(f"runner exited while waiting for {description}: {process.returncode}")
        time.sleep(0.02)
    raise TimeoutError(f"timed out waiting for {description}")


def queue(directory: Path, number: int, name: str, body: str) -> Path:
    path = directory / "commands" / f"{number:03d}_{name}.txt"
    path.write_text(body, encoding="utf-8")
    return directory / "processed" / path.name


def capture(module: Path, output: Path, frames: int, timeout: int) -> dict[str, object]:
    output.mkdir(parents=True)
    automation = output / "automation"
    for subdir in ("commands", "processed", "failed", "artifacts"):
        (automation / subdir).mkdir(parents=True)
    runner = BUNDLE / "moderngekko-run.exe"
    saved = BUNDLE / "match.sav"
    env = os.environ.copy()
    env["MODERNGEKKO_BOOT_IMAGE"] = str(BUNDLE / "GNNE69.iso")
    command = [str(runner), "--game", str(BUNDLE / "game"), "--user-dir",
               str(BUNDLE / "user"), "--automation-dir", str(automation),
               "--graphics", "Null", "--module", str(module), "--headless",
               "--load-state", str(saved)]
    paused = queue(automation, 0, "pause", "command=pause\n")
    with (output / "runner.log").open("w", encoding="utf-8") as log:
        process = subprocess.Popen(command, env=env, stdout=log, stderr=subprocess.STDOUT,
                                   text=True, errors="replace")
        try:
            wait_for(paused.exists, process, timeout, "initial pause")
            reloaded = queue(automation, 1, "reload", f"command=load_state\npath={saved}\n")
            wait_for(reloaded.exists, process, timeout, "state reload")
            status = automation / "status.txt"
            wait_for(lambda: int(fields(status).get("frame_count", "0")) >= 16000,
                     process, timeout, "restored frame counter")
            start_frame = int(fields(status)["frame_count"])
            resumed = queue(automation, 2, "resume", "command=resume\n")
            stepped = queue(automation, 3, "frames",
                            f"command=pad_frames\nport=0\nframes={frames}\n")
            paused_after = queue(automation, 4, "pause", "command=pause\n")
            first = queue(automation, 5, "memory_first",
                          "command=read_memory\naddress=0x80000000\nsize=12582912\n"
                          "path=artifacts/mem1_first.bin\n")
            second = queue(automation, 6, "memory_second",
                           "command=read_memory\naddress=0x80c00000\nsize=12582912\n"
                           "path=artifacts/mem1_second.bin\n")
            for marker, label in ((resumed, "resume"), (stepped, "frame count"),
                                  (paused_after, "checkpoint pause"),
                                  (first, "first memory half"), (second, "second memory half")):
                wait_for(marker.exists, process, timeout, label)
            checkpoint = fields(status)
            stopped = queue(automation, 7, "stop", "command=stop\n")
            wait_for(stopped.exists, process, timeout, "stop")
            process.wait(timeout=timeout)
            if process.returncode != 0 or any((automation / "failed").iterdir()):
                raise RuntimeError("runner or automation command failed")
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()
    images = [automation / "artifacts" / name for name in
              ("mem1_first.bin", "mem1_second.bin")]
    digest = hashlib.sha256()
    for path in images:
        with path.open("rb") as stream:
            for block in iter(lambda: stream.read(1024 * 1024), b""):
                digest.update(block)
    result: dict[str, object] = {
        "module_sha1": sha1(module), "runner_sha1": sha1(runner),
        "savestate_sha1": sha1(saved), "start_frame": start_frame,
        "checkpoint_frame": int(checkpoint["frame_count"]),
        "frames_requested": frames, "mem1_sha256": digest.hexdigest(),
        "exit_code": process.returncode,
    }
    (output / "checkpoint.json").write_text(json.dumps(result, indent=2) + "\n",
                                             encoding="utf-8")
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--module", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--frames", type=int, default=600)
    parser.add_argument("--timeout", type=int, default=180)
    args = parser.parse_args()
    if args.frames < 1 or args.frames > 36000 or args.timeout < 10:
        parser.error("frames must be 1..36000 and timeout at least 10")
    if args.output.exists() or not args.module.is_file():
        parser.error("output must not exist and module must be a file")
    print(json.dumps(capture(args.module, args.output, args.frames, args.timeout), indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
