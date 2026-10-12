"""Compare native record lookup/reassignment with original actor handoffs."""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
from pathlib import Path

from verify_handle_calls import verify
from verify_handle_writer import check_dol
from verify_target_selection import dol_word

WORDS = {
    0x800BE508: 0x816D8C30,  # selection manager
    0x800BE518: 0x880B0000,  # record count
    0x800BE528: 0x1D3E00F4,  # record stride
    0x800BE52C: 0x3BE90008,  # record-array start
    0x800BE534: 0x386300B4,  # handle field
    0x800BE538: 0x4BF6240D,  # handle -> actor lookup
    0x800BE53C: 0x7C1D1800,  # compare requested actor to resolved actor
    0x800BE540: 0x40820010,  # skip unequal actor
    0x800BE548: 0x7C63FA14,  # return matching record
    0x800BE554: 0x381E0001,  # next index
    0x800BE55C: 0x892B0000,  # reread count
    0x800BE564: 0x4180FFC4,  # scan remaining records
    0x800BE568: 0x38600000,  # null on miss
    0x8007C7A4: 0x48041D55,  # original record lookup call
    0x8007C7A8: 0x7C651B78,  # record result -> r5
    0x8007C7AC: 0x7F84E378,  # new actor -> r4
    0x8007C7B0: 0x7FE3FB78,  # current actor -> r3
    0x8007C7B4: 0x7FA6EB78,  # event -> r6
    0x8007C7B8: 0x4BFFF731,  # handoff function
    0x8007BF0C: 0x7FC3F378,  # incoming record -> assignment r3
    0x8007BF10: 0x480423A5,  # assignment wrapper
}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dol", required=True, type=Path)
    parser.add_argument("--trace", required=True, type=Path)
    parser.add_argument("--executable", required=True, type=Path)
    parser.add_argument("--windows", action="store_true")
    args = parser.parse_args()
    check_dol(args.dol)
    dol = args.dol.read_bytes()
    for pc, expected in WORDS.items():
        if dol_word(dol, pc) != expected:
            raise ValueError(f"original record lookup/handoff differs at {pc:#x}")
    report = verify(args.trace)
    if report["incomplete_call"]:
        raise ValueError("use a complete handoff trace")
    rows = [json.loads(line) for line in args.trace.read_text().splitlines()]
    ready = rows[0]
    per_team = ready["actors_per_team"]
    base = ready["roster_base"]
    handoffs = []
    for i, row in enumerate(rows):
        if row["event"] != "actor_handoff":
            continue
        call, returned = rows[i + 1:i + 3]
        if (call["event"] != "handle_call" or returned["event"] != "handle_return" or
                call["source"] != row["new_actor"] or
                call["source_value"] != row["new_handle"] or
                call["destination"] != row["record"] + 0xB4 or
                returned["after"] != row["new_handle"] or
                call["wrapper_return"] != 0x8007BF14):
            raise ValueError("actor handoff does not feed the captured handle write")
        for prefix in ("current", "new"):
            handle = row[f"{prefix}_handle"]
            slot = ((handle >> 8) & 255) * per_team + ((handle >> 16) & 255)
            if handle & 255 != 1 or not 0 <= slot < 2 * per_team or row[f"{prefix}_actor"] != base + slot * 0x14D4:
                raise ValueError("handoff actor/handle identity differs")
        handoffs.append(row)
    if len(handoffs) < 2:
        raise ValueError("need consecutive actor handoffs")
    handles = list(handoffs[0]["record_handles"])
    command = [str(args.executable), str(per_team), str(2 * per_team), str(len(handles)),
               *[hex(handle) for handle in handles]]
    expected_lines = []
    for row in handoffs:
        if row["record_handles"] != handles:
            raise ValueError("record state changed between observed handoffs")
        index, remainder = divmod(row["record"] - 0x803B6F08, 0xF4)
        if remainder or not 0 <= index < len(handles):
            raise ValueError("returned record outside manager")
        command.extend([hex(row["current_handle"]), hex(row["new_handle"])])
        handles[index] = row["new_handle"]
        expected_lines.append(str(index) + " " + " ".join(f"{handle:08x}" for handle in handles))
    if args.windows:
        executable = subprocess.run(["wslpath", "-w", str(args.executable.resolve())],
                                    text=True, capture_output=True, check=True).stdout.strip()
        command = ["cmd.exe", "/c", executable, *command[1:]]
    result = subprocess.run(command, text=True, capture_output=True, check=True,
                            cwd=args.executable.resolve().parent, timeout=15)
    if result.stdout.splitlines() != expected_lines:
        raise ValueError("native record index/handle sequence differs from original")
    print(json.dumps({"native_handoffs_matched": len(handoffs),
                      "trace_sha256": hashlib.sha256(args.trace.read_bytes()).hexdigest(),
                      "final_handles": [f"0x{handle:08X}" for handle in handles]}, sort_keys=True))


if __name__ == "__main__":
    main()
