"""Check the observed GNNE69 Thomas update order and original DOL call sites.

Reads a private live trace and original boot.dol; prints only a compact summary.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path

DOL_SHA1 = "46e3de854998a0f66c9be8c72a1164d22cd29eb4"
CALLS = {
    0x8003E858: 0x8003DDEC,
    0x800F870C: 0x800F7C90,
    0x800F8740: 0x80036B28,
}
PREPARATION_INDIRECT_CALL = 0x800F7218
EVENT_PCS = {
    "preparation": "0x800F705C",
    "movement_entry": "0x80036B28",
    "movement_exit": "0x80036E58",
    "input_entry": "0x8003DDEC",
    "input_written": "0x8003E85C",
    "reset_entry": "0x800F7C90",
    "reset_return": "0x800F8710",
}


def dol_word(dol: bytes, address: int) -> int:
    for index in range(7):
        offset = int.from_bytes(dol[4 * index:4 * index + 4], "big")
        start = int.from_bytes(dol[0x48 + 4 * index:0x4c + 4 * index], "big")
        size = int.from_bytes(dol[0x90 + 4 * index:0x94 + 4 * index], "big")
        if start <= address < start + size:
            file_offset = offset + address - start
            return int.from_bytes(dol[file_offset:file_offset + 4], "big")
    raise ValueError(f"DOL does not map {address:#x}")


def checked_call_sites(dol: bytes) -> None:
    if hashlib.sha1(dol).hexdigest() != DOL_SHA1:
        raise ValueError("original DOL SHA-1 differs")
    for caller, target in CALLS.items():
        word = dol_word(dol, caller)
        if word >> 26 != 18 or word & 3 != 1:
            raise ValueError(f"{caller:#x} is not a relative branch-and-link")
        displacement = word & 0x03FFFFFC
        if displacement & 0x02000000:
            displacement -= 0x04000000
        if caller + displacement != target:
            raise ValueError(f"{caller:#x} calls the wrong target")
    if dol_word(dol, PREPARATION_INDIRECT_CALL) != 0x4E800021:
        raise ValueError("preparation's return site is not bctrl")


def frame(row: dict) -> tuple[int, int]:
    count = row.get("dumped_frames")
    name = row.get("latest_frame")
    if not isinstance(count, int) or not isinstance(name, str):
        raise ValueError("trace lacks dumped-frame evidence")
    match = re.fullmatch(r"framedump_(\d+)\.png", name)
    if not match:
        raise ValueError("unexpected frame name")
    return count, int(match.group(1))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--trace", type=Path, required=True)
    parser.add_argument("--dol", type=Path, required=True)
    args = parser.parse_args()
    checked_call_sites(args.dol.read_bytes())
    raw = args.trace.read_bytes()
    rows = [json.loads(line) for line in raw.splitlines()]
    done = [row for row in rows if row.get("event") == "done"]
    if len(done) != 1 or done[0]["captured"] != done[0]["requested"]:
        raise ValueError("trace is incomplete")
    updates = done[0]["captured"]
    if not 2 <= updates <= 8:
        raise ValueError("unexpected update count")
    relevant = [row for row in rows if row.get("event") in EVENT_PCS]
    names = [row["event"] for row in relevant]
    expected = ["preparation", "movement_entry", "movement_exit"]
    for _ in range(1, updates):
        expected.extend(("input_entry", "input_written", "reset_entry",
                         "reset_return", "preparation", "movement_entry",
                         "movement_exit"))
    if names != expected:
        raise ValueError("unexpected call order")
    for row in relevant:
        if row["pc"] != EVENT_PCS[row["event"]]:
            raise ValueError(f"unexpected {row['event']} PC")
    preparations = [row for row in relevant if row["event"] == "preparation"]
    movements = [row for row in relevant if row["event"] in
                 ("movement_entry", "movement_exit")]
    inputs = [row for row in relevant if row["event"] == "input_entry"]
    resets = [row for row in relevant if row["event"] == "reset_entry"]
    if any(row.get("lr") != "0x800F721C" for row in preparations):
        raise ValueError("unexpected preparation return address")
    if any(row.get("lr") != "0x800F8744" for row in movements):
        raise ValueError("unexpected movement return address")
    if any(row.get("lr") != "0x8003E85C" for row in inputs):
        raise ValueError("unexpected input return address")
    if any(row.get("lr") != "0x800F8710" for row in resets):
        raise ValueError("unexpected reset return address")
    for index, prep in enumerate(preparations):
        if prep["index"] != index or frame(prep) != frame(movements[2 * index]):
            raise ValueError("movement entry crosses a frame boundary")
        if frame(prep) != frame(movements[2 * index + 1]):
            raise ValueError("movement exit crosses a frame boundary")
        if index:
            previous_count, previous_name = frame(preparations[index - 1])
            current_count, current_name = frame(prep)
            if (current_count - previous_count, current_name - previous_name) != (1, 1):
                raise ValueError("consecutive updates did not span one dumped frame")
            if frame(inputs[index - 1]) != frame(prep) or frame(resets[index - 1]) != frame(prep):
                raise ValueError("input or reset crossed a frame boundary")
    print(json.dumps({"trace": args.trace.name,
                      "trace_sha256": hashlib.sha256(raw).hexdigest(),
                      "dol_sha1": DOL_SHA1,
                      "matched_updates": updates,
                      "matched_intercall_resets": len(resets),
                      "dumped_frame_range": [frame(preparations[0])[1],
                                             frame(preparations[-1])[1]],
                      "dumped_frames_per_update": 1}, sort_keys=True))


if __name__ == "__main__":
    main()
