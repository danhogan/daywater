"""Compare two private GNNE69 actor preparation-order captures.

The first frame may be partial because the breakpoint was armed mid-frame.
Only frames bounded by observed frame changes are compared. No game data is
printed beyond actor counts and trace hashes.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


def checked_orders(path: Path) -> tuple[str, list[tuple[str, ...]]]:
    raw = path.read_bytes()
    rows = [json.loads(line) for line in raw.splitlines()]
    if not rows or rows[0].get("event") != "ready" or rows[-1].get("event") != "done":
        raise ValueError(f"incomplete capture: {path.name}")
    done = rows[-1]
    first = done["first_frame"]
    last = done["last_frame"]
    if (not isinstance(first, int) or not isinstance(last, int) or
            last - first < 2 or done["hits"] > 150):
        raise ValueError(f"insufficient bounded frames: {path.name}")
    hits = [row for row in rows if row.get("event") == "preparation"]
    if len(hits) != done["hits"]:
        raise ValueError(f"hit count differs: {path.name}")
    thomas = rows[0]["thomas"]
    names: dict[str, str] = {}
    for index, row in enumerate(hits):
        if (row["index"] != index or row["pc"] != "0x800F705C" or
                row["lr"] != "0x800F721C" or
                row["is_thomas"] != (row["actor"] == thomas)):
            raise ValueError(f"invalid preparation boundary: {path.name}")
        name = row["name_hex"]
        if not isinstance(name, str) or len(name) != 32:
            raise ValueError(f"missing actor name bytes: {path.name}")
        actor = row["actor"]
        if actor in names and names[actor] != name:
            raise ValueError(f"actor name changed: {path.name}")
        names[actor] = name
    orders = []
    for frame in range(first + 1, last + 1):
        members = [row["actor"] for row in hits if row["dumped_frame"] == frame]
        if len(members) < 2 or len(members) != len(set(members)) or thomas not in members:
            raise ValueError(f"incomplete actor order at frame {frame}: {path.name}")
        orders.append(tuple(members))
    if len(set(orders)) != 1:
        raise ValueError(f"actor order changed within trace: {path.name}")
    return hashlib.sha256(raw).hexdigest(), orders


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("first", type=Path)
    parser.add_argument("second", type=Path)
    args = parser.parse_args()
    first_hash, first_orders = checked_orders(args.first)
    second_hash, second_orders = checked_orders(args.second)
    if first_orders[0] != second_orders[0]:
        raise ValueError("actor order differs across fresh captures")
    print(json.dumps({"first_sha256": first_hash, "second_sha256": second_hash,
                      "complete_frames_per_trace": [len(first_orders), len(second_orders)],
                      "actors_per_complete_frame": len(first_orders[0]),
                      "same_order": True}, sort_keys=True))


if __name__ == "__main__":
    main()
