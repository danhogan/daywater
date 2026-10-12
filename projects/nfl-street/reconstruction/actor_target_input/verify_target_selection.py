"""Verify the observed GNNE69 actor-target handoff against a private live trace."""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path

DOL_SHA1 = "46e3de854998a0f66c9be8c72a1164d22cd29eb4"
CALLS = {0x8008DA60: 0x8008D764,
         0x8008D848: 0x801DE560,
         0x8008DA6C: 0x8008D864}


def dol_word(dol: bytes, address: int) -> int:
    for index in range(7):
        offset = int.from_bytes(dol[4 * index:4 * index + 4], "big")
        start = int.from_bytes(dol[0x48 + 4 * index:0x4c + 4 * index], "big")
        size = int.from_bytes(dol[0x90 + 4 * index:0x94 + 4 * index], "big")
        if start <= address < start + size:
            return int.from_bytes(dol[offset + address - start:
                                      offset + address - start + 4], "big")
    raise ValueError(f"unmapped DOL address {address:#x}")


def check_dol(dol: bytes) -> None:
    if hashlib.sha1(dol).hexdigest() != DOL_SHA1:
        raise ValueError("original DOL hash differs")
    for caller, target in CALLS.items():
        word = dol_word(dol, caller)
        if word >> 26 != 18 or word & 3 != 1:
            raise ValueError(f"{caller:#x} is not a relative call")
        displacement = word & 0x03FFFFFC
        if displacement & 0x02000000:
            displacement -= 0x04000000
        if caller + displacement != target:
            raise ValueError(f"{caller:#x} calls a different target")


def f32_sum(left: bytes, right: bytes) -> bytes:
    a = struct.unpack(">f", left)[0]
    b = struct.unpack(">f", right)[0]
    return struct.pack(">f", a + b)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--trace", required=True, type=Path)
    parser.add_argument("--dol", required=True, type=Path)
    args = parser.parse_args()
    check_dol(args.dol.read_bytes())
    raw = args.trace.read_bytes()
    rows = [json.loads(line) for line in raw.splitlines()]
    triples = [row for row in rows if row["event"] in
               ("selector_entry", "selector_return", "input_entry")]
    done = [row for row in rows if row["event"] == "done"]
    if len(done) != 1 or done[0]["captured"] != done[0]["requested"]:
        raise ValueError("incomplete selector trace")
    if len(triples) != 3 * done[0]["captured"]:
        raise ValueError("selector event count differs")
    for index in range(done[0]["captured"]):
        entry, returned, handed = triples[3 * index:3 * index + 3]
        if [row["event"] for row in (entry, returned, handed)] != \
                ["selector_entry", "selector_return", "input_entry"]:
            raise ValueError(f"handoff {index} event order differs")
        if (entry["pc"] != "0x8008D764" or entry["return_pc"] != "0x8008DA64" or
                returned["pc"] != "0x8008DA64" or handed["pc"] != "0x8008D864" or
                handed["return_pc"] != "0x8008DA70" or entry["other_slot"] != 8 or
                entry["other_pointer"] == entry["actor"] or
                returned["output_pointer"] != entry["output_pointer"] or
                handed["input_pointer"] != entry["output_pointer"] or
                handed["input_hex"] != returned["output_hex"]):
            raise ValueError(f"handoff {index} boundary differs")
        other = bytes.fromhex(entry["other_position_hex"])
        offset = bytes.fromhex(entry["source_vector_hex"])
        output = bytes.fromhex(returned["output_hex"])
        if (len(other) != 12 or len(offset) != 8 or len(output) != 12 or
                offset.hex() != "00000000c0000000" or
                output != f32_sum(other[:4], offset[:4]) +
                f32_sum(other[4:8], offset[4:8]) + other[8:12]):
            raise ValueError(f"handoff {index} target vector differs")
    print(json.dumps({"trace": args.trace.name,
                      "trace_sha256": hashlib.sha256(raw).hexdigest(),
                      "matched_handoffs": done[0]["captured"],
                      "selected_roster_slot": 8,
                      "observed_offset": [0.0, -2.0],
                      "dol_sha1": DOL_SHA1}, sort_keys=True))


if __name__ == "__main__":
    main()
