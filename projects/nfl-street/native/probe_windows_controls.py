"""Compare a saved NFL Street play under neutral, left, and right input."""

import argparse
import os
from pathlib import Path
import subprocess
import time


ROOT = Path(r"W:\Daywater")
BUILD = ROOT / "tools/ModernGekko-Template/lib/ModernGekko/build-win64"
GAME = ROOT / "research-temp/nflstreet-moderngekko-game"
ISO = ROOT / "research-temp/nflstreet-GNNE69.iso"
USER = ROOT / "research-temp/nflstreet-moderngekko-win-user"
MODULE_FILE = Path(r"W:\nfsmod\GNNE69\active-module.txt")
STATE = ROOT / "research-temp/nflstreet-moderngekko-win-vulkan-automation/artifacts/early-play.sav"


def status_values(path: Path) -> dict[str, str]:
    if not path.is_file():
        return {}
    return dict(line.split("=", 1) for line in path.read_text().splitlines() if "=" in line)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("direction", choices=("neutral", "left", "right"))
    parser.add_argument("--frames", type=int, default=180)
    parser.add_argument("--trial", default="")
    parser.add_argument("--user-dir", type=Path, default=USER)
    parser.add_argument("--module-file", type=Path, default=MODULE_FILE)
    parser.add_argument("--module", type=Path)
    parser.add_argument("--runner", type=Path, default=BUILD / "moderngekko-run.exe")
    parser.add_argument("--game", type=Path, default=GAME)
    parser.add_argument("--iso", type=Path, default=ISO)
    parser.add_argument("--state", type=Path, default=STATE)
    args = parser.parse_args()
    if not 1 <= args.frames <= 36000:
        parser.error("--frames must be between 1 and 36000")
    if args.trial and not args.trial.isalnum():
        parser.error("--trial must contain letters and digits only")

    suffix = f"-{args.trial}" if args.trial else ""
    automation = ROOT / f"research-temp/nflstreet-moderngekko-win-control-{args.direction}{suffix}"
    artifacts = automation / "artifacts"
    artifacts.mkdir(parents=True, exist_ok=True)
    status = automation / "status.txt"
    status.unlink(missing_ok=True)
    for name in ("memory.bin", "frame.png"):
        (artifacts / name).unlink(missing_ok=True)
    module = args.module or Path(args.module_file.read_text().strip())
    for path in (args.runner, args.game, args.iso, args.state, module):
        if not path.exists():
            parser.error(f"missing input: {path}")
    environment = os.environ.copy()
    environment["MODERNGEKKO_BOOT_IMAGE"] = str(args.iso)
    command = [
        str(args.runner),
        "--game", str(args.game),
        "--module", str(module),
        "--user-dir", str(args.user_dir),
        "--automation-dir", str(automation),
        "--graphics", "Vulkan",
        "--load-state", str(args.state),
    ]
    process = subprocess.Popen(
        command, env=environment, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        text=True, errors="replace",
    )
    commands = automation / "commands"
    commands.mkdir(exist_ok=True)
    sequence = 0

    def queue(name: str, contents: str) -> None:
        nonlocal sequence
        sequence += 1
        (commands / f"{sequence:03d}_{name}.txt").write_text(contents)

    def wait_for(count: int, timeout: float = 120) -> dict[str, str]:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            values = status_values(status)
            if values.get("last_error"):
                raise RuntimeError(values["last_error"])
            if int(values.get("processed_commands", "0")) >= count:
                return values
            if process.poll() is not None:
                raise RuntimeError(f"runner exited early: {process.returncode}")
            time.sleep(0.1)
        raise TimeoutError(f"command {count} did not complete")

    try:
        deadline = time.monotonic() + 60
        while time.monotonic() < deadline:
            values = status_values(status)
            if values.get("booted") == "1" and int(values.get("frame_count", "0")) > 100:
                break
            if process.poll() is not None:
                raise RuntimeError(f"runner exited during boot: {process.returncode}")
            time.sleep(0.1)
        else:
            raise TimeoutError("runner did not boot")

        queue("snap", "command=pad_frames\nport=0\nframes=3\na=1\n")
        wait_for(1)
        axis = {"neutral": "", "left": "main_x=-1\n", "right": "main_x=1\n"}[args.direction]
        queue("motion", f"command=pad_frames\nport=0\nframes={args.frames}\n{axis}")
        wait_for(2)
        queue("memory", "command=read_memory\naddress=0x803A0000\nsize=2097152\npath=artifacts/memory.bin\n")
        wait_for(3)
        queue("screenshot", "command=screenshot\npath=artifacts/frame.png\n")
        values = wait_for(4)
        print(f"direction={args.direction} frame_count={values.get('frame_count')}")
        print(f"memory={artifacts / 'memory.bin'} screenshot={artifacts / 'frame.png'}")
    finally:
        if process.poll() is None:
            queue("stop", "command=stop\n")
        try:
            output, _ = process.communicate(timeout=15)
        except subprocess.TimeoutExpired:
            process.kill()
            output, _ = process.communicate()
        print(output)
        print(f"exit_code={process.returncode}")
    return 0 if process.returncode == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
