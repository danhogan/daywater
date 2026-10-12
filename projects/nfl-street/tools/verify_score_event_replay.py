"""Compare a private Dolphin score trace with the native score replay program."""

from __future__ import annotations

import argparse
import json
import subprocess
from pathlib import Path


def exactly_one(events: list[dict], kind: str, pc: str | None = None) -> dict:
    matches = [event for event in events if event.get("event") == kind
               and (pc is None or event.get("pc") == pc)]
    if len(matches) != 1:
        raise ValueError(f"expected one {kind} {pc or ''}, found {len(matches)}")
    return matches[0]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--trace", required=True, type=Path)
    parser.add_argument("--executable", required=True,
                        help="native replay executable, as a Windows path with --windows")
    parser.add_argument("--windows", action="store_true",
                        help="invoke the Windows executable through cmd.exe")
    args = parser.parse_args()

    events = [json.loads(line) for line in args.trace.read_text().splitlines()]
    record = exactly_one(events, "score_record_entry")
    entry = exactly_one(events, "breakpoint", "0x80025ECC")
    writer = exactly_one(events, "breakpoint", "0x80104294")
    done = exactly_one(events, "done")
    command = [args.executable, record["raw"], str(record["control"]),
               str(entry["side_0"]), str(entry["side_1"])]
    if args.windows:
        command = ["cmd.exe", "/c", *command]
    run = subprocess.run(command, capture_output=True, text=True, check=True)
    native = json.loads(run.stdout.strip().splitlines()[-1])
    for key in ("side_0", "side_1"):
        if native[key] != done[key]:
            raise AssertionError(f"{key}: native {native[key]} != Dolphin {done[key]}")
    side = writer["r3"]
    if side not in (0, 1) or native[f"side_{side}"] != writer["r4"]:
        raise AssertionError("native score differs from the original writer argument")
    print(json.dumps({"trace": args.trace.name, "matched": True,
                      "side_0": native["side_0"], "side_1": native["side_1"],
                      "named_side_0": native["named_side_0"],
                      "named_side_1": native["named_side_1"],
                      "score_event_updates": native["updates"]}, sort_keys=True))


if __name__ == "__main__":
    main()
