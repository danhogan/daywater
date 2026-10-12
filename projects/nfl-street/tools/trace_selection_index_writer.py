"""Look for a write to GNNE69's selection index or selected record handle.

Requires the compatible saved match. Keep raw output in private storage.
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

SMALL_DATA = 0x803A9A40
MANAGER_SLOT = SMALL_DATA - 0x73D0


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", required=True, type=Path)
    parser.add_argument("--savestate", required=True, type=Path)
    parser.add_argument("--dolphin-binary", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--field", choices=("index", "handle"), default="index")
    parser.add_argument("--snap", action="store_true",
                        help="press A once in the first-down preplay state")
    parser.add_argument("--stick-x", type=float, default=1.0)
    parser.add_argument("--stick-y", type=float, default=0.5)
    parser.add_argument("--seconds", type=float, default=40)
    parser.add_argument("--gdb-port", type=int, default=2485)
    args = parser.parse_args()
    if (args.output.exists() or not 0 <= args.stick_x <= 1 or
            not 0 <= args.stick_y <= 1 or args.seconds <= 0):
        parser.error("invalid output, stick, or duration")
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

        with tempfile.TemporaryDirectory(prefix="nflstreet-index-writer-") as root:
            with session_module.DolphinSession.start(
                args.iso, savestate=args.savestate, pipe_input=True,
                gdb_port=args.gdb_port, video_backend="OGL", user_dir=Path(root),
            ) as session:
                if not session.wait_for_first_frame(90):
                    raise RuntimeError("Dolphin produced no completed frame")
                time.sleep(2)
                thomas, _ = thomas_info(session)
                manager = session.read_u32(MANAGER_SLOT)
                if manager != 0x803B6F00:
                    raise RuntimeError("selection manager differs from saved match")
                index = session.read_bytes(manager + 1, 1)[0]
                address = (manager + 1 if args.field == "index" else
                           manager + 8 + index * 0xF4 + 0xB4)
                size = 1 if args.field == "index" else 4
                initial = int.from_bytes(session.read_bytes(address, size), "big")
                emit(event="ready", field=args.field,
                     index_address=f"0x{address:08X}",
                     initial_index=initial, selection_index=index,
                     thomas=f"0x{thomas:08X}",
                     stick=[args.stick_x, args.stick_y], snap=args.snap,
                     requested_seconds=args.seconds,
                     savestate_sha256=hashlib.sha256(args.savestate.read_bytes()).hexdigest())
                gdb = session._gdb
                assert gdb is not None
                gdb.interrupt()
                gdb.set_write_watchpoint(address, size)
                gdb.continue_execution()
                if args.snap:
                    session.play_sequence(InputSequence([
                        InputCommand(0, "PRESS A"),
                        InputCommand(0.15, "RELEASE A"),
                    ]))
                session.play_sequence_async(InputSequence([
                    InputCommand(0, f"SET MAIN {args.stick_x} {args.stick_y}"),
                    InputCommand(args.seconds, "SET MAIN 0.5 0.5"),
                ]))
                started = time.monotonic()
                unrelated = 0
                while time.monotonic() - started < args.seconds:
                    try:
                        reply = gdb.wait_for_stop(0.5)
                        pc = gdb.read_pc()
                    except GDBError:
                        continue
                    # The GDB stub reports the watched address in its stop
                    # packet. Do not treat unrelated stops as write evidence.
                    if f"{address:08x}" not in reply.lower():
                        unrelated += 1
                        if unrelated > 8:
                            raise RuntimeError("too many unrelated GDB stops")
                        gdb.continue_execution()
                        continue
                    gdb.remove_write_watchpoint(address, size)
                    emit(event="write_hit", pc=f"0x{pc:08X}", reply=reply,
                         index_after=int.from_bytes(
                             session.read_bytes(address, size), "big"),
                         r13=f"0x{gdb.read_register(13):08X}",
                         r3=f"0x{gdb.read_register(3):08X}",
                         r4=f"0x{gdb.read_register(4):08X}",
                         r5=f"0x{gdb.read_register(5):08X}",
                         r6=f"0x{gdb.read_register(6):08X}",
                         r9=f"0x{gdb.read_register(9):08X}")
                    emit(event="done", hit=True, unrelated_stops=unrelated,
                         elapsed_seconds=round(time.monotonic() - started, 3))
                    return
                gdb.interrupt()
                gdb.remove_write_watchpoint(address, size)
                emit(event="done", hit=False, unrelated_stops=unrelated,
                     elapsed_seconds=round(time.monotonic() - started, 3),
                     final_index=int.from_bytes(
                         session.read_bytes(address, size), "big"))


if __name__ == "__main__":
    main()
