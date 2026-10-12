"""Capture GNNE69 actor preparation order by dumped frame in a live match.

Write raw hits only to private storage. Actor names and addresses are evidence,
not an established general roster contract.
"""

from __future__ import annotations

import argparse
import hashlib
import json
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
from trace_score_path import verified_iso  # noqa: E402
from trace_thomas_position_writer import thomas_info  # noqa: E402

PREPARATION_PC = 0x800F705C
ACTOR_STRIDE = 0x14D4


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", required=True, type=Path)
    parser.add_argument("--savestate", required=True, type=Path)
    parser.add_argument("--dolphin-binary", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--gdb-port", type=int, default=2446)
    parser.add_argument("--max-hits", type=int, default=120)
    parser.add_argument("--duration", type=float, default=45.0)
    parser.add_argument("--stick-x", type=float, default=1.0)
    parser.add_argument("--stick-y", type=float, default=0.5)
    args = parser.parse_args()
    if (args.output.exists() or not 1 <= args.max_hits <= 1000 or args.duration <= 0 or
            not 0 <= args.stick_x <= 1 or not 0 <= args.stick_y <= 1):
        parser.error("invalid output, hit count, or duration")
    verified_iso(args.iso)
    runner._find_linux_dolphin = lambda: str(args.dolphin_binary)
    session_module._find_linux_dolphin = lambda: str(args.dolphin_binary)

    with args.output.open("x", encoding="utf-8") as output:
        def emit(**row: object) -> None:
            output.write(json.dumps(row, sort_keys=True) + "\n")
            output.flush()

        with tempfile.TemporaryDirectory(prefix="nflstreet-preparation-order-") as root:
            with session_module.DolphinSession.start(
                args.iso, savestate=args.savestate, pipe_input=True,
                gdb_port=args.gdb_port, video_backend="OGL", user_dir=Path(root),
            ) as session:
                if not session.wait_for_first_frame(90):
                    raise RuntimeError("Dolphin produced no completed frame")
                time.sleep(2)
                thomas, _ = thomas_info(session)
                manager = session.read_u32(0x803A1E10)
                actor_base = session.read_u32(manager)
                roster_count = int.from_bytes(session.read_bytes(manager + 8, 2), "big")
                emit(event="ready", thomas=f"0x{thomas:08X}",
                     roster_base=f"0x{actor_base:08X}", roster_count=roster_count,
                     stick=[args.stick_x, args.stick_y],
                     savestate_sha256=hashlib.sha256(args.savestate.read_bytes()).hexdigest())
                gdb = session._gdb
                assert gdb is not None
                gdb.interrupt()
                gdb.read_pc()
                gdb.continue_execution()
                session.play_sequence_async(InputSequence([
                    InputCommand(0, f"SET MAIN {args.stick_x} {args.stick_y}"),
                    InputCommand(args.duration + 3, "SET MAIN 0.5 0.5"),
                ]))
                time.sleep(2)
                gdb.interrupt()
                gdb.read_pc()
                gdb.set_execution_breakpoint(PREPARATION_PC)
                gdb.continue_execution()
                started = time.monotonic()
                hits = 0
                frames_dir = session.user_dir / "Dump" / "Frames"
                while hits < args.max_hits and time.monotonic() - started < args.duration:
                    try:
                        gdb.wait_for_stop(0.5)
                        pc = gdb.read_pc()
                    except GDBError:
                        continue
                    if pc != PREPARATION_PC:
                        emit(event="other_stop", pc=f"0x{pc:08X}")
                        gdb.continue_execution()
                        continue
                    actor = gdb.read_register(3)
                    frame_files = list(frames_dir.glob("*.png"))
                    latest = max(frame_files, key=lambda path: int(path.stem.rsplit("_", 1)[-1]))
                    slot = ((actor - actor_base) // ACTOR_STRIDE
                            if actor >= actor_base and (actor - actor_base) % ACTOR_STRIDE == 0
                            else None)
                    in_roster = slot is not None and 0 <= slot < 2 * roster_count
                    name = (session.read_bytes(actor + 0xB80, 16).split(b"\0", 1)[0]
                            if in_roster else b"")
                    emit(event="preparation", hit=hits, pc=f"0x{pc:08X}",
                         actor=f"0x{actor:08X}", slot=slot if in_roster else None,
                         name_hex=name.hex(), is_thomas=actor == thomas,
                         lr=f"0x{gdb.read_register(0x43):08X}",
                         dumped_frames=len(frame_files), latest_frame=latest.name)
                    hits += 1
                    gdb.remove_execution_breakpoint(PREPARATION_PC)
                    gdb.step_instruction()
                    gdb.set_execution_breakpoint(PREPARATION_PC)
                    gdb.continue_execution()
                emit(event="done", hits=hits, max_hits=args.max_hits)


if __name__ == "__main__":
    main()
