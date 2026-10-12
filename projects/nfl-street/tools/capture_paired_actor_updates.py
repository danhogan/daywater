"""Capture consecutive Stewart/Thomas movement pairs from a verified match.

Raw actor blocks and positions belong on private storage outside Git.
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
from trace_thomas_position_writer import ACTOR_STRIDE, POSITION_OFFSET, thomas_info  # noqa: E402

PREPARATION_PC = 0x800F705C
MOVEMENT_PC = 0x80036B28
MOVEMENT_EXIT_PC = 0x80036E58
SOURCE_PC = 0x8008D864


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", required=True, type=Path)
    parser.add_argument("--savestate", required=True, type=Path)
    parser.add_argument("--dolphin-binary", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--first-mem1", type=Path,
                        help="private 24 MiB MEM1 image at first Stewart preparation")
    parser.add_argument("--pairs", type=int, default=3)
    parser.add_argument("--duration", type=float, default=70.0)
    parser.add_argument("--gdb-port", type=int, default=2470)
    parser.add_argument("--stick-x", type=float, default=1.0)
    parser.add_argument("--stick-y", type=float, default=0.5)
    args = parser.parse_args()
    if (args.output.exists() or (args.first_mem1 and args.first_mem1.exists()) or
            not 2 <= args.pairs <= 8 or args.duration <= 0 or
            not 0 <= args.stick_x <= 1 or not 0 <= args.stick_y <= 1):
        parser.error("invalid output, pair count, or duration")
    verified_iso(args.iso)
    runner._find_linux_dolphin = lambda: str(args.dolphin_binary)
    session_module._find_linux_dolphin = lambda: str(args.dolphin_binary)

    with args.output.open("x", encoding="utf-8") as output:
        def emit(**row: object) -> None:
            output.write(json.dumps(row, sort_keys=True) + "\n")
            output.flush()

        with tempfile.TemporaryDirectory(prefix="nflstreet-paired-actors-") as root:
            with session_module.DolphinSession.start(
                args.iso, savestate=args.savestate, pipe_input=True,
                gdb_port=args.gdb_port, video_backend="OGL", user_dir=Path(root),
            ) as session:
                if not session.wait_for_first_frame(90):
                    raise RuntimeError("Dolphin produced no completed frame")
                time.sleep(2)
                thomas, _ = thomas_info(session)
                manager = session.read_u32(0x803A1E10)
                actors = session.read_u32(manager)
                count = int.from_bytes(session.read_bytes(manager + 8, 2), "big")
                if count != 7 or thomas != actors + 8 * ACTOR_STRIDE:
                    raise RuntimeError("unexpected saved-match roster layout")
                stewart = actors + 7 * ACTOR_STRIDE
                if session.read_bytes(stewart + 0xB80, 16).split(b"\0", 1)[0] != b"Stewart":
                    raise RuntimeError("expected Stewart at observed slot 7")
                emit(event="ready", actors={"7": f"0x{stewart:08X}",
                                            "8": f"0x{thomas:08X}"},
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
                frames_dir = session.user_dir / "Dump" / "Frames"

                def latest_frame() -> int:
                    files = frames_dir.glob("*.png")
                    return max(int(path.stem.rsplit("_", 1)[-1]) for path in files)

                pair = 0
                expected_actor = stewart
                active_actor = None
                phase = "preparation"
                source_armed = False
                started = time.monotonic()
                other_stops = 0
                while pair < args.pairs and time.monotonic() - started < args.duration:
                    try:
                        gdb.wait_for_stop(0.5)
                        pc = gdb.read_pc()
                    except GDBError:
                        continue
                    if source_armed and pc == SOURCE_PC:
                        destination = gdb.read_register(3)
                        gdb.remove_execution_breakpoint(SOURCE_PC)
                        source_armed = False
                        if destination == stewart:
                            other = gdb.read_register(29)
                            offset_pointer = gdb.read_register(28) + 4
                            input_pointer = gdb.read_register(4)
                            emit(event="target_source", pair=pair,
                                 pc=f"0x{pc:08X}", actor=f"0x{stewart:08X}",
                                 selected_actor=f"0x{other:08X}",
                                 selected_position_hex=session.read_bytes(
                                     other + POSITION_OFFSET, 12).hex()
                                 if other == thomas else None,
                                 offset_hex=session.read_bytes(offset_pointer, 8).hex(),
                                 target_hex=session.read_bytes(input_pointer, 12).hex(),
                                 frame=latest_frame())
                        else:
                            gdb.step_instruction()
                            gdb.set_execution_breakpoint(SOURCE_PC)
                            source_armed = True
                            gdb.continue_execution()
                            continue
                    elif phase == "preparation" and pc == PREPARATION_PC:
                        destination = gdb.read_register(3)
                        gdb.remove_execution_breakpoint(PREPARATION_PC)
                        if destination != expected_actor:
                            gdb.step_instruction()
                            gdb.set_execution_breakpoint(PREPARATION_PC)
                            gdb.continue_execution()
                            continue
                        if source_armed:
                            gdb.remove_execution_breakpoint(SOURCE_PC)
                            source_armed = False
                        active_actor = destination
                        slot = 7 if destination == stewart else 8
                        target = destination + POSITION_OFFSET
                        if pair == 0 and slot == 7 and args.first_mem1:
                            memory = session.read_bytes(0x80000000, 0x1800000)
                            args.first_mem1.write_bytes(memory)
                            emit(event="mem1_snapshot", pair=0, slot=7,
                                 sha256=hashlib.sha256(memory).hexdigest(),
                                 size=len(memory), frame=latest_frame())
                        emit(event="preparation", pair=pair, slot=slot,
                             pc=f"0x{pc:08X}", frame=latest_frame(),
                             header_hex=session.read_bytes(destination, 4).hex(),
                             rating_bb4_hex=session.read_bytes(destination + 0xBB4, 2).hex(),
                             rating_bbc_hex=session.read_bytes(destination + 0xBBC, 2).hex(),
                             actor_408_hex=session.read_bytes(destination + 0x408, 4).hex(),
                             actor_hex=session.read_bytes(destination + 0x200, 0x30).hex(),
                             state_hex=session.read_bytes(target, 0x54).hex())
                        gdb.set_execution_breakpoint(MOVEMENT_PC)
                        phase = "movement"
                    elif phase == "movement" and pc == MOVEMENT_PC:
                        target = active_actor + POSITION_OFFSET
                        gdb.remove_execution_breakpoint(MOVEMENT_PC)
                        if gdb.read_register(3) != target:
                            gdb.step_instruction()
                            gdb.set_execution_breakpoint(MOVEMENT_PC)
                            gdb.continue_execution()
                            continue
                        emit(event="movement_entry", pair=pair,
                             slot=7 if active_actor == stewart else 8,
                             pc=f"0x{pc:08X}", frame=latest_frame(),
                             state_hex=session.read_bytes(target, 0x54).hex())
                        gdb.set_execution_breakpoint(MOVEMENT_EXIT_PC)
                        phase = "exit"
                    elif phase == "exit" and pc == MOVEMENT_EXIT_PC:
                        gdb.remove_execution_breakpoint(MOVEMENT_EXIT_PC)
                        slot = 7 if active_actor == stewart else 8
                        emit(event="movement_exit", pair=pair, slot=slot,
                             pc=f"0x{pc:08X}", frame=latest_frame(),
                             state_hex=session.read_bytes(active_actor + POSITION_OFFSET,
                                                          0x54).hex())
                        if slot == 7:
                            expected_actor = thomas
                        else:
                            pair += 1
                            expected_actor = stewart
                        if pair < args.pairs:
                            gdb.set_execution_breakpoint(PREPARATION_PC)
                            if slot == 8:
                                gdb.set_execution_breakpoint(SOURCE_PC)
                                source_armed = True
                            phase = "preparation"
                    else:
                        other_stops += 1
                        if other_stops > 12:
                            raise RuntimeError(f"unexpected stop at {pc:#x} in {phase}")
                        emit(event="other_stop", pc=f"0x{pc:08X}", phase=phase)
                    if pair < args.pairs:
                        gdb.continue_execution()
                emit(event="done", captured=pair, requested=args.pairs)
                if pair != args.pairs:
                    raise RuntimeError("incomplete paired actor sequence")


if __name__ == "__main__":
    main()
