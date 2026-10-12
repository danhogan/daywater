"""Trace GNNE69 target-output handoffs into the actor input function.

The raw trace contains game RAM and belongs in private storage outside Git.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
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
from trace_score_path import verified_iso  # noqa: E402
from trace_thomas_position_writer import thomas_info  # noqa: E402

CHOICE_CALL_PC = 0x8008D99C
CHOICE_RETURN_PC = 0x8008D9A0
RECORD_RETURN_PC = 0x800BE400
LOOKUP_CALL_PC = 0x800BE39C
LOOKUP_RETURN_PC = 0x800BE3A0
RESOLVER_RETURN_PC = 0x800BE404
SELECTOR_PC = 0x8008D764
SELECTOR_RETURN_PC = 0x8008DA64
INPUT_PC = 0x8008D864
INPUT_RETURN_PC = 0x8008DA70
ACTOR_STRIDE = 0x14D4


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", required=True, type=Path)
    parser.add_argument("--savestate", required=True, type=Path)
    parser.add_argument("--dolphin-binary", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--actor-slot", type=int, default=7)
    parser.add_argument("--stick-x", type=float, default=1.0)
    parser.add_argument("--stick-y", type=float, default=0.5)
    parser.add_argument("--snap", action="store_true",
                        help="press A in the first-down preplay state")
    parser.add_argument("--seconds", type=float, default=45)
    parser.add_argument("--captures", type=int, default=1,
                        help="number of consecutive actor-specific handoffs to capture")
    parser.add_argument("--trace-choice", action="store_true",
                        help="also capture the immediate other-actor selection call")
    parser.add_argument("--trace-choice-internals", action="store_true",
                        help="capture record and handle lookup boundaries")
    parser.add_argument("--gdb-port", type=int, default=2468)
    args = parser.parse_args()
    if (args.output.exists() or not 0 <= args.actor_slot < 40 or
            not 0 <= args.stick_x <= 1 or not 0 <= args.stick_y <= 1 or
            args.seconds <= 0 or not 1 <= args.captures <= 8):
        parser.error("invalid output, actor slot, stick, or duration")
    if args.trace_choice_internals and not args.trace_choice:
        parser.error("trace-choice-internals requires trace-choice")
    version = subprocess.run([str(args.dolphin_binary), "--version"],
                             capture_output=True, text=True, timeout=10,
                             env={**os.environ, "QT_QPA_PLATFORM": "offscreen"})
    if version.returncode != 0 or "2603a" not in version.stdout + version.stderr:
        parser.error("this GNNE69 savestate requires Dolphin 2603a")
    verified_iso(args.iso)
    runner._find_linux_dolphin = lambda: str(args.dolphin_binary)
    session_module._find_linux_dolphin = lambda: str(args.dolphin_binary)

    with args.output.open("x", encoding="utf-8") as output:
        def emit(**row: object) -> None:
            output.write(json.dumps(row, sort_keys=True) + "\n")
            output.flush()

        with tempfile.TemporaryDirectory(prefix="nflstreet-target-source-") as root:
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
                if args.actor_slot >= 2 * roster_count:
                    raise RuntimeError("actor slot outside live roster")
                actor = actor_base + args.actor_slot * ACTOR_STRIDE
                name = session.read_bytes(actor + 0xB80, 16).split(b"\0", 1)[0]
                emit(event="ready", actor=f"0x{actor:08X}", actor_slot=args.actor_slot,
                     actor_name_hex=name.hex(), thomas=f"0x{thomas:08X}",
                     roster_base=f"0x{actor_base:08X}", roster_count=roster_count,
                     savestate_sha256=hashlib.sha256(args.savestate.read_bytes()).hexdigest())
                gdb = session._gdb
                assert gdb is not None
                gdb.interrupt()
                gdb.read_pc()
                gdb.continue_execution()
                if not args.snap:
                    session.play_sequence_async(InputSequence([
                        InputCommand(0, f"SET MAIN {args.stick_x} {args.stick_y}"),
                        InputCommand(args.seconds, "SET MAIN 0.5 0.5"),
                    ]))
                time.sleep(2)
                gdb.interrupt()
                gdb.read_pc()
                gdb.set_execution_breakpoint(
                    CHOICE_CALL_PC if args.trace_choice else SELECTOR_PC)
                gdb.continue_execution()
                if args.snap:
                    session.play_sequence_async(InputSequence([
                        InputCommand(0, "PRESS A"),
                        InputCommand(0.15, "RELEASE A"),
                        InputCommand(0.2, f"SET MAIN {args.stick_x} {args.stick_y}"),
                        InputCommand(args.seconds, "SET MAIN 0.5 0.5"),
                    ]))
                phase = "choice" if args.trace_choice else "selector"
                output_pointer = None
                selected_other_pointer = None
                selected_other_in_roster = False
                choice_pointer = None
                choice_record = None
                lookup_pointer = None
                started = time.monotonic()
                other_hits = 0
                unrelated_stops = 0
                captured = 0
                while time.monotonic() - started < args.seconds:
                    try:
                        gdb.wait_for_stop(0.5)
                        pc = gdb.read_pc()
                    except GDBError:
                        continue
                    if phase == "choice" and pc == CHOICE_CALL_PC:
                        candidate = gdb.read_register(3)
                        gdb.remove_execution_breakpoint(CHOICE_CALL_PC)
                        if candidate != actor:
                            other_hits += 1
                            gdb.step_instruction()
                            gdb.set_execution_breakpoint(CHOICE_CALL_PC)
                            gdb.continue_execution()
                            continue
                        emit(event="choice_call", pc=f"0x{pc:08X}",
                             actor=f"0x{actor:08X}")
                        if args.trace_choice_internals:
                            gdb.set_execution_breakpoint(RECORD_RETURN_PC)
                            phase = "record_return"
                        else:
                            gdb.set_execution_breakpoint(CHOICE_RETURN_PC)
                            phase = "choice_return"
                    elif phase == "record_return" and pc == RECORD_RETURN_PC:
                        gdb.remove_execution_breakpoint(RECORD_RETURN_PC)
                        choice_record = gdb.read_register(3)
                        emit(event="record_return", pc=f"0x{pc:08X}",
                             record_pointer=f"0x{choice_record:08X}")
                        gdb.set_execution_breakpoint(LOOKUP_CALL_PC)
                        phase = "lookup_call"
                    elif phase == "lookup_call" and pc == LOOKUP_CALL_PC:
                        gdb.remove_execution_breakpoint(LOOKUP_CALL_PC)
                        handle_pointer = gdb.read_register(3)
                        if choice_record is None or handle_pointer != choice_record + 0xB4:
                            raise RuntimeError("handle argument does not follow record +0xB4")
                        emit(event="lookup_call", pc=f"0x{pc:08X}",
                             handle_pointer=f"0x{handle_pointer:08X}",
                             handle=f"0x{session.read_u32(handle_pointer):08X}")
                        gdb.set_execution_breakpoint(LOOKUP_RETURN_PC)
                        phase = "lookup_return"
                    elif phase == "lookup_return" and pc == LOOKUP_RETURN_PC:
                        gdb.remove_execution_breakpoint(LOOKUP_RETURN_PC)
                        lookup_pointer = gdb.read_register(3)
                        emit(event="lookup_return", pc=f"0x{pc:08X}",
                             other_pointer=f"0x{lookup_pointer:08X}")
                        gdb.set_execution_breakpoint(RESOLVER_RETURN_PC)
                        phase = "resolver_return"
                    elif phase == "resolver_return" and pc == RESOLVER_RETURN_PC:
                        gdb.remove_execution_breakpoint(RESOLVER_RETURN_PC)
                        resolved_pointer = gdb.read_register(3)
                        if resolved_pointer != lookup_pointer:
                            raise RuntimeError("resolver changed lookup result")
                        emit(event="resolver_return", pc=f"0x{pc:08X}",
                             other_pointer=f"0x{resolved_pointer:08X}")
                        gdb.set_execution_breakpoint(CHOICE_RETURN_PC)
                        phase = "choice_return"
                    elif phase == "choice_return" and pc == CHOICE_RETURN_PC:
                        gdb.remove_execution_breakpoint(CHOICE_RETURN_PC)
                        choice_pointer = gdb.read_register(3)
                        if (args.trace_choice_internals and
                                choice_pointer != lookup_pointer):
                            raise RuntimeError("choice changed lookup result")
                        emit(event="choice_return", pc=f"0x{pc:08X}",
                             other_pointer=f"0x{choice_pointer:08X}")
                        gdb.set_execution_breakpoint(SELECTOR_PC)
                        phase = "selector"
                    elif phase == "selector" and pc == SELECTOR_PC:
                        destination = gdb.read_register(3)
                        gdb.remove_execution_breakpoint(SELECTOR_PC)
                        if destination != actor:
                            other_hits += 1
                            gdb.step_instruction()
                            gdb.set_execution_breakpoint(SELECTOR_PC)
                            gdb.continue_execution()
                            continue
                        other_pointer = gdb.read_register(4)
                        if args.trace_choice and other_pointer != choice_pointer:
                            raise RuntimeError("choice return does not feed selector r4")
                        small_data = gdb.read_register(13)
                        selection_manager = session.read_u32(small_data - 0x73D0)
                        selection_index = session.read_bytes(selection_manager + 1, 1)[0]
                        selection_record = selection_manager + 8 + selection_index * 0xF4
                        selection_handle = session.read_u32(selection_record + 0xB4)
                        source_vector_pointer = gdb.read_register(5)
                        selected_other_pointer = other_pointer
                        output_pointer = gdb.read_register(6)
                        candidate_slot = ((other_pointer - actor_base) // ACTOR_STRIDE
                                          if other_pointer >= actor_base and
                                          (other_pointer - actor_base) % ACTOR_STRIDE == 0
                                          else None)
                        in_roster = candidate_slot is not None and candidate_slot < 2 * roster_count
                        selected_other_in_roster = in_roster
                        other_name = (session.read_bytes(other_pointer + 0xB80, 16).split(b"\0", 1)[0]
                                      if in_roster else b"")
                        other_position = (session.read_bytes(other_pointer + 0x1A8, 12).hex()
                                          if in_roster else None)
                        emit(event="selector_entry", pc=f"0x{pc:08X}",
                             return_pc=f"0x{gdb.read_register(0x43):08X}",
                             actor=f"0x{actor:08X}", other_pointer=f"0x{other_pointer:08X}",
                             other_slot=candidate_slot if in_roster else None,
                             other_name_hex=other_name.hex(),
                             other_position_hex=other_position,
                             small_data=f"0x{small_data:08X}",
                             selection_manager=f"0x{selection_manager:08X}",
                             selection_index=selection_index,
                             selection_record=f"0x{selection_record:08X}",
                             selection_handle=f"0x{selection_handle:08X}",
                             actor_position_hex=session.read_bytes(actor + 0x1A8, 12).hex(),
                             source_vector_hex=session.read_bytes(source_vector_pointer, 8).hex(),
                             output_pointer=f"0x{output_pointer:08X}",
                             r5=f"0x{source_vector_pointer:08X}",
                             other_hits=other_hits)
                        gdb.set_execution_breakpoint(SELECTOR_RETURN_PC)
                        phase = "selector_return"
                    elif phase == "selector_return" and pc == SELECTOR_RETURN_PC:
                        gdb.remove_execution_breakpoint(SELECTOR_RETURN_PC)
                        assert output_pointer is not None
                        assert selected_other_pointer is not None
                        emit(event="selector_return", pc=f"0x{pc:08X}",
                             output_pointer=f"0x{output_pointer:08X}",
                             output_hex=session.read_bytes(output_pointer, 12).hex(),
                             other_position_hex=(session.read_bytes(
                                 selected_other_pointer + 0x1A8, 12).hex()
                                 if selected_other_in_roster else None))
                        gdb.set_execution_breakpoint(INPUT_PC)
                        phase = "input"
                    elif phase == "input" and pc == INPUT_PC:
                        gdb.remove_execution_breakpoint(INPUT_PC)
                        if gdb.read_register(3) != actor:
                            gdb.step_instruction()
                            gdb.set_execution_breakpoint(INPUT_PC)
                            gdb.continue_execution()
                            continue
                        pointer = gdb.read_register(4)
                        emit(event="input_entry", pc=f"0x{pc:08X}",
                             return_pc=f"0x{gdb.read_register(0x43):08X}",
                             input_pointer=f"0x{pointer:08X}",
                             input_hex=session.read_bytes(pointer, 12).hex())
                        captured += 1
                        if captured == args.captures:
                            emit(event="done", captured=captured,
                                 requested=args.captures)
                            return
                        gdb.set_execution_breakpoint(
                            CHOICE_CALL_PC if args.trace_choice else SELECTOR_PC)
                        phase = "choice" if args.trace_choice else "selector"
                    else:
                        # Dolphin can report an unrelated stop while waiting for
                        # the first actor-specific breakpoint. Keep this bounded
                        # so a broken breakpoint setup cannot silently time out.
                        if phase not in ("choice", "selector") or unrelated_stops >= 8:
                            raise RuntimeError(f"unexpected GDB stop at {pc:#x} in {phase}")
                        unrelated_stops += 1
                    gdb.continue_execution()
                emit(event="done", captured=captured,
                     requested=args.captures, phase=phase)
                raise RuntimeError("no complete target-source handoff captured")


if __name__ == "__main__":
    main()
