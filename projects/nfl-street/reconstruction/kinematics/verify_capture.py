"""Compare a private Dolphin kinematics capture with the native replay.

The trace contains game RAM and should remain on W: or another private path.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
from pathlib import Path


def single(rows: list[dict], event: str) -> dict:
    found = [row for row in rows if row.get("event") == event]
    if len(found) != 1:
        raise ValueError(f"expected exactly one {event} event, found {len(found)}")
    return found[0]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--trace", required=True, type=Path)
    parser.add_argument("--executable", required=True)
    parser.add_argument("--windows", action="store_true",
                        help="invoke the Windows executable through cmd.exe")
    args = parser.parse_args()
    trace_data = args.trace.read_bytes()
    rows = [json.loads(line) for line in trace_data.splitlines()]
    entry = single(rows, "entry")
    exit_event = single(rows, "exit")
    done = single(rows, "done")
    if not done["captured"] or entry["address"] != exit_event["address"]:
        raise ValueError("incomplete or mismatched capture")
    if entry["pc"] != "0x80036B28" or exit_event["pc"] != "0x80036E58":
        raise ValueError("unexpected function boundary")
    original = bytes.fromhex(exit_event["state_hex"])
    if len(original) != 0x54 or len(bytes.fromhex(entry["state_hex"])) != 0x54:
        raise ValueError("expected 0x54-byte input and output blocks")
    if any(row.get("event") == "command_entry" for row in rows):
        command = single(rows, "command_entry")
        written = single(rows, "command_written")
        actor_inputs = bytes.fromhex(command["actor_inputs_hex"])
        command_state = bytes.fromhex(written["state_hex"])
        direction = int(command["r4"], 16)
        if (command["return_pc"] != "0x800F71E0" or len(actor_inputs) != 0x30
                or int.from_bytes(actor_inputs[4:8], "big") != direction
                or int.from_bytes(command_state[0x38:0x3C], "big") != direction
                or command_state != bytes.fromhex(entry["state_hex"])):
            raise ValueError("command capture does not reach the movement entry")
    if any(row.get("event") == "input_entry" for row in rows):
        input_event = single(rows, "input_entry")
        input_written = single(rows, "input_written")
        if (input_event["return_pc"] != "0x8003E85C"
                or len(bytes.fromhex(input_event["input_hex"])) != 0x20
                or len(bytes.fromhex(input_written["actor_inputs_hex"])) != 0x30):
            raise ValueError("incomplete stick-to-actor input capture")
    if any(row.get("event") == "heading_store_entry" for row in rows):
        heading = single(rows, "heading_store_entry")
        heading_written = single(rows, "heading_written")
        command = single(rows, "command_entry")
        if (heading["pc"] != "0x8003E0A4"
                or heading_written["pc"] != "0x8003E0A8"
                or heading["before"] == heading["r29"]
                or heading_written["after"] != heading["r29"]
                or heading_written["after"] != command["r4"]):
            raise ValueError("heading transition did not reach the command setter")
    command = [args.executable, entry["state_hex"]]
    if args.windows:
        command = ["cmd.exe", "/c", *command]
    result = subprocess.run(command, capture_output=True, text=True, check=True)
    actual = bytes.fromhex(result.stdout.strip().splitlines()[-1])
    if len(actual) != 0x54:
        raise ValueError("native output is not a 0x54-byte block")
    different = [f"0x{offset:02X}" for offset in range(0, 0x54, 4)
                 if actual[offset:offset + 4] != original[offset:offset + 4]]
    report = {"trace": args.trace.name, "trace_sha256": hashlib.sha256(trace_data).hexdigest(),
              "matched": not different, "different_words": different}
    print(json.dumps(report, sort_keys=True))
    if different:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
