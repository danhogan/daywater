"""Interactively probe controller selection after a clean NFL Street boot.

Run in the Daywater container with Dolphin 2603a. Commands on stdin:
``press A 2`` (button and seconds to settle), ``wait 5``, ``save label``,
and ``quit``. Raw MEM1 captures remain temporary until moved to W:.
"""

from __future__ import annotations

import json
import shutil
import sys
import time
from pathlib import Path

import src.core.dolphin.runner as runner
import src.core.dolphin.session as session_module
from src.core.dolphin.input import InputCommand, InputSequence

ISO = Path("/app/cache/isos/813c4511ba3135059a13de424555647e8252254b.iso")
OUT = Path("/app/cache/research-temp/clean-match-selection-probe")
LAUNCHER = "/tmp/daywater-dolphin-2603a"
MEM1_BASE = 0x80000000
MEM1_SIZE = 0x01800000


def snapshot(session: session_module.DolphinSession, label: str, user_dir: Path, save_mem1: bool) -> None:
    mapping = session.read_u32(0x803A1E30)
    entries = session.read_u32(mapping) if mapping else 0
    team = index = None
    if entries:
        team, index = session.read_bytes(entries, 2)
    manager = session.read_u32(0x803A1E10)
    actor = 0
    name = None
    if manager and team is not None and index is not None and team != 0xFF and index != 0xFF:
        actor_base = session.read_u32(manager)
        count = int.from_bytes(session.read_bytes(manager + 8, 2), "big")
        if actor_base and 1 <= count <= 20 and team <= 1 and index < count:
            actor = actor_base + (team * count + index) * 0x14D4
            name = session.read_bytes(actor + 0xB80, 16).split(b"\0", 1)[0].decode("ascii", "replace")
    root = session.read_u32(0x8027BE8C)
    first_actor = session.read_u32(root + 0xFC) if root else 0
    record = {
        "label": label, "controller_map_global": f"0x{mapping:08X}",
        "entry_array": f"0x{entries:08X}", "team": team, "index": index,
        "roster_manager": f"0x{manager:08X}", "resolved_actor": f"0x{actor:08X}",
        "resolved_name": name, "first_visual_object_actor": f"0x{first_actor:08X}",
    }
    frames = sorted((user_dir / "Dump" / "Frames").glob("*.png"), key=lambda p: p.stat().st_mtime_ns)
    if len(frames) >= 8:
        source = frames[-8]
        shutil.copy2(source, OUT / f"{label}.png")
        record["source_frame"] = source.name
    if save_mem1:
        (OUT / f"{label}.mem1").write_bytes(session.read_bytes(MEM1_BASE, MEM1_SIZE))
    (OUT / f"{label}.json").write_text(json.dumps(record, indent=2) + "\n")
    print(json.dumps(record), flush=True)


def main() -> None:
    session_module._find_linux_dolphin = lambda: LAUNCHER
    runner._find_linux_dolphin = lambda: LAUNCHER
    session_module.DEFAULT_DOLPHIN_INI += "\n[Input]\nBackgroundInput = True\n"
    OUT.mkdir(parents=True, exist_ok=False)
    user_dir = OUT / "dolphin-user"
    try:
        with session_module.DolphinSession.start(
            ISO, pipe_input=True, video_backend="OGL", user_dir=user_dir,
        ) as session:
            if not session.wait_for_first_frame(90):
                raise RuntimeError("No first frame")
            time.sleep(5)
            snapshot(session, "boot", user_dir, False)
            for line in sys.stdin:
                parts = line.strip().split()
                if not parts:
                    continue
                if parts[0] == "quit":
                    break
                if parts[0] == "press":
                    button = parts[1].upper()
                    session.play_sequence(InputSequence([
                        InputCommand(0, f"PRESS {button}"),
                        InputCommand(0.15, f"RELEASE {button}"),
                    ]))
                    time.sleep(float(parts[2]) if len(parts) > 2 else 1)
                elif parts[0] == "wait":
                    time.sleep(float(parts[1]))
                elif parts[0] != "save":
                    print("Unknown command", flush=True)
                    continue
                label = parts[1] if parts[0] == "save" else f"step-{int(time.time())}"
                snapshot(session, label, user_dir, parts[0] == "save")
    finally:
        if user_dir.exists():
            shutil.rmtree(user_dir)


if __name__ == "__main__":
    main()
