"""Validate native command flag effects and stage-three record polling."""
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
from pathlib import Path

from verify_handle_calls import verify
from verify_handle_writer import check_dol
from verify_record_handoff import WORDS as RECORD_WORDS
from verify_target_selection import dol_word

WORDS = {
    0x80015340: 0x7C9E2378, 0x80015348: 0x7C7F1B78,
    0x800153CC: 0x38000000, 0x800153D0: 0x6000C004,
    0x800153D4: 0x7C1E0000, 0x800153D8: 0x41820420,
    0x8001542C: 0x38000000, 0x80015430: 0x6000C006,
    0x80015434: 0x7C1E0000, 0x80015438: 0x418203AC,
    0x800157E4: 0x801F000C, 0x800157EC: 0x60000001,
    0x800157F0: 0x901F000C, 0x800157F8: 0x801F000C,
    0x80015800: 0x60000004, 0x80015804: 0x901F000C,
    0x8007C2FC: 0x2C000003, 0x8007C300: 0x41820458,
    0x8007C758: 0x801F000C, 0x8007C75C: 0x70090001,
    0x8007C760: 0x418200A8, 0x8007C764: 0x5400003C,
    0x8007C768: 0x901F000C,
    0x8003D834: 0x801E000C, 0x8003D838: 0x70090400,
    0x8003D83C: 0x41820038, 0x8003D840: 0x801D000C,
    0x8003D844: 0x70090400, 0x8003D848: 0x4082002C,
    0x8003D75C: 0x801F000C, 0x8003D764: 0x993F0008,
    0x8003D768: 0x540005A8, 0x8003D76C: 0x540004A0,
    0x8003D770: 0x901F000C, 0x8007BFE8: 0x4BFC1835,
    0x8002E4F8: 0x812D8308, 0x8002E4FC: 0x80090000,
}


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--dol", required=True, type=Path)
    p.add_argument("--trace", required=True, type=Path)
    p.add_argument("--executable", required=True, type=Path)
    p.add_argument("--windows", action="store_true")
    args = p.parse_args()
    check_dol(args.dol)
    raw = args.dol.read_bytes()
    for pc, word in {**WORDS, **RECORD_WORDS}.items():
        if dol_word(raw, pc) != word:
            raise ValueError(f"original command/gate differs at {pc:#x}")
    report = verify(args.trace)
    if report["completed_calls"] != 3 or report["incomplete_call"]:
        raise ValueError("need three complete original record writes")
    rows = [json.loads(line) for line in args.trace.read_text().splitlines()]
    ready = rows[0]
    prefix = [str(args.executable)]
    if args.windows:
        exe = subprocess.run(["wslpath", "-w", str(args.executable.resolve())],
                             text=True, capture_output=True, check=True).stdout.strip()
        prefix = ["cmd.exe", "/c", exe]

    def replay(arguments: list[str], expected: str) -> None:
        result = subprocess.run(prefix + arguments, text=True, capture_output=True,
                                check=True, timeout=15, cwd=args.executable.resolve().parent)
        if result.stdout.strip() != expected:
            raise ValueError("native command/gate output differs from original")

    commands = gates = changed = waits = 0
    latest_ready = {}
    for i, row in enumerate(rows):
        if row["event"] == "event_flag_command":
            after = rows[i + 1]
            expected_pc = {0xc004: 0x80015804, 0xc006: 0x800157F0}.get(row["command"])
            if (expected_pc != row["pc"] or after["event"] != "event_flag_written" or
                    after["actor"] != row["actor"] or after["pc"] != row["pc"] + 4 or
                    after["after"] != row["store_value"] or
                    row["callback_pointer"] != 0x8001532C or
                    row["parser_return"] != 0x80173A64 or
                    row["relay_return"] != 0x80177088):
                raise ValueError("command does not follow observed callback/store chain")
            replay(["C", hex(row["before"]), hex(row["command"])], f"{after['after']:08x}")
            commands += 1
            if row["command"] == 0xc006:
                latest_ready[row["actor"]] = row
        elif row["event"] == "event_gate_entry":
            j = i + 1
            while j < len(rows) and rows[j]["event"] != "event_gate_return":
                if rows[j]["event"] == "event_gate_entry":
                    raise ValueError("nested gate")
                j += 1
            if j == len(rows):
                raise ValueError("gate has no return")
            after = rows[j]
            active = bool(row["flags"] & 1)
            if (row["pc"] != 0x8007C758 or after["actor"] != row["actor"] or
                    after["pc"] != (0x8007C7BC if active else 0x8007C808)):
                raise ValueError("gate boundary differs from supported path")
            inside = [r for r in rows[i + 1:j] if r["event"] == "actor_handoff"]
            index = 255
            if active:
                if len(inside) != 1 or row["actor"] not in latest_ready:
                    raise ValueError("ready gate lacks command or original handoff")
                signal = latest_ready[row["actor"]]
                if signal["store_value"] != row["flags"]:
                    raise ValueError("ready flag changed after the measured command")
                transfer = [r for r in rows[i + 1:j] if r["event"] == "actor_transfer_entry"]
                transfer_after = [r for r in rows[i + 1:j] if r["event"] == "actor_transfer_return"]
                if (len(transfer) != 1 or len(transfer_after) != 1 or
                        transfer[0]["pc"] != 0x8003D81C or transfer_after[0]["pc"] != 0x8007BFEC or
                        transfer[0]["actor"] != row["actor"] or
                        transfer[0]["incoming_actor"] != row["incoming_actor"]):
                    raise ValueError("gate lacks the original flag-transfer call")
                old_flags = int.from_bytes(bytes.fromhex(transfer[0]["old_header_hex"])[12:16], "big")
                new_flags = int.from_bytes(bytes.fromhex(transfer[0]["new_header_hex"])[12:16], "big")
                if old_flags != row["flags"] & ~1 or new_flags & 0x400 != row["incoming_flags"] & 0x400:
                    raise ValueError("actor flag inputs changed before transfer")
                index, remainder = divmod(inside[0]["record"] - 0x803B6F08, 0xF4)
                if remainder or not 0 <= index < len(row["record_handles"]):
                    raise ValueError("handoff record outside array")
                changed += 1
            else:
                if inside or after["record_handles"] != row["record_handles"]:
                    raise ValueError("waiting gate changed a record")
                waits += 1
            expected = f"{int(active)} {after['flags']:08x} {index} " + " ".join(
                f"{handle:08x}" for handle in after["record_handles"])
            replay(["G", row["event_hex"], hex(row["flags"]), hex(row["current_handle"]),
                    str(ready["actors_per_team"]), str(2 * ready["actors_per_team"]),
                    str(len(row["record_handles"])), hex(row["incoming_flags"]),
                    str(row["dispatch_value"]), *map(hex, row["record_handles"])], expected)
            gates += 1
    if commands < 3 or changed != 2 or waits != 2:
        raise ValueError("need both original wait/ready pairs and their command producers")
    print(json.dumps({"native_commands_matched": commands, "native_gates_matched": gates,
                      "waits": waits, "handoffs": changed,
                      "trace_sha256": hashlib.sha256(args.trace.read_bytes()).hexdigest()}, sort_keys=True))


if __name__ == "__main__":
    main()
