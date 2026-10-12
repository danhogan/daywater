"""Verify the observed selector-to-input handoff in private trace files.

This checks a narrow GNNE69 slot-7 branch, not general target selection.
The traces contain game RAM and must remain outside Git.
"""

from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path


def verify(path: Path) -> None:
    rows = [json.loads(line) for line in path.read_text().splitlines()]
    assert [row["event"] for row in rows] == [
        "ready", "selector_entry", "selector_return", "input_entry", "done"
    ]
    ready, entry, returned, consumed, done = rows
    assert ready["actor_slot"] == 7
    assert bytes.fromhex(ready["actor_name_hex"]) == b"Stewart"
    assert entry["pc"] == "0x8008D764"
    assert entry["return_pc"] == returned["pc"] == "0x8008DA64"
    assert entry["actor"] == ready["actor"]
    assert entry["other_pointer"] == ready["thomas"]
    assert entry["other_slot"] == 8
    assert bytes.fromhex(entry["other_name_hex"]) == b"Thomas"
    assert entry["r5"] == f"0x{int(ready['actor'], 16) + 0x154:08X}"
    assert returned["output_pointer"] == entry["output_pointer"]
    assert consumed["pc"] == "0x8008D864"
    assert consumed["return_pc"] == "0x8008DA70"
    assert consumed["input_pointer"] == returned["output_pointer"]
    assert consumed["input_hex"] == returned["output_hex"]
    assert done["captured"] is True

    before = bytes.fromhex(entry["other_position_hex"])
    after = bytes.fromhex(returned["other_position_hex"])
    target = bytes.fromhex(returned["output_hex"])
    assert len(before) == len(after) == len(target) == 12
    assert before == after  # Other actor did not move during this call.
    ox, oy, oz = struct.unpack(">fff", after)
    tx, ty, tz = struct.unpack(">fff", target)
    assert (tx, ty, tz) == (ox, oy - 2.0, oz)
    print(f"{path.name}: slot-7 handoff and observed -2.0 Y offset verified")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("traces", nargs="+", type=Path)
    for path in parser.parse_args().traces:
        verify(path)


if __name__ == "__main__":
    main()
