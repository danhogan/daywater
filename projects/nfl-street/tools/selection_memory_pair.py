"""Capture paired NFL Street MEM1 images around a controlled controller input.

Run inside the Daywater container with the temporary Dolphin 2603a launcher.
Move the raw captures to W: after analysis; do not add them to the repository.
"""

from __future__ import annotations

import argparse
import json
import shutil
import time
from pathlib import Path

import src.core.dolphin.runner as runner
import src.core.dolphin.session as session_module
from src.core.dolphin.input import InputCommand, InputSequence

ISO = Path("/app/cache/isos/813c4511ba3135059a13de424555647e8252254b.iso")
STATES = Path("/app/sessions/58b4dd7af31f/savestates")
OUT = Path("/app/cache/research-temp")
LAUNCHER = "/tmp/daywater-dolphin-2603a"
MEM1_BASE = 0x80000000
MEM1_SIZE = 0x01800000
ROOT_SLOT = 0x8027BE8C
MANAGER_SLOT = 0x803A1E10


def completed_frame(user_dir: Path, dest: Path) -> str | None:
    frames = sorted((user_dir / "Dump" / "Frames").glob("*.png"), key=lambda p: p.stat().st_mtime_ns)
    if len(frames) < 8:
        return None
    source = frames[-8]
    shutil.copy2(source, dest)
    return source.name


def capture(label: str, state_id: str, button: str | None, post_input_wait: float) -> None:
    session_module._find_linux_dolphin = lambda: LAUNCHER
    runner._find_linux_dolphin = lambda: LAUNCHER
    session_module.DEFAULT_DOLPHIN_INI += "\n[Input]\nBackgroundInput = True\n"
    out = OUT / f"selection-memory-{label}"
    out.mkdir(parents=True, exist_ok=False)
    user_dir = out / "dolphin-user"

    with session_module.DolphinSession.start(
        ISO, savestate=STATES / state_id / "savestate.sav",
        pipe_input=True, video_backend="OGL", user_dir=user_dir,
    ) as session:
        if not session.wait_for_first_frame(90):
            raise RuntimeError("No first frame")
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline:
            if session.read_u32(MANAGER_SLOT) and session.read_u32(ROOT_SLOT):
                break
            time.sleep(0.2)
        else:
            raise RuntimeError("Savestate did not restore manager and object array")
        time.sleep(3)

        before = session.read_bytes(MEM1_BASE, MEM1_SIZE)
        (out / "before.mem1").write_bytes(before)
        before_frame = completed_frame(user_dir, out / "before.png")
        before_root = session.read_u32(ROOT_SLOT)
        before_actor = session.read_u32(before_root + 0xFC)

        if button:
            session.play_sequence(InputSequence([
                InputCommand(0, f"PRESS {button}"),
                InputCommand(0.15, f"RELEASE {button}"),
            ]))
        time.sleep(post_input_wait)
        after = session.read_bytes(MEM1_BASE, MEM1_SIZE)
        (out / "after.mem1").write_bytes(after)
        after_frame = completed_frame(user_dir, out / "after.png")
        after_root = session.read_u32(ROOT_SLOT)
        after_actor = session.read_u32(after_root + 0xFC)

    metadata = {
        "label": label,
        "state_id": state_id,
        "button": button,
        "button_duration_s": 0.15 if button else 0,
        "post_input_wait_s": post_input_wait,
        "dolphin_build": "Official Linux 2603a",
        "before_root": f"0x{before_root:08X}",
        "after_root": f"0x{after_root:08X}",
        "before_first_object_actor": f"0x{before_actor:08X}",
        "after_first_object_actor": f"0x{after_actor:08X}",
        "before_source_frame": before_frame,
        "after_source_frame": after_frame,
        "frame_warning": "Copied frames lag the RAM sample; use only stable visual states.",
    }
    (out / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n")
    shutil.rmtree(user_dir)
    print(json.dumps(metadata), flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("label")
    parser.add_argument("--state", default="27bb155a")
    parser.add_argument("--button", choices=("A", "B", "X", "Y"))
    parser.add_argument("--post-input-wait", type=float, default=1.5)
    args = parser.parse_args()
    capture(args.label, args.state, args.button, args.post_input_wait)
