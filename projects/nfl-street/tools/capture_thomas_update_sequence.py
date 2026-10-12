"""Capture consecutive Thomas movement updates from a verified NFL Street match.

Raw actor and movement blocks contain game RAM and must be saved privately.
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


PREPARATION_PC = 0x800F705C
MOVEMENT_PC = 0x80036B28
MOVEMENT_EXIT_PC = 0x80036E58
RESET_PC = 0x800F7C90
RESET_RETURN_PC = 0x800F8710
INPUT_PC = 0x8003DDEC
ACTOR_INPUT_SOURCE_PC = 0x8008D864
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
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--stick-x", type=float, default=1.0)
    parser.add_argument("--stick-y", type=float, default=0.5)
    parser.add_argument("--wait-before-trace", type=float, default=2.0)
    parser.add_argument("--trace-duration", type=float, default=45.0)
    parser.add_argument("--updates", type=int, default=3)
    parser.add_argument("--switch-after-first", choices=("right", "forward", "backward", "left", "neutral",
                                                         "half_right", "half_forward"),
                        help="change the held stick after the first captured movement exit")
    parser.add_argument("--switch-after-second", choices=("right", "forward", "backward", "left", "neutral",
                                                          "half_right", "half_forward"),
                        help="change the held stick after the second captured movement exit")
    parser.add_argument("--watch-acceleration-reset", action="store_true",
                        help="watch the +0x34 movement field between the first two updates")
    parser.add_argument("--capture-reset-function", action="store_true",
                        help="capture 0x800F7C90 entry and return between the first two updates")
    parser.add_argument("--capture-reset-all", action="store_true",
                        help="capture 0x800F7C90 entry and return between every pair of updates")
    parser.add_argument("--capture-input-function", action="store_true",
                        help="capture Thomas input translator calls between updates")
    parser.add_argument("--capture-actor-input-source", action="store_true",
                        help="capture actor 0x8008D864 vector-to-input calls between updates")
    parser.add_argument("--watch-input-handoff", action="store_true",
                        help="step writes to actor +0x214 after the first input return")
    parser.add_argument("--watch-input-handoff-offset", type=lambda value: int(value, 0),
                        choices=(0, 0x14), default=0x14)
    parser.add_argument("--watch-actor-input-after-first", action="store_true",
                        help="trace writes to actor +0x200 between the first two updates")
    parser.add_argument("--watch-actor-input-offset", type=lambda value: int(value, 0),
                        choices=(0, 4), default=0,
                        help="offset from actor +0x200 to watch")
    parser.add_argument("--watch-actor-input-hits", type=int, default=0,
                        help="remove the actor-input watchpoint after this many hits (0 means until preparation)")
    parser.add_argument("--watch-position-after-first", action="store_true",
                        help="record Thomas X writes after the first movement exit")
    parser.add_argument("--capture-scheduler", action="store_true",
                        help="record live call returns and dumped-frame progress at each update boundary")
    parser.add_argument("--gdb-port", type=int, default=2406)
    args = parser.parse_args()
    if (not 0 <= args.stick_x <= 1 or not 0 <= args.stick_y <= 1 or
            args.wait_before_trace < 0 or args.trace_duration <= 0 or
            not 2 <= args.updates <= 8 or args.watch_actor_input_hits < 0):
        parser.error("invalid stick, timing, or update count")
    if args.watch_input_handoff and not args.capture_input_function:
        parser.error("input handoff watch requires input-function capture")
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
        with tempfile.TemporaryDirectory(prefix="nflstreet-update-sequence-") as root:
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
                expected_name = session.read_bytes(actor + 0xB80, 16).split(b"\0", 1)[0]
                target = actor + POSITION_OFFSET
                emit(output, event="ready", actor=f"0x{actor:08X}",
                     target=f"0x{target:08X}", position=position,
                     actor_slot=args.actor_slot, actor_name_hex=expected_name.hex(),
                     savestate_sha256=hashlib.sha256(args.savestate.read_bytes()).hexdigest(),
                     stick=[args.stick_x, args.stick_y])
                gdb = session._gdb
                assert gdb is not None
                frames_dir = session.user_dir / "Dump" / "Frames"

                def scheduler_context() -> dict[str, object]:
                    if not args.capture_scheduler:
                        return {}
                    stack = gdb.read_register(1)
                    frames = sorted(frames_dir.glob("*.png"))
                    return {
                        "lr": f"0x{gdb.read_register(0x43):08X}",
                        "sp": f"0x{stack:08X}",
                        "saved_return": f"0x{session.read_u32(stack + 4):08X}",
                        "dumped_frames": len(frames),
                        "latest_frame": max(frames, key=lambda path: int(path.stem.rsplit("_", 1)[-1])).name
                        if frames else None,
                    }

                gdb.interrupt()
                wait_for_pc(gdb)
                gdb.continue_execution()
                session.play_sequence_async(InputSequence([
                    InputCommand(0, f"SET MAIN {args.stick_x} {args.stick_y}"),
                    InputCommand(args.wait_before_trace + args.trace_duration,
                                 "SET MAIN 0.5 0.5"),
                ]))
                time.sleep(args.wait_before_trace)
                gdb.interrupt()
                wait_for_pc(gdb)
                gdb.set_execution_breakpoint(PREPARATION_PC)
                gdb.continue_execution()

                phase = "preparation"
                completed = 0
                reset_watching = False
                input_listening = False
                input_return_pc = None
                actor_source_listening = False
                actor_source_return_pc = None
                input_watch_active = False
                actor_input_watch_active = False
                actor_input_watch_hits = 0
                position_watch_active = False
                position_write_count = 0
                started = time.monotonic()
                while completed < args.updates and time.monotonic() - started < args.trace_duration:
                    try:
                        reply = gdb.wait_for_stop(0.5)
                        pc = gdb.read_pc()
                    except GDBError:
                        continue
                    if input_return_pc is not None and pc == input_return_pc:
                        gdb.remove_execution_breakpoint(input_return_pc)
                        input_return_pc = None
                        emit(output, event="input_written", before_index=completed,
                             pc=f"0x{pc:08X}", actor=f"0x{actor:08X}",
                             actor_inputs_hex=session.read_bytes(actor + 0x200, 0x30).hex())
                        if args.watch_input_handoff and completed == 1:
                            gdb.set_write_watchpoint(actor + 0x200 + args.watch_input_handoff_offset, 4)
                            input_watch_active = True
                        gdb.set_execution_breakpoint(INPUT_PC)
                        input_listening = True
                    elif actor_source_return_pc is not None and pc == actor_source_return_pc:
                        gdb.remove_execution_breakpoint(actor_source_return_pc)
                        actor_source_return_pc = None
                        emit(output, event="actor_source_return", before_index=completed,
                             pc=f"0x{pc:08X}", actor=f"0x{actor:08X}",
                             actor_inputs_hex=session.read_bytes(actor + 0x200, 0x30).hex())
                        gdb.set_execution_breakpoint(ACTOR_INPUT_SOURCE_PC)
                        actor_source_listening = True
                    elif input_listening and pc == INPUT_PC:
                        destination = gdb.read_register(3)
                        gdb.remove_execution_breakpoint(INPUT_PC)
                        input_listening = False
                        if destination != actor:
                            gdb.step_instruction()
                            gdb.set_execution_breakpoint(INPUT_PC)
                            input_listening = True
                            gdb.continue_execution()
                            continue
                        input_pointer = gdb.read_register(4)
                        input_return_pc = gdb.read_register(0x43)
                        emit(output, event="input_entry", before_index=completed,
                             pc=f"0x{pc:08X}", return_pc=f"0x{input_return_pc:08X}",
                             actor=f"0x{actor:08X}", input_pointer=f"0x{input_pointer:08X}",
                             input_hex=session.read_bytes(input_pointer, 0x20).hex(),
                             actor_inputs_hex=session.read_bytes(actor + 0x200, 0x30).hex(),
                             **scheduler_context())
                        gdb.set_execution_breakpoint(input_return_pc)
                    elif actor_source_listening and pc == ACTOR_INPUT_SOURCE_PC:
                        destination = gdb.read_register(3)
                        gdb.remove_execution_breakpoint(ACTOR_INPUT_SOURCE_PC)
                        actor_source_listening = False
                        if destination != actor:
                            gdb.step_instruction()
                            gdb.set_execution_breakpoint(ACTOR_INPUT_SOURCE_PC)
                            actor_source_listening = True
                            gdb.continue_execution()
                            continue
                        vector_pointer = gdb.read_register(4)
                        actor_source_return_pc = gdb.read_register(0x43)
                        selected_actor = gdb.read_register(29)
                        formation_pointer = gdb.read_register(28) + 4
                        roster_manager = session.read_u32(0x803A1E10)
                        roster_base = session.read_u32(roster_manager)
                        roster_count = int.from_bytes(
                            session.read_bytes(roster_manager + 8, 2), "big")
                        selected_slot = ((selected_actor - roster_base) // 0x14D4
                                         if selected_actor >= roster_base and
                                         (selected_actor - roster_base) % 0x14D4 == 0
                                         else None)
                        if selected_slot is not None and selected_slot >= 2 * roster_count:
                            selected_slot = None
                        emit(output, event="actor_source_entry", before_index=completed,
                             pc=f"0x{pc:08X}", return_pc=f"0x{actor_source_return_pc:08X}",
                             actor=f"0x{actor:08X}", vector_pointer=f"0x{vector_pointer:08X}",
                             vector_hex=session.read_bytes(vector_pointer, 12).hex(),
                             selected_actor=f"0x{selected_actor:08X}",
                             selected_slot=selected_slot,
                             selected_position_hex=(session.read_bytes(
                                 selected_actor + POSITION_OFFSET, 12).hex()
                                 if selected_slot is not None else None),
                             formation_vector_hex=session.read_bytes(
                                 formation_pointer, 8).hex(),
                             position_hex=session.read_bytes(target, 12).hex(),
                             actor_inputs_hex=session.read_bytes(actor + 0x200, 0x30).hex())
                        gdb.set_execution_breakpoint(actor_source_return_pc)
                    elif phase == "preparation" and pc == PREPARATION_PC:
                        destination = gdb.read_register(3)
                        gdb.remove_execution_breakpoint(PREPARATION_PC)
                        if destination != actor:
                            gdb.step_instruction()
                            gdb.set_execution_breakpoint(PREPARATION_PC)
                            gdb.continue_execution()
                            continue
                        if reset_watching:
                            gdb.remove_write_watchpoint(target + 0x34, 4)
                            reset_watching = False
                        if input_listening:
                            gdb.remove_execution_breakpoint(INPUT_PC)
                            input_listening = False
                        if actor_source_listening:
                            gdb.remove_execution_breakpoint(ACTOR_INPUT_SOURCE_PC)
                            actor_source_listening = False
                        if input_watch_active:
                            gdb.remove_write_watchpoint(actor + 0x200 + args.watch_input_handoff_offset, 4)
                            input_watch_active = False
                        if actor_input_watch_active:
                            gdb.remove_write_watchpoint(actor + 0x200 + args.watch_actor_input_offset, 4)
                            actor_input_watch_active = False
                        if position_watch_active:
                            gdb.remove_write_watchpoint(target, 4)
                            position_watch_active = False
                        emit(output, event="preparation", index=completed,
                             pc=f"0x{pc:08X}", actor=f"0x{actor:08X}",
                             actor_header_hex=session.read_bytes(actor, 4).hex(),
                             actor_bb4_hex=session.read_bytes(actor + 0xbb4, 2).hex(),
                             actor_inputs_hex=session.read_bytes(actor + 0x200, 0x30).hex(),
                             state_hex=session.read_bytes(target, STATE_SIZE).hex(),
                             **scheduler_context())
                        gdb.set_execution_breakpoint(MOVEMENT_PC)
                        phase = "movement"
                    elif phase == "movement" and pc == MOVEMENT_PC:
                        destination = gdb.read_register(3)
                        gdb.remove_execution_breakpoint(MOVEMENT_PC)
                        if destination != target:
                            gdb.step_instruction()
                            gdb.set_execution_breakpoint(MOVEMENT_PC)
                            gdb.continue_execution()
                            continue
                        emit(output, event="movement_entry", index=completed,
                             pc=f"0x{pc:08X}", address=f"0x{target:08X}",
                             state_hex=session.read_bytes(target, STATE_SIZE).hex(),
                             **scheduler_context())
                        gdb.set_execution_breakpoint(MOVEMENT_EXIT_PC)
                        phase = "exit"
                    elif phase == "exit" and pc == MOVEMENT_EXIT_PC:
                        gdb.remove_execution_breakpoint(MOVEMENT_EXIT_PC)
                        emit(output, event="movement_exit", index=completed,
                             pc=f"0x{pc:08X}", address=f"0x{target:08X}",
                             state_hex=session.read_bytes(target, STATE_SIZE).hex(),
                             **scheduler_context())
                        completed += 1
                        if args.capture_input_function and completed < args.updates:
                            gdb.set_execution_breakpoint(INPUT_PC)
                            input_listening = True
                        if args.capture_actor_input_source and completed < args.updates:
                            gdb.set_execution_breakpoint(ACTOR_INPUT_SOURCE_PC)
                            actor_source_listening = True
                        switch = (args.switch_after_first if completed == 1 else
                                  args.switch_after_second if completed == 2 else None)
                        if switch and completed < args.updates:
                            stick = {"right": (1.0, 0.5), "forward": (0.5, 0.0),
                                     "backward": (0.5, 1.0), "left": (0.0, 0.5),
                                     "neutral": (0.5, 0.5),
                                     "half_right": (0.75, 0.5),
                                     "half_forward": (0.5, 0.25)}[
                                         switch]
                            session.send_input(f"SET MAIN {stick[0]} {stick[1]}")
                            emit(output, event="input_switch", after_index=completed - 1,
                                 stick=list(stick))
                        if completed < args.updates:
                            if completed == 1 and args.watch_actor_input_after_first:
                                gdb.set_write_watchpoint(actor + 0x200 + args.watch_actor_input_offset, 4)
                                actor_input_watch_active = True
                            if completed == 1 and args.watch_position_after_first:
                                gdb.set_write_watchpoint(target, 4)
                                position_watch_active = True
                            if args.capture_reset_all or (completed == 1 and args.capture_reset_function):
                                gdb.set_execution_breakpoint(RESET_PC)
                                phase = "reset_entry"
                            else:
                                if completed == 1 and args.watch_acceleration_reset:
                                    gdb.set_write_watchpoint(target + 0x34, 4)
                                    reset_watching = True
                                gdb.set_execution_breakpoint(PREPARATION_PC)
                                phase = "preparation"
                    elif phase == "reset_entry" and pc == RESET_PC:
                        destination = gdb.read_register(3)
                        gdb.remove_execution_breakpoint(RESET_PC)
                        if destination != actor:
                            gdb.step_instruction()
                            gdb.set_execution_breakpoint(RESET_PC)
                            gdb.continue_execution()
                            continue
                        return_pc = gdb.read_register(0x43)
                        if return_pc != RESET_RETURN_PC:
                            raise RuntimeError(f"unexpected reset return {return_pc:#x}")
                        emit(output, event="reset_entry", after_index=completed - 1,
                             pc=f"0x{pc:08X}", actor=f"0x{actor:08X}",
                             actor_header_hex=session.read_bytes(actor, 4).hex(),
                             actor_inputs_hex=session.read_bytes(actor + 0x200, 0x30).hex(),
                             actor_bbc_hex=session.read_bytes(actor + 0xbbc, 2).hex(),
                             actor_408_hex=session.read_bytes(actor + 0x408, 4).hex(),
                             actor_30c_hex=session.read_bytes(actor + 0x30c, 4).hex(),
                             state_hex=session.read_bytes(target, STATE_SIZE).hex(),
                             **scheduler_context())
                        gdb.set_execution_breakpoint(RESET_RETURN_PC)
                        phase = "reset_return"
                    elif phase == "reset_return" and pc == RESET_RETURN_PC:
                        gdb.remove_execution_breakpoint(RESET_RETURN_PC)
                        emit(output, event="reset_return", after_index=completed - 1,
                             pc=f"0x{pc:08X}", actor=f"0x{actor:08X}",
                             state_hex=session.read_bytes(target, STATE_SIZE).hex())
                        gdb.set_execution_breakpoint(PREPARATION_PC)
                        phase = "preparation"
                    elif input_watch_active:
                        before = session.read_bytes(actor + 0x200, 0x30).hex()
                        registers = {f"r{number}": f"0x{gdb.read_register(number):08X}"
                                     for number in (0, 9, 28, 31)}
                        source_hex = session.read_bytes(gdb.read_register(31), 0x20).hex()
                        destination_hex = session.read_bytes(gdb.read_register(28), 0x20).hex()
                        gdb.remove_write_watchpoint(actor + 0x200 + args.watch_input_handoff_offset, 4)
                        gdb.step_instruction()
                        next_pc = gdb.read_pc()
                        after = session.read_bytes(actor + 0x200, 0x30).hex()
                        emit(output, event="input_handoff_write", before_index=completed,
                             pc=f"0x{pc:08X}", next_pc=f"0x{next_pc:08X}",
                             reply=reply, offset=args.watch_input_handoff_offset,
                             registers=registers, source_hex=source_hex,
                             destination_hex=destination_hex,
                             before_hex=before, after_hex=after)
                        gdb.set_write_watchpoint(actor + 0x200 + args.watch_input_handoff_offset, 4)
                    elif actor_input_watch_active:
                        actor_input_watch_hits += 1
                        before = session.read_bytes(actor + 0x200, 0x30).hex()
                        registers = {f"r{number}": f"0x{gdb.read_register(number):08X}"
                                     for number in (3, 4, 30, 31)}
                        lr = gdb.read_register(0x43)
                        gdb.remove_write_watchpoint(actor + 0x200 + args.watch_actor_input_offset, 4)
                        gdb.step_instruction()
                        emit(output, event="actor_input_write", after_index=completed - 1,
                             pc=f"0x{pc:08X}", next_pc=f"0x{gdb.read_pc():08X}",
                             reply=reply, lr=f"0x{lr:08X}", registers=registers,
                             offset=args.watch_actor_input_offset,
                             before_hex=before,
                             after_hex=session.read_bytes(actor + 0x200, 0x30).hex())
                        if (args.watch_actor_input_hits and
                                actor_input_watch_hits >= args.watch_actor_input_hits):
                            actor_input_watch_active = False
                        else:
                            gdb.set_write_watchpoint(actor + 0x200 + args.watch_actor_input_offset, 4)
                    elif position_watch_active:
                        position_write_count += 1
                        actor_name = session.read_bytes(actor + 0xB80, 16)
                        emit(output, event="position_write", after_index=completed - 1,
                             hit=position_write_count, pc=f"0x{pc:08X}", reply=reply,
                             r3=f"0x{gdb.read_register(3):08X}",
                             actor_name_hex=actor_name.hex(),
                             actor_valid=actor_name.split(b"\0", 1)[0] == expected_name,
                             roster_manager=f"0x{session.read_u32(0x803A1E10):08X}",
                             actor_inputs_hex=session.read_bytes(actor + 0x200, 0x30).hex(),
                             state_hex=session.read_bytes(target, STATE_SIZE).hex())
                        if position_write_count >= 8:
                            gdb.remove_write_watchpoint(target, 4)
                            position_watch_active = False
                    elif phase == "preparation" and reset_watching:
                        before = session.read_bytes(target + 0x34, 4).hex()
                        gdb.remove_write_watchpoint(target + 0x34, 4)
                        gdb.step_instruction()
                        next_pc = gdb.read_pc()
                        after = session.read_bytes(target + 0x34, 4).hex()
                        emit(output, event="acceleration_write", after_index=completed - 1,
                             pc=f"0x{pc:08X}", next_pc=f"0x{next_pc:08X}",
                             reply=reply, before_hex=before, after_hex=after)
                        gdb.set_write_watchpoint(target + 0x34, 4)
                    else:
                        emit(output, event="other_stop", pc=f"0x{pc:08X}",
                             phase=phase)
                    if completed < args.updates:
                        gdb.continue_execution()
                emit(output, event="done", captured=completed,
                     requested=args.updates)
                if completed != args.updates:
                    gdb.interrupt()
                    final_pc = wait_for_pc(gdb)
                    emit(output, event="incomplete_snapshot", captured=completed,
                         pc=f"0x{final_pc:08X}",
                         r3=f"0x{gdb.read_register(3):08X}",
                         actor_inputs_hex=session.read_bytes(actor + 0x200, 0x30).hex(),
                         state_hex=session.read_bytes(target, STATE_SIZE).hex())
                    raise RuntimeError("incomplete target-actor update sequence")


if __name__ == "__main__":
    main()
