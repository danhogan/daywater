"""Print read-only NFL Street observations from MEM1 or a live Dolphin run.

Offline: python projects/nfl-street/tools/observe.py --image capture.mem1
Live:    python projects/nfl-street/tools/observe.py --iso game.iso \
             --savestate state.sav --dolphin-binary /path/to/dolphin-emu
"""

from __future__ import annotations

import argparse
import json
import sys
import tempfile
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from nflstreet_core.observer import Mem1Image, NFLStreetObserver  # noqa: E402


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--image", type=Path, help="24 MiB MEM1 capture")
    source.add_argument("--iso", type=Path, help="game image for a new Dolphin session")
    parser.add_argument("--savestate", type=Path)
    parser.add_argument("--dolphin-binary", type=Path)
    parser.add_argument("--duration", type=float, default=10.0, help="live sampling seconds")
    parser.add_argument("--interval", type=float, default=0.5, help="live sampling interval")
    args = parser.parse_args()

    if args.image:
        observation = NFLStreetObserver(Mem1Image(args.image.read_bytes())).observe()
        print(json.dumps(observation.to_dict()))
        return

    if args.duration <= 0 or args.interval <= 0:
        parser.error("duration and interval must be positive")

    import src.core.dolphin.runner as runner
    import src.core.dolphin.session as session_module

    if args.dolphin_binary:
        binary = str(args.dolphin_binary)
        runner._find_linux_dolphin = lambda: binary
        session_module._find_linux_dolphin = lambda: binary

    with tempfile.TemporaryDirectory(prefix="nflstreet-observer-") as directory:
        with session_module.DolphinSession.start(
            args.iso, savestate=args.savestate, video_backend="OGL",
            user_dir=Path(directory),
        ) as session:
            if not session.wait_for_first_frame(90):
                raise RuntimeError("Dolphin produced no completed frame")
            observer = NFLStreetObserver(session)
            start = time.monotonic()
            while True:
                elapsed = time.monotonic() - start
                if elapsed > args.duration:
                    break
                record = observer.observe().to_dict()
                record["elapsed_seconds"] = round(elapsed, 3)
                print(json.dumps(record), flush=True)
                time.sleep(min(args.interval, max(0, args.duration - elapsed)))


if __name__ == "__main__":
    main()
