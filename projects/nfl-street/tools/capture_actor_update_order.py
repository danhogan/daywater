"""Capture the order of actor preparation calls in a bounded GNNE69 run.

This is an integration probe. Keep output outside Git; actor names and pointers
are observations from the running game, not inferred roles or update policy.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
import tempfile
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
sys.path.insert(0, str(Path(__file__).resolve().parent))

import src.core.dolphin.runner as runner  # noqa: E402
import src.core.dolphin.session as session_module  # noqa: E402
from src.core.dolphin.debugger import GDBError  # noqa: E402
from src.core.dolphin.input import InputCommand, InputSequence  # noqa: E402
from src.core.dolphin.memory import DolphinMemoryError  # noqa: E402
from trace_score_path import verified_iso  # noqa: E402
from trace_thomas_position_writer import thomas_info  # noqa: E402

PREPARATION_PC = 0x800F705C


def current_frame(directory: Path) -> int | None:
    numbers = []
    for path in directory.glob("framedump_*.png"):
        try:
            numbers.append(int(path.stem.rsplit("_", 1)[1]))
        except (IndexError, ValueError):
            continue
    return max(numbers, default=None)


def wait_for_pc(gdb) -> int:
    for attempt in range(8):
        try:
            return gdb.read_pc()
        except GDBError:
            if attempt == 7:
                raise
            time.sleep(0.1)
    raise AssertionError("unreachable")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", required=True, type=Path)
    parser.add_argument("--savestate", required=True, type=Path)
    parser.add_argument("--dolphin-binary", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--frames", type=int, default=3)
    parser.add_argument("--max-hits", type=int, default=150)
    parser.add_argument("--seconds", type=float, default=45)
    parser.add_argument("--gdb-port", type=int, default=2460)
    args = parser.parse_args()
    if not 2 <= args.frames <= 8 or args.max_hits < 1 or args.seconds <= 0:
        parser.error("invalid frame, hit, or time bound")
    if args.output.exists():
        parser.error("output already exists")
    version = subprocess.run(
        [str(args.dolphin_binary), "--version"], capture_output=True,
        text=True, timeout=10,
    )
    if version.returncode != 0 or "2603a" not in version.stdout + version.stderr:
        parser.error("this GNNE69 savestate requires Dolphin 2603a")
    verified_iso(args.iso)
    runner._find_linux_dolphin = lambda: str(args.dolphin_binary)
    session_module._find_linux_dolphin = lambda: str(args.dolphin_binary)

    with args.output.open("x", encoding="utf-8") as output:
        with tempfile.TemporaryDirectory(prefix="nflstreet-actor-order-") as root:
            with session_module.DolphinSession.start(
                args.iso, savestate=args.savestate, pipe_input=True,
                gdb_port=args.gdb_port, video_backend="OGL", user_dir=Path(root),
            ) as session:
                if not session.wait_for_first_frame(90):
                    raise RuntimeError("Dolphin produced no frame")
                time.sleep(2)
                thomas, _ = thomas_info(session)
                gdb = session._gdb
                assert gdb is not None
                frames_dir = session.user_dir / "Dump" / "Frames"

                def emit(**event: object) -> None:
                    line = json.dumps(event, sort_keys=True)
                    output.write(line + "\n")
                    output.flush()

                emit(event="ready", thomas=f"0x{thomas:08X}",
                     savestate_sha256=hashlib.sha256(args.savestate.read_bytes()).hexdigest())
                gdb.interrupt()
                wait_for_pc(gdb)
                gdb.continue_execution()
                session.play_sequence_async(InputSequence([
                    InputCommand(0, "SET MAIN 1 0.5"),
                    InputCommand(args.seconds, "SET MAIN 0.5 0.5"),
                ]))
                gdb.interrupt()
                wait_for_pc(gdb)
                gdb.set_execution_breakpoint(PREPARATION_PC)
                gdb.continue_execution()

                first_frame = None
                last_frame = None
                hits = 0
                started = time.monotonic()
                while hits < args.max_hits and time.monotonic() - started < args.seconds:
                    try:
                        gdb.wait_for_stop(0.5)
                        pc = gdb.read_pc()
                    except GDBError:
                        continue
                    if pc != PREPARATION_PC:
                        raise RuntimeError(f"unexpected GDB stop at {pc:#x}")
                    actor = gdb.read_register(3)
                    frame = current_frame(frames_dir)
                    if frame is not None:
                        if first_frame is None:
                            first_frame = frame
                        if frame >= first_frame + args.frames:
                            break
                    name_hex = None
                    if 0x80000000 <= actor < 0x81800000:
                        try:
                            name_hex = session.read_bytes(actor + 0xB80, 16).hex()
                        except DolphinMemoryError:  # bad pointer is an observation
                            pass
                    emit(event="preparation", index=hits, pc=f"0x{pc:08X}",
                         actor=f"0x{actor:08X}", name_hex=name_hex,
                         is_thomas=actor == thomas, dumped_frame=frame,
                         lr=f"0x{gdb.read_register(0x43):08X}")
                    hits += 1
                    last_frame = frame
                    gdb.remove_execution_breakpoint(PREPARATION_PC)
                    gdb.step_instruction()
                    gdb.set_execution_breakpoint(PREPARATION_PC)
                    gdb.continue_execution()
                emit(event="done", hits=hits, first_frame=first_frame,
                     last_frame=last_frame, requested_frames=args.frames)


if __name__ == "__main__":
    main()
