"""Trace one executed Thomas X or Y store in a controlled GNNE69 play.

Use a compatible Dolphin 2603a savestate. Output belongs outside Git.
"""

from __future__ import annotations

import argparse
import json
import struct
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


ROSTER_ROOT = 0x803A1E10
ACTOR_STRIDE = 0x14D4
POSITION_OFFSET = 0x1A8
WRITER_PCS = {"x": 0x80068730, "y": 0x80068748}


def thomas_info(session: session_module.DolphinSession) -> tuple[int, tuple[float, float, float]]:
    manager = session.read_u32(ROSTER_ROOT)
    actors = session.read_u32(manager)
    count = int.from_bytes(session.read_bytes(manager + 8, 2), "big")
    if not actors or not 1 <= count <= 20:
        raise RuntimeError("invalid roster manager")
    actor = actors + (count + 1) * ACTOR_STRIDE  # observed team 1, slot 1
    name = session.read_bytes(actor + 0xB80, 16).split(b"\0", 1)[0]
    if name != b"Thomas":
        raise RuntimeError(f"expected Thomas at roster slot 1, got {name!r}")
    position = struct.unpack(">fff", session.read_bytes(actor + POSITION_OFFSET, 12))
    return actor, position


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", required=True, type=Path)
    parser.add_argument("--savestate", required=True, type=Path)
    parser.add_argument("--dolphin-binary", required=True, type=Path)
    parser.add_argument("--snap", action="store_true", help="press A before stick input")
    parser.add_argument("--axis", choices=("x", "y"), default="x")
    parser.add_argument("--stick-x", type=float, default=0.0, help="0=left, 1=right")
    parser.add_argument("--stick-y", type=float, default=0.5, help="0=forward, 1=backward")
    parser.add_argument("--wait-before-trace", type=float, default=6.0)
    parser.add_argument("--trace-duration", type=float, default=35.0)
    parser.add_argument("--output", type=Path, help="JSONL output outside Git")
    parser.add_argument("--gdb-port", type=int, default=2402)
    args = parser.parse_args()
    if (not 0 <= args.stick_x <= 1 or not 0 <= args.stick_y <= 1
            or args.wait_before_trace < 0
            or args.trace_duration <= 0 or not 0 < args.gdb_port < 65536):
        parser.error("invalid stick, timing, or GDB port")
    verified_iso(args.iso)
    writer_pc = WRITER_PCS[args.axis]

    binary = str(args.dolphin_binary)
    runner._find_linux_dolphin = lambda: binary
    session_module._find_linux_dolphin = lambda: binary
    output = args.output.open("w", encoding="utf-8") if args.output else None

    def emit(**event: object) -> None:
        line = json.dumps(event, sort_keys=True)
        print(line, flush=True)
        if output:
            output.write(line + "\n")
            output.flush()

    try:
        with tempfile.TemporaryDirectory(prefix="nflstreet-thomas-writer-") as root:
            with session_module.DolphinSession.start(
                args.iso, savestate=args.savestate, pipe_input=True,
                gdb_port=args.gdb_port, video_backend="OGL", user_dir=Path(root),
            ) as session:
                if not session.wait_for_first_frame(90):
                    raise RuntimeError("Dolphin produced no completed frame")
                time.sleep(2)
                actor, position = thomas_info(session)
                emit(event="ready", actor=f"0x{actor:08X}", position=position)

                gdb = session._gdb
                assert gdb is not None
                gdb.interrupt()
                gdb.read_pc()  # Synchronize after Dolphin's duplicate stop notification.
                gdb.continue_execution()
                if args.snap:
                    session.play_sequence(InputSequence([
                        InputCommand(0, "PRESS A"), InputCommand(0.15, "RELEASE A"),
                    ]))
                session.play_sequence_async(InputSequence([
                    InputCommand(0, f"SET MAIN {args.stick_x} {args.stick_y}"),
                    InputCommand(args.wait_before_trace + args.trace_duration,
                                 "SET MAIN 0.5 0.5"),
                ]))
                time.sleep(args.wait_before_trace)
                gdb.interrupt()
                gdb.read_pc()
                emit(event="before_trace", position=thomas_info(session)[1])
                gdb.set_execution_breakpoint(writer_pc)
                gdb.continue_execution()
                started = time.monotonic()
                hits = 0
                matched = False
                while time.monotonic() - started < args.trace_duration:
                    try:
                        reply = gdb.wait_for_stop(0.5)
                    except GDBError:
                        continue
                    pc = gdb.read_pc()
                    if pc != writer_pc:
                        emit(event="other_stop", pc=f"0x{pc:08X}", reply=reply)
                        gdb.continue_execution()
                        continue
                    hits += 1
                    base = gdb.read_register(27 if args.axis == "x" else 26)
                    target = base if args.axis == "x" else base + 4
                    expected = actor if args.axis == "x" else actor + POSITION_OFFSET + 4
                    before = thomas_info(session)[1]
                    emit(event="writer_entry", hit=hits, pc=f"0x{pc:08X}",
                         target=f"0x{target:08X}", is_thomas=target == expected,
                         position=before)
                    gdb.remove_execution_breakpoint(writer_pc)
                    step_reply = gdb.step_instruction()
                    step_pc = gdb.read_pc()
                    if step_pc != writer_pc + 4:
                        raise RuntimeError(f"single-step stopped at 0x{step_pc:08X}: {step_reply}")
                    if target == expected:
                        after = thomas_info(session)[1]
                        emit(event=f"thomas_{args.axis}_store", pc=f"0x{pc:08X}",
                             next_pc=f"0x{step_pc:08X}", actor=f"0x{actor:08X}",
                             before=before, after=after)
                        matched = True
                        break
                    gdb.set_execution_breakpoint(writer_pc)
                    gdb.continue_execution()
                emit(event="done", hits=hits, matched=matched,
                     position=thomas_info(session)[1])
    finally:
        if output:
            output.close()


if __name__ == "__main__":
    main()
