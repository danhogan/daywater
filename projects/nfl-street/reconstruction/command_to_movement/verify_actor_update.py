"""Check all three native actor-update stages against a private live capture."""

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
    preparation = event(rows, "preparation_entry")
    command = event(rows, "command_entry")
    command_written = event(rows, "command_written")
    movement_entry = event(rows, "entry")
    movement_exit = event(rows, "exit")
    if (preparation["pc"] != "0x800F705C" or
            command["pc"] != "0x80036B18" or
            command_written["pc"] != "0x800F71E0" or
            movement_entry["pc"] != "0x80036B28" or
            movement_exit["pc"] != "0x80036E58"):
        raise ValueError("unexpected original function boundary")
    before = bytes.fromhex(preparation["state_hex"])
    prepared = bytes.fromhex(command["state_hex"])
    written = bytes.fromhex(command_written["state_hex"])
    entered = bytes.fromhex(movement_entry["state_hex"])
    moved = bytes.fromhex(movement_exit["state_hex"])
    inputs = bytes.fromhex(preparation["actor_inputs_hex"])
    motion = bytes.fromhex(preparation["actor_motion_hex"])
    if (len(before) != 0x54 or len(inputs) != 0x30 or len(motion) != 0x28 or
            any(len(x) != 0x54 for x in (prepared, written, entered, moved))):
        raise ValueError("wrong captured block size")
    for actor_offset, state_offset in ((0x1c4, 0x1c), (0x1c8, 0x20),
                                       (0x1cc, 0x24), (0x1dc, 0x34)):
        if motion[actor_offset - 0x1c0:actor_offset - 0x1bc] != before[
                state_offset:state_offset + 4]:
            raise ValueError("actor motion fields do not alias the movement block")
    if (preparation["actor_inputs_hex"] != command["actor_inputs_hex"] or
            written != entered or
            int(command["r4"], 16) != int.from_bytes(inputs[4:8], "big") or
            int(command["r5"], 16) != int.from_bytes(written[0x18:0x1c], "big")):
        raise ValueError("inconsistent live handoff")
    base = [args.executable, preparation["actor_header_hex"],
            "0000" + preparation["actor_bb4_hex"],
            preparation["actor_inputs_hex"], preparation["state_hex"]]
    if args.windows:
        base = ["cmd.exe", "/c", *base]
    for stage, expected in (("preparation", prepared), ("command", written),
                            ("movement", moved)):
        argv = [*base]
        if stage != "movement":
            argv.append(f"--{stage}-only")
        result = subprocess.run(argv, capture_output=True, text=True, check=True)
        actual = bytes.fromhex(result.stdout.strip().splitlines()[-1])
        if actual != expected:
            different = [f"0x{i:02X}" for i in range(0, 0x54, 4)
                         if actual[i:i + 4] != expected[i:i + 4]]
            raise ValueError(f"{stage} output differs at {different}")
    print(json.dumps({"trace": args.trace.name,
                      "trace_sha256": hashlib.sha256(data).hexdigest(),
                      "matched_stages": ["preparation", "command", "movement"]},
                     sort_keys=True))


if __name__ == "__main__":
    main()
