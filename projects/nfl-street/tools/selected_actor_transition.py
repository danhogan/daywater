"""Capture the first actor-reference object during a supplied NFL Street play.

The field correlated with player selection in the supplied match but is not
established as a generic selected-player locator. Run inside the Daywater
container with the official Dolphin 2603a launcher at
/tmp/daywater-dolphin-2603a. Output is temporary until moved to W:.
"""

from __future__ import annotations

import argparse
import csv
import json
import shutil
import struct
import time
from pathlib import Path

import src.core.dolphin.runner as runner
import src.core.dolphin.session as session_module
from src.core.dolphin.input import InputCommand, InputSequence

ISO = Path("/app/cache/isos/813c4511ba3135059a13de424555647e8252254b.iso")
STATE_ROOT = Path("/app/sessions/58b4dd7af31f/savestates")
OUT_ROOT = Path("/app/cache/research-temp")
LAUNCHER = "/tmp/daywater-dolphin-2603a"
POINTER_SLOT = 0x80DDFDCC
ROOT_SLOT = 0x8027BE8C
ACTOR_OFFSET = 0xFC
MANAGER_SLOT = 0x803A1E10
ACTOR_STRIDE = 0x14D4


def read_actor_info(session: session_module.DolphinSession, actor: int, base: int, count: int) -> tuple[str, str]:
    if actor < base or (actor - base) % ACTOR_STRIDE:
        return "", ""
    index = (actor - base) // ACTOR_STRIDE
    if index >= count * 2:
        return "", ""
    name = session.read_bytes(actor + 0xB80, 16).split(b"\0", 1)[0].decode("ascii", errors="replace")
    return str(index), name


def latest_frame(frames_dir: Path) -> Path | None:
    frames = sorted(frames_dir.glob("*.png"), key=lambda path: path.stat().st_mtime_ns)
    # Dolphin may still be writing its newest PNG. Retain an older complete
    # frame, and record its source name so timing can be checked later.
    return frames[-5] if len(frames) >= 5 else None


def capture(label: str, *, state_id: str, button: str | None, stick_x: float, duration: float) -> Path:
    session_module._find_linux_dolphin = lambda: LAUNCHER
    runner._find_linux_dolphin = lambda: LAUNCHER
    session_module.DEFAULT_DOLPHIN_INI += "\n[Input]\nBackgroundInput = True\n"

    out = OUT_ROOT / f"selected-actor-{label}"
    if out.exists():
        raise FileExistsError(out)
    out.mkdir(parents=True)
    frames_out = out / "frames"
    frames_out.mkdir()
    user_dir = out / "dolphin-user"
    samples: list[dict[str, object]] = []

    with session_module.DolphinSession.start(
        ISO, savestate=STATE_ROOT / state_id / "savestate.sav",
        pipe_input=True, video_backend="OGL", user_dir=user_dir
    ) as session:
        if not session.wait_for_first_frame(90):
            raise RuntimeError("Dolphin did not produce a frame")
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline:
            manager = session.read_u32(MANAGER_SLOT)
            selected = session.read_u32(POINTER_SLOT)
            if manager and selected:
                break
            time.sleep(0.2)
        else:
            raise RuntimeError("Savestate did not restore roster manager and candidate pointer")

        base = session.read_u32(manager)
        count = struct.unpack(">H", session.read_bytes(manager + 8, 2))[0]
        if not base or not 1 <= count <= 20:
            raise RuntimeError("Invalid roster pointer or team count")
        time.sleep(3)

        if button:
            session.play_sequence(InputSequence([
                InputCommand(0, f"PRESS {button}"), InputCommand(0.15, f"RELEASE {button}")
            ]))
        session.play_sequence_async(InputSequence([
            InputCommand(0, f"SET MAIN {stick_x} 0.5"),
            InputCommand(duration, "SET MAIN 0.5 0.5"),
        ]))
        start = time.monotonic()
        interval = 0.05
        next_frame = 0.0
        index = 0
        while True:
            elapsed = time.monotonic() - start
            if elapsed > duration:
                break
            object_base = session.read_u32(ROOT_SLOT)
            derived_slot = object_base + ACTOR_OFFSET if object_base else 0
            derived_actor = session.read_u32(derived_slot) if derived_slot else 0
            actor = session.read_u32(POINTER_SLOT)
            slot, name = read_actor_info(session, actor, base, count)
            row: dict[str, object] = {
                "elapsed_s": round(elapsed, 4),
                "pointer": f"0x{actor:08X}",
                "object_base": f"0x{object_base:08X}",
                "derived_slot": f"0x{derived_slot:08X}",
                "derived_actor": f"0x{derived_actor:08X}",
                "chain_matches_literal": derived_slot == POINTER_SLOT and derived_actor == actor,
                "actor_slot": slot,
                "actor_name": name,
                "frame": "",
            }
            if elapsed >= next_frame:
                source = latest_frame(user_dir / "Dump" / "Frames")
                if source:
                    dest = frames_out / f"frame-{index:04d}-{source.stem}.png"
                    shutil.copy2(source, dest)
                    row["frame"] = dest.name
                next_frame += 0.25
            samples.append(row)
            index += 1
            time.sleep(max(0, start + index * interval - time.monotonic()))

    with (out / "samples.csv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=["elapsed_s", "pointer", "object_base", "derived_slot",
                                                  "derived_actor", "chain_matches_literal", "actor_slot",
                                                  "actor_name", "frame"])
        writer.writeheader()
        writer.writerows(samples)
    (out / "metadata.json").write_text(json.dumps({
        "label": label,
        "button": button,
        "stick_x": stick_x,
        "duration_s": duration,
        "sampling_interval_s": interval,
        "savestate": state_id,
        "dolphin_build": "2603a",
        "candidate_pointer_slot": f"0x{POINTER_SLOT:08X}",
        "static_root_slot": f"0x{ROOT_SLOT:08X}",
        "actor_offset": f"0x{ACTOR_OFFSET:X}",
        "roster_manager": f"0x{manager:08X}",
        "roster_base": f"0x{base:08X}",
        "actors_per_team": count,
        "source_frame_warning": "Nearest dumped frame may lag the memory sample; inspect transition visually.",
    }, indent=2) + "\n")
    shutil.rmtree(user_dir)
    print(out, "samples", len(samples), "frames", len(list(frames_out.iterdir())), flush=True)
    return out


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("label")
    parser.add_argument("--press-a", action="store_true")
    parser.add_argument("--button", choices=("A", "B", "X", "Y"))
    parser.add_argument("--state", default="19bb4a7c")
    parser.add_argument("--stick-x", type=float, default=0.5)
    parser.add_argument("--duration", type=float, default=12)
    args = parser.parse_args()
    capture(args.label, state_id=args.state, button=args.button or ("A" if args.press_a else None),
            stick_x=args.stick_x, duration=args.duration)
