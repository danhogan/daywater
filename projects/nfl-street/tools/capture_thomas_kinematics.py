"""Capture the live second-down 0x80036B28 state transition for Thomas.

The 0x54-byte input and output records contain game RAM and belong on W:.
This records a whole kinematics call, rather than only its final vector add.
"""

from __future__ import annotations

import argparse
import hashlib
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
from trace_thomas_position_writer import POSITION_OFFSET, thomas_info  # noqa: E402


ENTRY_PC = 0x80036B28
EXIT_PC = 0x80036E58  # blr, after writes and callee-saved register restore
COMMAND_PC = 0x80036B18  # writes the movement command at +0x18/+0x34/+0x38
PREPARATION_PC = 0x800F705C  # computes acceleration and result heading before the setter
INPUT_PC = 0x8003DDEC  # candidate controller-vector-to-actor-input translation
HEADING_PC = 0x8003E0A4  # nonzero-stick store to actor +0x204
STATE_SIZE = 0x54


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
    parser.add_argument("--actor-slot", type=int,
                        help="capture this observed roster slot instead of Thomas")
    parser.add_argument("--stick-x", type=float, default=1.0)
    parser.add_argument("--stick-y", type=float, default=0.5)
    parser.add_argument("--wait-before-trace", type=float, default=2.0)
    parser.add_argument("--trace-duration", type=float, default=30.0)
    parser.add_argument("--capture-command", action="store_true",
                        help="capture the Thomas command setter immediately before kinematics")
    parser.add_argument("--capture-input", action="store_true",
                        help="capture the candidate stick translation before the command setter")
    parser.add_argument("--capture-heading-transition", action="store_true",
                        help="arm the actor heading store before changing stick input")
    parser.add_argument("--capture-preparation", action="store_true",
                        help="capture Thomas at the movement command caller entry")
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--gdb-port", type=int, default=2404)
    args = parser.parse_args()
    if not (0 <= args.stick_x <= 1 and 0 <= args.stick_y <= 1
            and args.wait_before_trace >= 0 and args.trace_duration > 0):
        parser.error("invalid stick or timing")
    if sum((args.capture_heading_transition, args.capture_input,
            args.capture_preparation)) > 1:
        parser.error("heading transition, input, and preparation are separate modes")
    if args.output.exists():
        parser.error("output already exists")
    if args.actor_slot is not None and not 0 <= args.actor_slot < 40:
        parser.error("actor slot must be within the observed roster range")
    verified_iso(args.iso)
    runner._find_linux_dolphin = lambda: str(args.dolphin_binary)
    session_module._find_linux_dolphin = lambda: str(args.dolphin_binary)

    def emit(stream, **event: object) -> None:
        line = json.dumps(event, sort_keys=True)
        print(line, flush=True)
        stream.write(line + "\n")
        stream.flush()

    with args.output.open("x", encoding="utf-8") as output:
        with tempfile.TemporaryDirectory(prefix="nflstreet-kinematics-") as root:
            with session_module.DolphinSession.start(
                args.iso, savestate=args.savestate, pipe_input=True,
                gdb_port=args.gdb_port, video_backend="OGL", user_dir=Path(root),
            ) as session:
                if not session.wait_for_first_frame(90):
                    raise RuntimeError("Dolphin produced no completed frame")
                time.sleep(2)
                thomas, position = thomas_info(session)
                actor = thomas
                if args.actor_slot is not None:
                    manager = session.read_u32(0x803A1E10)
                    actor_base = session.read_u32(manager)
                    roster_count = int.from_bytes(session.read_bytes(manager + 8, 2), "big")
                    if not 0 <= args.actor_slot < 2 * roster_count:
                        raise RuntimeError("requested actor slot is outside the live roster")
                    actor = actor_base + args.actor_slot * 0x14D4
                    position = struct.unpack(">fff", session.read_bytes(actor + POSITION_OFFSET, 12))
                actor_name = session.read_bytes(actor + 0xB80, 16).split(b"\0", 1)[0]
                target = actor + POSITION_OFFSET
                emit(output, event="ready", actor=f"0x{actor:08X}",
                     target=f"0x{target:08X}", position=position,
                     actor_slot=args.actor_slot, actor_name_hex=actor_name.hex(),
                     actor_inputs_hex=session.read_bytes(actor + 0x200, 0x30).hex(),
                     savestate_sha256=hashlib.sha256(args.savestate.read_bytes()).hexdigest(),
                     stick=[args.stick_x, args.stick_y])

                gdb = session._gdb
                assert gdb is not None
                gdb.interrupt()
                wait_for_pc(gdb)
                first_pc = (PREPARATION_PC if args.capture_preparation else
                            HEADING_PC if args.capture_heading_transition else
                            INPUT_PC if args.capture_input else
                            COMMAND_PC if args.capture_command else ENTRY_PC)
                if args.capture_heading_transition:
                    gdb.set_execution_breakpoint(first_pc)
                else:
                    gdb.continue_execution()
                session.play_sequence_async(InputSequence([
                    InputCommand(0, f"SET MAIN {args.stick_x} {args.stick_y}"),
                    InputCommand(args.wait_before_trace + args.trace_duration,
                                 "SET MAIN 0.5 0.5"),
                ]))
                if args.capture_heading_transition:
                    time.sleep(0.1)  # Allow the input pipe command while the CPU is stopped.
                else:
                    time.sleep(args.wait_before_trace)
                    gdb.interrupt()
                    wait_for_pc(gdb)
                    gdb.set_execution_breakpoint(first_pc)
                gdb.continue_execution()
                started = time.monotonic()
                hits = 0
                heading_hits = 0
                heading_captured = not args.capture_heading_transition
                input_hits = 0
                input_captured = not args.capture_input
                preparation_captured = not args.capture_preparation
                input_return_pc = None
                command_hits = 0
                command_captured = not (args.capture_command or args.capture_input
                                        or args.capture_heading_transition
                                        or args.capture_preparation)
                command_return_pc = None
                waiting_for_exit = False
                captured = False
                while time.monotonic() - started < args.trace_duration:
                    try:
                        reply = gdb.wait_for_stop(0.5)
                        pc = gdb.read_pc()
                    except GDBError:
                        continue
                    if pc == PREPARATION_PC and not preparation_captured:
                        destination = gdb.read_register(3)
                        gdb.remove_execution_breakpoint(PREPARATION_PC)
                        if destination != actor:
                            gdb.step_instruction()
                            gdb.set_execution_breakpoint(PREPARATION_PC)
                            gdb.continue_execution()
                            continue
                        emit(output, event="preparation_entry", pc=f"0x{pc:08X}",
                             actor=f"0x{actor:08X}",
                             actor_header_hex=session.read_bytes(actor, 4).hex(),
                             actor_motion_hex=session.read_bytes(actor + 0x1c0, 0x28).hex(),
                             actor_inputs_hex=session.read_bytes(actor + 0x200, 0x30).hex(),
                             actor_bb4_hex=session.read_bytes(actor + 0xbb4, 2).hex(),
                             state_hex=session.read_bytes(target, STATE_SIZE).hex())
                        preparation_captured = True
                        gdb.set_execution_breakpoint(COMMAND_PC)
                        gdb.continue_execution()
                    elif pc == HEADING_PC and not heading_captured:
                        heading_hits += 1
                        destination = gdb.read_register(31)
                        value = gdb.read_register(29)
                        before = int.from_bytes(session.read_bytes(actor + 0x204, 4), "big")
                        gdb.remove_execution_breakpoint(HEADING_PC)
                        if destination != actor or before == value:
                            gdb.step_instruction()
                            gdb.set_execution_breakpoint(HEADING_PC)
                            gdb.continue_execution()
                            continue
                        emit(output, event="heading_store_entry", hit=heading_hits,
                             pc=f"0x{pc:08X}", actor=f"0x{actor:08X}",
                             before=f"0x{before:08X}", r29=f"0x{value:08X}")
                        gdb.step_instruction()
                        next_pc = gdb.read_pc()
                        after = int.from_bytes(session.read_bytes(actor + 0x204, 4), "big")
                        emit(output, event="heading_written", pc=f"0x{next_pc:08X}",
                             actor=f"0x{actor:08X}", after=f"0x{after:08X}")
                        if after != value:
                            raise RuntimeError("heading store did not write r29")
                        heading_captured = True
                        gdb.set_execution_breakpoint(COMMAND_PC)
                        gdb.continue_execution()
                    elif pc == INPUT_PC and not input_captured:
                        input_hits += 1
                        destination = gdb.read_register(3)
                        gdb.remove_execution_breakpoint(INPUT_PC)
                        if destination != actor:
                            gdb.step_instruction()
                            gdb.set_execution_breakpoint(INPUT_PC)
                            gdb.continue_execution()
                            continue
                        input_pointer = gdb.read_register(4)
                        input_return_pc = gdb.read_register(0x43)
                        emit(output, event="input_entry", hit=input_hits,
                             pc=f"0x{pc:08X}", return_pc=f"0x{input_return_pc:08X}",
                             actor=f"0x{actor:08X}", input_pointer=f"0x{input_pointer:08X}",
                             input_hex=session.read_bytes(input_pointer, 0x20).hex(),
                             actor_inputs_hex=session.read_bytes(actor + 0x200, 0x30).hex())
                        gdb.set_execution_breakpoint(input_return_pc)
                        gdb.continue_execution()
                    elif input_return_pc is not None and pc == input_return_pc:
                        gdb.remove_execution_breakpoint(input_return_pc)
                        emit(output, event="input_written", pc=f"0x{pc:08X}",
                             actor=f"0x{actor:08X}",
                             actor_inputs_hex=session.read_bytes(actor + 0x200, 0x30).hex())
                        input_return_pc = None
                        input_captured = True
                        gdb.set_execution_breakpoint(COMMAND_PC)
                        gdb.continue_execution()
                    elif pc == COMMAND_PC and heading_captured and input_captured and not command_captured:
                        command_hits += 1
                        destination = gdb.read_register(3)
                        gdb.remove_execution_breakpoint(COMMAND_PC)
                        if destination != target:
                            gdb.step_instruction()
                            gdb.set_execution_breakpoint(COMMAND_PC)
                            gdb.continue_execution()
                            continue
                        before = session.read_bytes(target, STATE_SIZE)
                        return_pc = gdb.read_register(0x43)
                        r4 = gdb.read_register(4)
                        r5 = gdb.read_register(5)
                        emit(output, event="command_entry", hit=command_hits,
                             pc=f"0x{pc:08X}", return_pc=f"0x{return_pc:08X}",
                             address=f"0x{target:08X}", r4=f"0x{r4:08X}",
                             r5=f"0x{r5:08X}", state_hex=before.hex(),
                             actor_inputs_hex=session.read_bytes(actor + 0x200, 0x30).hex())
                        command_return_pc = return_pc
                        gdb.set_execution_breakpoint(return_pc)
                        gdb.continue_execution()
                    elif command_return_pc is not None and pc == command_return_pc:
                        gdb.remove_execution_breakpoint(command_return_pc)
                        after = session.read_bytes(target, STATE_SIZE)
                        emit(output, event="command_written", pc=f"0x{gdb.read_pc():08X}",
                             address=f"0x{target:08X}", state_hex=after.hex())
                        command_return_pc = None
                        command_captured = True
                        gdb.set_execution_breakpoint(ENTRY_PC)
                        gdb.continue_execution()
                    elif pc == ENTRY_PC and command_captured and not waiting_for_exit:
                        hits += 1
                        destination = gdb.read_register(3)
                        gdb.remove_execution_breakpoint(ENTRY_PC)
                        if destination != target:
                            gdb.step_instruction()
                            gdb.set_execution_breakpoint(ENTRY_PC)
                            gdb.continue_execution()
                            continue
                        before = session.read_bytes(target, STATE_SIZE)
                        emit(output, event="entry", hit=hits, pc=f"0x{pc:08X}",
                             address=f"0x{target:08X}", state_hex=before.hex(),
                             position=struct.unpack(">fff", session.read_bytes(target, 12)))
                        gdb.set_execution_breakpoint(EXIT_PC)
                        waiting_for_exit = True
                        gdb.continue_execution()
                    elif pc == EXIT_PC and waiting_for_exit:
                        after = session.read_bytes(target, STATE_SIZE)
                        emit(output, event="exit", pc=f"0x{pc:08X}",
                             address=f"0x{target:08X}", state_hex=after.hex(),
                             position=struct.unpack(">fff", session.read_bytes(target, 12)))
                        captured = True
                        break
                    else:
                        emit(output, event="other_stop", pc=f"0x{pc:08X}", reply=reply)
                        gdb.continue_execution()
                emit(output, event="done", hits=hits, heading_hits=heading_hits,
                     heading_captured=heading_captured, input_hits=input_hits,
                     input_captured=input_captured,
                     preparation_captured=preparation_captured,
                     command_hits=command_hits,
                     command_captured=command_captured, captured=captured)
                if not captured:
                    raise RuntimeError("no complete target-actor kinematics call captured")


if __name__ == "__main__":
    main()
