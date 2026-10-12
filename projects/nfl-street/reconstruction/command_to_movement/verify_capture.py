"""Verify the native command store and movement step against a private live trace."""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
from pathlib import Path


def event(rows: list[dict], name: str) -> dict:
    found = [row for row in rows if row.get("event") == name]
    if len(found) != 1:
        raise ValueError(f"expected one {name} event, got {len(found)}")
    return found[0]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--trace", required=True, type=Path)
    parser.add_argument("--executable", required=True)
    parser.add_argument("--windows", action="store_true")
    args = parser.parse_args()
    data = args.trace.read_bytes()
    rows = [json.loads(line) for line in data.splitlines()]
    command_entry = event(rows, "command_entry")
    command_written = event(rows, "command_written")
    movement_entry = event(rows, "entry")
    movement_exit = event(rows, "exit")
    actor = bytes.fromhex(command_entry["actor_inputs_hex"])
    before = bytes.fromhex(command_entry["state_hex"])
    written = bytes.fromhex(command_written["state_hex"])
    entered = bytes.fromhex(movement_entry["state_hex"])
    expected = bytes.fromhex(movement_exit["state_hex"])
    if (len(actor) != 0x30 or any(len(x) != 0x54 for x in
                                  (before, written, entered, expected))):
        raise ValueError("unexpected live block length")
    if (command_entry["pc"] != "0x80036B18" or
            command_entry["return_pc"] != "0x800F71E0" or
            movement_entry["pc"] != "0x80036B28" or
            movement_exit["pc"] != "0x80036E58" or
            written != entered):
        raise ValueError("unexpected command-to-movement boundary")
    heading = int.from_bytes(actor[4:8], "big")
    result_heading = int(command_entry["r5"], 16)
    if (heading != int(command_entry["r4"], 16) or
            written[0x18:0x1c] != result_heading.to_bytes(4, "big") or
            written[0x38:0x3c] != heading.to_bytes(4, "big") or
            written[0x34:0x38] != before[0x34:0x38]):
        raise ValueError("capture does not establish the guarded command inputs")
    argv = [args.executable, actor.hex(), before.hex(),
            before[0x34:0x38].hex()]
    if args.windows:
        argv = ["cmd.exe", "/c", *argv]
    command_result = subprocess.run([*argv, "--command-only"],
                                    capture_output=True, text=True, check=True)
    command_actual = bytes.fromhex(command_result.stdout.strip().splitlines()[-1])
    if command_actual != written:
        raise ValueError("native command store differs from the live state")
    result = subprocess.run(argv, capture_output=True, text=True, check=True)
    actual = bytes.fromhex(result.stdout.strip().splitlines()[-1])
    different = [f"0x{i:02X}" for i in range(0, 0x54, 4)
                 if actual[i:i + 4] != expected[i:i + 4]]
    print(json.dumps({"trace": args.trace.name,
                      "trace_sha256": hashlib.sha256(data).hexdigest(),
                      "matched": not different, "different_words": different}, sort_keys=True))
    if different:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
