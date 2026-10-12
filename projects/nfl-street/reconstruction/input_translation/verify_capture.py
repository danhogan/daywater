"""Compare private live actor-input captures with the guarded native translator."""

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
    before = event(rows, "input_entry")
    after = event(rows, "input_written")
    input_hex = before["input_hex"]
    actor_hex = before["actor_inputs_hex"]
    expected = after["actor_inputs_hex"]
    if len(input_hex) != 0x40 or len(actor_hex) != 0x60 or len(expected) != 0x60:
        raise ValueError("wrong live block length")
    command = [args.executable, input_hex, actor_hex]
    if args.windows:
        command = ["cmd.exe", "/c", *command]
    result = subprocess.run(command, capture_output=True, text=True, check=True)
    actual = result.stdout.strip().splitlines()[-1].lower()
    different = [f"0x{offset:02X}" for offset, (a, b) in
                 enumerate(zip(bytes.fromhex(actual), bytes.fromhex(expected))) if a != b]
    print(json.dumps({"trace": args.trace.name,
                      "trace_sha256": hashlib.sha256(data).hexdigest(),
                      "matched": not different, "different_bytes": different}, sort_keys=True))
    if different:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
