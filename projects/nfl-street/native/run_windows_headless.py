"""Bounded native Windows boot check for the local GNNE69 research image."""

import argparse
import os
from pathlib import Path
import signal
import subprocess
import threading
import time


ROOT = Path(r"W:\Daywater")
BUILD = ROOT / "tools/ModernGekko-Template/lib/ModernGekko/build-win64"
GAME = ROOT / "research-temp/nflstreet-moderngekko-game"
ISO = ROOT / "research-temp/nflstreet-GNNE69.iso"
MODULE_FILE = Path(r"W:\nfsmod\GNNE69\active-module.txt")
USER = ROOT / "research-temp/nflstreet-moderngekko-win-user"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--graphics", default="Null")
    parser.add_argument("--user-dir", type=Path, default=USER)
    parser.add_argument("--windowed", action="store_true")
    parser.add_argument("--seconds", type=int, default=30)
    parser.add_argument("--frames", type=int, help="stop after this many observed emulated frames")
    parser.add_argument("--minimum-start-frame", type=int,
                        help="require the measured interval to start at or above this saved-state frame")
    parser.add_argument("--screenshot", action="store_true")
    parser.add_argument("--screenshot-at", type=float, default=10)
    parser.add_argument("--screenshot-name", default="boot.png")
    parser.add_argument("--press-start-at", type=float)
    parser.add_argument("--press-a-at", type=float, action="append", default=[])
    parser.add_argument("--pad-at", action="append", default=[], metavar="SECONDS:CONTROL")
    parser.add_argument("--save-state-at", type=float)
    parser.add_argument("--save-state-name", default="menu.sav")
    parser.add_argument("--load-state", type=Path)
    parser.add_argument("--module-file", type=Path, default=MODULE_FILE)
    parser.add_argument("--module", type=Path, help="direct module path, for a staged bundle")
    parser.add_argument("--game", type=Path, default=GAME)
    parser.add_argument("--iso", type=Path, default=ISO)
    parser.add_argument("--runner", type=Path, default=BUILD / "moderngekko-run.exe")
    parser.add_argument("--static-no-module-baseline", action="store_true")
    parser.add_argument("--jit-baseline", action="store_true")
    parser.add_argument("--dispatch-samples", action="store_true")
    parser.add_argument("--timing-samples", action="store_true")
    parser.add_argument("--timed-sites", action="store_true",
                        help="sample inclusive generated dispatch time by guest PC")
    parser.add_argument("--pair-probe", action="store_true",
                        help="log bounded inputs and outputs for the two paired-function candidates")
    parser.add_argument("--live-diff-start", type=int,
                        help="start live function comparisons after this many calls per function")
    args = parser.parse_args()
    if args.frames is not None and args.frames <= 0:
        parser.error("--frames must be positive")
    if args.minimum_start_frame is not None and (
            args.frames is None or args.minimum_start_frame < 0):
        parser.error("--minimum-start-frame requires --frames and a nonnegative value")
    if args.live_diff_start is not None and not 0 <= args.live_diff_start <= 1000000000:
        parser.error("--live-diff-start must be between 0 and 1000000000")
    if args.static_no_module_baseline and args.jit_baseline:
        parser.error("choose only one baseline mode")
    allowed_controls = {"a", "b", "x", "y", "start", "dpad_up", "dpad_down", "dpad_left", "dpad_right"}
    pad_events = []
    for item in args.pad_at:
        try:
            when, control = item.split(":", 1)
            when = float(when)
        except ValueError:
            parser.error("--pad-at requires SECONDS:CONTROL")
        if control not in allowed_controls or when < 0:
            parser.error(f"invalid --pad-at value: {item}")
        pad_events.append((when, control))
    baseline = args.static_no_module_baseline or args.jit_baseline
    suffix = "-jit-baseline" if args.jit_baseline else "-baseline" if args.static_no_module_baseline else ""
    automation = ROOT / f"research-temp/nflstreet-moderngekko-win-{args.graphics.lower()}{suffix}-automation"
    module = args.module or Path(args.module_file.read_text(encoding="utf-8").strip())
    assert args.iso.is_file() and args.game.is_dir() and (baseline or module.is_file())
    status = automation / "status.txt"
    status.unlink(missing_ok=True)
    screenshot = automation / "artifacts" / args.screenshot_name
    if args.screenshot:
        screenshot.unlink(missing_ok=True)

    environment = os.environ.copy()
    environment["MODERNGEKKO_BOOT_IMAGE"] = str(args.iso)
    if args.jit_baseline:
        environment["MODERNGEKKO_STATICRECOMP"] = "0"
    if args.dispatch_samples:
        environment["STATICRECOMP_DISPATCH_SAMPLES"] = "1"
    if args.timing_samples:
        environment["STATICRECOMP_TIMING_SAMPLES"] = "1"
    if args.timed_sites:
        environment["STATICRECOMP_TIMED_SITES"] = "1"
    if args.pair_probe:
        environment["STATICRECOMP_PAIR_PROBE"] = "1"
    if args.live_diff_start is not None:
        environment["NFLSTREET_LIVE_DIFF_START"] = str(args.live_diff_start)
    command = [
        str(args.runner),
        "--game", str(args.game),
        "--user-dir", str(args.user_dir),
        "--automation-dir", str(automation),
        "--graphics", args.graphics,
    ]
    if baseline:
        command.append("--allow-interpreter")
    else:
        command.extend(("--module", str(module)))
    if not args.windowed:
        command.append("--headless")
    if args.load_state:
        command.extend(("--load-state", str(args.load_state)))
    process = subprocess.Popen(
        command,
        env=environment,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        errors="replace",
        creationflags=subprocess.CREATE_NEW_PROCESS_GROUP,
    )
    def queue_command(name: str, contents: str) -> None:
        commands = automation / "commands"
        commands.mkdir(parents=True, exist_ok=True)
        (commands / f"{time.monotonic_ns()}_{name}.txt").write_text(
            contents, encoding="utf-8"
        )

    if args.screenshot:
        threading.Timer(
            args.screenshot_at,
            lambda: queue_command(
                "screenshot", f"command=screenshot\npath=artifacts/{args.screenshot_name}\n"
            ),
        ).start()
    if args.press_start_at is not None:
        threading.Timer(
            args.press_start_at,
            lambda: queue_command("start", "command=pad_frames\nport=0\nframes=10\nstart=1\n"),
        ).start()
    for press_time in args.press_a_at:
        threading.Timer(
            press_time,
            lambda: queue_command("a", "command=pad_frames\nport=0\nframes=10\na=1\n"),
        ).start()
    for when, control in pad_events:
        threading.Timer(
            when,
            lambda control=control: queue_command(
                control, f"command=pad_frames\nport=0\nframes=10\n{control}=1\n"
            ),
        ).start()
    if args.save_state_at is not None:
        threading.Timer(
            args.save_state_at,
            lambda: queue_command(
                "save_state", f"command=save_state\npath=artifacts/{args.save_state_name}\n"
            ),
        ).start()
    benchmark: dict[str, float | int | bool] = {}
    monitor_done = threading.Event()

    def monitor_frames() -> None:
        while not monitor_done.wait(0.05):
            try:
                fields = dict(
                    line.split("=", 1)
                    for line in status.read_text(encoding="utf-8").splitlines()
                    if "=" in line
                )
                frame = int(fields.get("frame_count", "0"))
            except (OSError, ValueError):
                continue
            if fields.get("booted") != "1" or frame <= 0:
                continue
            now = time.monotonic()
            if ("frame_start" not in benchmark or
                    frame < int(benchmark.get("frame_observed", frame))):
                if "frame_start" in benchmark:
                    benchmark["frame_counter_resets"] = int(
                        benchmark.get("frame_counter_resets", 0)) + 1
                benchmark["frame_start"] = frame
                benchmark["wall_start"] = now
            benchmark["frame_observed"] = frame
            if frame - int(benchmark["frame_start"]) >= args.frames:
                benchmark["reached"] = True
                benchmark["wall_seconds"] = now - float(benchmark["wall_start"])
                benchmark["frame_delta"] = frame - int(benchmark["frame_start"])
                benchmark["observed_fps"] = (
                    int(benchmark["frame_delta"]) / float(benchmark["wall_seconds"])
                )
                queue_command("frame_stop", "command=stop\n")
                return

    monitor = None
    if args.frames is not None:
        monitor = threading.Thread(target=monitor_frames, daemon=True)
        monitor.start()
    try:
        output, _ = process.communicate(timeout=args.seconds)
    except subprocess.TimeoutExpired:
        queue_command("stop", "command=stop\n")
        try:
            output, _ = process.communicate(timeout=10)
        except subprocess.TimeoutExpired:
            process.send_signal(signal.CTRL_BREAK_EVENT)
            try:
                output, _ = process.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                output, _ = process.communicate()
    monitor_done.set()
    if monitor is not None:
        monitor.join(timeout=1)
    print(output)
    print(f"exit_code={process.returncode}")
    if args.frames is not None:
        print(f"frame_benchmark={{{', '.join(f'{key}={value}' for key, value in benchmark.items() if key != 'wall_start')}}}")
    if status.is_file():
        print(status.read_text(encoding="utf-8"))
    else:
        print("status.txt was not produced")
    if args.screenshot:
        print(f"screenshot={screenshot if screenshot.is_file() else 'missing'}")
    return 0 if process.returncode == 0 and status.is_file() and (
        args.frames is None or benchmark.get("reached") is True
    ) and (
        args.minimum_start_frame is None or (
            int(benchmark.get("frame_start", -1)) >= args.minimum_start_frame
            and int(benchmark.get("frame_counter_resets", 0)) == 0)
    ) and (
        not args.screenshot or screenshot.is_file()
    ) else 1


if __name__ == "__main__":
    raise SystemExit(main())
