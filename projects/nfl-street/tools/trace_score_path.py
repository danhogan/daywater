"""Replay the GNNE69 PAT state and trace the verified score update path.

This is an NFL Street research tool. It does not alter game memory. Generated
logs and optional frames belong outside the repository (W: is preferred).
The replay can yield a successful or failed conversion; retain both results.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import sys
import tempfile
import threading
import time
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from find_named_score_candidate import find_candidates  # noqa: E402
from src.core.dolphin.input import InputCommand, InputSequence  # noqa: E402
from src.core.dolphin.runner import read_game_id  # noqa: E402
import src.core.dolphin.runner as runner  # noqa: E402
import src.core.dolphin.session as session_module  # noqa: E402
from src.core.dolphin.debugger import GDBError  # noqa: E402


ISO_SHA1 = "813c4511ba3135059a13de424555647e8252254b"
SCORE_POINTER_SLOT = 0x803A29C8
SCORE_RECORD_PC = 0x80025ECC
SCORE_DELTA_PC = 0x8002676C
SCORE_WRITER_PC = 0x80104294
TABLE_GET_RETURN_PC = 0x80053170
TABLE_SET_ENTRY_PC = 0x80053180
TABLE_LOAD_PC = 0x801B94F8
TABLE_STORE_PC = 0x801B95E0
TARGETS = (SCORE_DELTA_PC, SCORE_WRITER_PC)


def pat_sequence() -> InputSequence:
    """Input timing that yielded both 6→7 and 6→6 observed outcomes."""
    return InputSequence(
        [
            InputCommand(0, "PRESS A"), InputCommand(0.15, "RELEASE A"),
            InputCommand(4, "PRESS A"), InputCommand(4.15, "RELEASE A"),
            InputCommand(8, "PRESS A"), InputCommand(8.15, "RELEASE A"),
            InputCommand(12, "PRESS A"), InputCommand(12.15, "RELEASE A"),
            InputCommand(16, "PRESS A"), InputCommand(16.15, "RELEASE A"),
            InputCommand(25, "PRESS A"), InputCommand(25.15, "RELEASE A"),
            InputCommand(26, "SET MAIN 0.5 0.0"),
            InputCommand(32, "PRESS A"), InputCommand(32.15, "RELEASE A"),
            InputCommand(40, "PRESS A"), InputCommand(40.15, "RELEASE A"),
            InputCommand(60, "SET MAIN 0.5 0.5"),
        ]
    )


def verified_iso(path: Path) -> None:
    if read_game_id(path) != "GNNE69":
        raise ValueError("expected GNNE69 game image")
    digest = hashlib.sha1()
    with path.open("rb") as image:
        for block in iter(lambda: image.read(8 * 1024 * 1024), b""):
            digest.update(block)
    if digest.hexdigest() != ISO_SHA1:
        raise ValueError("ISO SHA-1 differs from the researched GNNE69 image")


def score_fields(session: session_module.DolphinSession) -> dict[str, int | str]:
    pointer = session.read_u32(SCORE_POINTER_SLOT)
    if not 0x80000000 <= pointer <= 0x817FFFBC or pointer % 4:
        raise RuntimeError(f"invalid score object pointer: 0x{pointer:08X}")
    fields = session.read_bytes(pointer + 0x3C, 4)
    return {
        "pointer": f"0x{pointer:08X}",
        "side_0": int.from_bytes(fields[:2], "big"),
        "side_1": int.from_bytes(fields[2:], "big"),
    }


def complete_png(path: Path) -> bool:
    """Reject a frame still being written by Dolphin."""
    data = path.read_bytes()
    if not data.startswith(b"\x89PNG\r\n\x1a\n"):
        return False
    offset = 8
    while offset + 12 <= len(data):
        length = int.from_bytes(data[offset : offset + 4], "big")
        end = offset + 12 + length
        if end > len(data):
            return False
        expected = int.from_bytes(data[end - 4 : end], "big")
        if zlib.crc32(data[offset + 4 : end - 4]) != expected:
            return False
        if data[offset + 4 : offset + 8] == b"IEND":
            return end == len(data)
        offset = end
    return False


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", required=True, type=Path)
    parser.add_argument("--savestate", required=True, type=Path)
    parser.add_argument("--dolphin-binary", required=True, type=Path)
    parser.add_argument("--output", type=Path, help="JSON Lines trace outside Git")
    parser.add_argument("--frame-output", type=Path, help="last complete frame outside Git")
    parser.add_argument("--trace-table", action="store_true", help="also trace FNIG score wrapper values")
    parser.add_argument("--trace-store", action="store_true", help="trace the FNIG-linked score-byte store")
    parser.add_argument("--capture-record", action="store_true", help="capture the score-handler entry record for native replay")
    parser.add_argument("--duration", type=float, default=90)
    parser.add_argument("--gdb-port", type=int, default=2394)
    args = parser.parse_args()
    if args.duration <= 0 or not 0 < args.gdb_port < 65536:
        parser.error("duration and GDB port must be positive")
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
        with tempfile.TemporaryDirectory(prefix="nflstreet-score-trace-") as root:
            user_dir = Path(root)
            stop_pruning = threading.Event()

            def prune_frames() -> None:
                frames_dir = user_dir / "Dump" / "Frames"
                while not stop_pruning.wait(0.5):
                    frames = sorted(frames_dir.glob("*.png"), key=lambda p: p.stat().st_mtime)
                    for frame in frames[:-12]:
                        frame.unlink(missing_ok=True)

            pruner = threading.Thread(target=prune_frames, daemon=True)
            pruner.start()
            try:
                with session_module.DolphinSession.start(
                    args.iso, savestate=args.savestate, pipe_input=True,
                    gdb_port=args.gdb_port, video_backend="OGL", user_dir=user_dir,
                ) as session:
                    if not session.wait_for_first_frame(90):
                        raise RuntimeError("Dolphin produced no completed frame")
                    time.sleep(2)
                    initial = score_fields(session)
                    frame_dir = user_dir / "Dump" / "Frames"
                    first_frame = max(frame_dir.glob("*.png"), key=lambda p: p.stat().st_mtime_ns)
                    emit(event="ready", frame=first_frame.name,
                         frame_mtime_ns=first_frame.stat().st_mtime_ns, **initial)
                    if initial["side_1"] != 6 or initial["side_0"] != 0:
                        raise RuntimeError("savestate is not the researched 6–0 PAT state")

                    gdb = session._gdb
                    assert gdb is not None
                    gdb.interrupt()
                    for attempt in range(3):
                        try:
                            gdb.read_pc()  # Synchronize after duplicate stop notifications.
                            break
                        except GDBError as exc:
                            if "Invalid PC response: ''" not in str(exc) or attempt == 2:
                                raise
                            time.sleep(0.1)
                    targets = TARGETS + ((SCORE_RECORD_PC,) if args.capture_record else ())
                    armed = set(targets)
                    for target in targets:
                        gdb.set_execution_breakpoint(target)
                    gdb.continue_execution()
                    session.play_sequence_async(pat_sequence())

                    started = time.monotonic()
                    previous = initial
                    hit_count = 0
                    store_address: int | None = None
                    while time.monotonic() - started < args.duration:
                        try:
                            reply = gdb.wait_for_stop(0.5)
                        except GDBError:
                            reply = None
                        if reply:
                            pc = gdb.read_pc()
                            if pc in armed:
                                fields = score_fields(session)
                                emit(
                                    event="breakpoint", seconds=round(time.monotonic() - started, 3),
                                    pc=f"0x{pc:08X}", r3=gdb.read_register(3),
                                    r4=gdb.read_register(4), lr=f"0x{gdb.read_register(0x43):08X}",
                                    **fields,
                                )
                                hit_count += 1
                                gdb.remove_execution_breakpoint(pc)
                                armed.remove(pc)
                                if pc == SCORE_RECORD_PC:
                                    address = gdb.read_register(3)
                                    if not 0x80000000 <= address <= 0x817FFFE0:
                                        raise RuntimeError(f"invalid score record pointer: 0x{address:08X}")
                                    record = session.read_bytes(address, 0x20)
                                    emit(
                                        event="score_record_entry", address=f"0x{address:08X}",
                                        control=gdb.read_register(4), raw=record.hex(),
                                        flags=int.from_bytes(record[0x18:0x1C], "big"),
                                        side=record[0x1C],
                                        amount=int.from_bytes(record[0x1D:0x1E], "big", signed=True),
                                    )
                                if args.trace_store and pc == SCORE_DELTA_PC and gdb.read_register(4) == 0:
                                    emit(event="no_score_control", reason="delta amount was zero")
                                    break
                                if (args.trace_table or args.trace_store) and pc == SCORE_DELTA_PC:
                                    for table_pc in (TABLE_GET_RETURN_PC, TABLE_SET_ENTRY_PC):
                                        gdb.set_execution_breakpoint(table_pc)
                                        armed.add(table_pc)
                                    if args.trace_store:
                                        gdb.set_execution_breakpoint(TABLE_LOAD_PC)
                                        armed.add(TABLE_LOAD_PC)
                                elif args.trace_store and pc == TABLE_LOAD_PC:
                                    records = find_candidates(session.read_bytes(0x80000000, 0x01800000))
                                    if len(records) != 1:
                                        raise RuntimeError(f"expected one FNIG-linked record, found {len(records)}")
                                    descriptor, record, left, right = records[0]
                                    address = gdb.read_register(3)
                                    if address != record + 0x10:
                                        raise RuntimeError(
                                            f"load address 0x{address:08X} differs from FNIG byte 0x{record + 0x10:08X}"
                                        )
                                    emit(
                                        event="fnig_load", pc=f"0x{pc:08X}",
                                        descriptor=f"0x{descriptor:08X}",
                                        address=f"0x{address:08X}",
                                        bytes=session.read_bytes(address, 8).hex(),
                                        side_1=left, side_0=right,
                                    )
                                elif args.trace_store and pc == TABLE_SET_ENTRY_PC:
                                    records = find_candidates(session.read_bytes(0x80000000, 0x01800000))
                                    if len(records) != 1:
                                        raise RuntimeError(f"expected one FNIG-linked record, found {len(records)}")
                                    descriptor, record, left, right = records[0]
                                    store_address = record + 0x10
                                    emit(
                                        event="fnig_record", descriptor=f"0x{descriptor:08X}",
                                        record=f"0x{record:08X}", side_1=left, side_0=right,
                                    )
                                    gdb.set_execution_breakpoint(TABLE_STORE_PC)
                                    armed.add(TABLE_STORE_PC)
                                elif args.trace_store and pc == TABLE_STORE_PC:
                                    assert store_address is not None
                                    target = gdb.read_register(22)
                                    value = gdb.read_register(11)
                                    before = session.read_bytes(store_address, 4).hex()
                                    if target != store_address:
                                        raise RuntimeError(
                                            f"store target 0x{target:08X} differs from FNIG byte 0x{store_address:08X}"
                                        )
                                    gdb.continue_execution()
                                    time.sleep(0.5)
                                    emit(
                                        event="fnig_store", pc=f"0x{pc:08X}",
                                        address=f"0x{target:08X}", value=f"0x{value:08X}",
                                        before=before, after=session.read_bytes(store_address, 4).hex(),
                                        **score_fields(session),
                                    )
                                    break
                            else:
                                emit(event="other_stop", pc=f"0x{pc:08X}", reply=reply)
                            gdb.continue_execution()

                        current = score_fields(session)
                        if current != previous:
                            emit(
                                event="score_change", seconds=round(time.monotonic() - started, 3),
                                before=previous, after=current,
                            )
                            if not args.trace_store:
                                break
                        previous = current

                    latest_frames = list(frame_dir.glob("*.png"))
                    latest_frame = max(latest_frames, key=lambda p: p.stat().st_mtime_ns)
                    emit(
                        event="done", seconds=round(time.monotonic() - started, 3),
                        hits=hit_count, frame=latest_frame.name,
                        frame_mtime_ns=latest_frame.stat().st_mtime_ns,
                        **score_fields(session),
                    )
                    if args.frame_output:
                        frames = sorted(
                            (user_dir / "Dump" / "Frames").glob("*.png"),
                            key=lambda p: p.stat().st_mtime, reverse=True,
                        )
                        for frame in frames:
                            if complete_png(frame):
                                shutil.copy2(frame, args.frame_output)
                                break
            finally:
                stop_pruning.set()
                pruner.join(timeout=2)
    finally:
        if output:
            output.close()


if __name__ == "__main__":
    main()
