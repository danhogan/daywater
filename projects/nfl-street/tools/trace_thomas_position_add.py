"""Trace the executed Thomas position adds/stores in the tested second-down play.

Requires the researched GNNE69 image and a compatible Dolphin 2603a state.
Keep output outside Git.
"""

from __future__ import annotations

import argparse
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
from trace_thomas_position_writer import POSITION_OFFSET, thomas_info  # noqa: E402


CALL_PC = 0x80036E08
STORE_PC = 0x801DE590


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", required=True, type=Path)
    parser.add_argument("--savestate", required=True, type=Path)
    parser.add_argument("--dolphin-binary", required=True, type=Path)
    parser.add_argument("--stick-x", type=float, default=1.0, help="0=left, 1=right")
    parser.add_argument("--stick-y", type=float, default=0.5, help="0=forward, 1=backward")
    parser.add_argument("--trace-vector", action="store_true",
                        help="step through the adjacent Y and Z stores after X")
    parser.add_argument("--wait-before-trace", type=float, default=2.0)
    parser.add_argument("--trace-duration", type=float, default=30.0)
    parser.add_argument("--output", type=Path, help="JSONL output outside Git")
    parser.add_argument("--gdb-port", type=int, default=2403)
    args = parser.parse_args()
    if (not 0 <= args.stick_x <= 1 or not 0 <= args.stick_y <= 1
            or args.wait_before_trace < 0
            or args.trace_duration <= 0 or not 0 < args.gdb_port < 65536):
        parser.error("invalid stick, timing, or GDB port")
    verified_iso(args.iso)

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
        with tempfile.TemporaryDirectory(prefix="nflstreet-thomas-add-") as root:
            with session_module.DolphinSession.start(
                args.iso, savestate=args.savestate, pipe_input=True,
                gdb_port=args.gdb_port, video_backend="OGL", user_dir=Path(root),
            ) as session:
                if not session.wait_for_first_frame(90):
                    raise RuntimeError("Dolphin produced no completed frame")
                time.sleep(2)
                actor, position = thomas_info(session)
                target = actor + POSITION_OFFSET
                emit(event="ready", actor=f"0x{actor:08X}",
                     target=f"0x{target:08X}", position=position)

                gdb = session._gdb
                assert gdb is not None
                gdb.interrupt()
                gdb.read_pc()  # Synchronize after duplicate stop notification.
                gdb.continue_execution()
                session.play_sequence_async(InputSequence([
                    InputCommand(0, f"SET MAIN {args.stick_x} {args.stick_y}"),
                    InputCommand(args.wait_before_trace + args.trace_duration,
                                 "SET MAIN 0.5 0.5"),
                ]))
                time.sleep(args.wait_before_trace)
                gdb.interrupt()
                gdb.read_pc()
                emit(event="before_trace", position=thomas_info(session)[1])
                gdb.set_execution_breakpoint(CALL_PC)
                gdb.continue_execution()
                started = time.monotonic()
                hits = 0
                matched = False
                waiting_for_store = False
                while time.monotonic() - started < args.trace_duration:
                    try:
                        reply = gdb.wait_for_stop(0.5)
                    except GDBError:
                        continue
                    pc = gdb.read_pc()
                    if pc == CALL_PC and not waiting_for_store:
                        hits += 1
                        destination = gdb.read_register(30)
                        emit(event="caller", hit=hits, pc=f"0x{pc:08X}",
                             r30=f"0x{destination:08X}",
                             is_thomas_x=destination == target,
                             position=thomas_info(session)[1])
                        gdb.remove_execution_breakpoint(CALL_PC)
                        if destination == target:
                            gdb.set_execution_breakpoint(STORE_PC)
                            waiting_for_store = True
                            gdb.continue_execution()
                        else:
                            gdb.step_instruction()
                            step_pc = gdb.read_pc()
                            if step_pc == CALL_PC:
                                raise RuntimeError("step stayed at caller")
                            gdb.set_execution_breakpoint(CALL_PC)
                            gdb.continue_execution()
                    elif pc == STORE_PC and waiting_for_store:
                        store_target = gdb.read_register(3)
                        if store_target != target:
                            raise RuntimeError("store target differed from caller")
                        before = thomas_info(session)[1]
                        gdb.remove_execution_breakpoint(STORE_PC)
                        step_reply = gdb.step_instruction()
                        step_pc = gdb.read_pc()
                        if step_pc != STORE_PC + 4:
                            raise RuntimeError(
                                f"single-step stopped at 0x{step_pc:08X}: {step_reply}")
                        after = thomas_info(session)[1]
                        emit(event="thomas_x_store", pc=f"0x{pc:08X}",
                             next_pc=f"0x{step_pc:08X}",
                             target=f"0x{target:08X}", before=before, after=after)
                        if args.trace_vector:
                            previous = after
                            for field, store_pc, next_pc in (
                                ("y", 0x801DE5A0, 0x801DE5A4),
                                ("z", 0x801DE5B0, 0x801DE5B4),
                            ):
                                while gdb.read_pc() != store_pc:
                                    if gdb.read_pc() > store_pc:
                                        raise RuntimeError(f"stepped past {field} store")
                                    gdb.step_instruction()
                                if gdb.read_register(3) != target:
                                    raise RuntimeError(f"{field} store base changed")
                                gdb.step_instruction()
                                if gdb.read_pc() != next_pc:
                                    raise RuntimeError(f"{field} store step missed")
                                current = thomas_info(session)[1]
                                emit(event=f"thomas_{field}_store", pc=f"0x{store_pc:08X}",
                                     next_pc=f"0x{next_pc:08X}",
                                     target=f"0x{target + (4 if field == 'y' else 8):08X}",
                                     before=previous, after=current)
                                previous = current
                        matched = True
                        break
                    else:
                        emit(event="other_stop", pc=f"0x{pc:08X}", reply=reply)
                        gdb.continue_execution()
                emit(event="done", hits=hits, matched=matched,
                     position=thomas_info(session)[1])
    finally:
        if output:
            output.close()


if __name__ == "__main__":
    main()
