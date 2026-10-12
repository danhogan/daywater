"""Compare the bounded native target composer with private live handoffs."""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--library", required=True, type=Path)
    parser.add_argument("traces", nargs="+", type=Path)
    args = parser.parse_args()
    library = ctypes.CDLL(str(args.library.resolve()))
    compose = library.nflstreet_observed_actor_target
    byte_pointer = ctypes.POINTER(ctypes.c_uint8)
    compose.argtypes = [byte_pointer, byte_pointer, byte_pointer]
    compose.restype = ctypes.c_int
    total = 0
    for path in args.traces:
        raw = path.read_bytes()
        rows = [json.loads(line) for line in raw.splitlines()]
        entries = [row for row in rows if row["event"] == "selector_entry"]
        returns = [row for row in rows if row["event"] == "selector_return"]
        handoffs = [row for row in rows if row["event"] == "input_entry"]
        done = [row for row in rows if row["event"] == "done"]
        if (len(done) != 1 or not len(entries) == len(returns) == len(handoffs) or
                not entries or done[0]["captured"] != len(entries)):
            raise ValueError(f"incomplete target trace: {path}")
        for entry, returned, handed in zip(entries, returns, handoffs):
            if (entry["other_slot"] != 8 or entry["pc"] != "0x8008D764" or
                    returned["pc"] != "0x8008DA64" or
                    handed["pc"] != "0x8008D864" or
                    handed["input_hex"] != returned["output_hex"]):
                raise ValueError("unexpected target handoff")
            position = bytes.fromhex(entry["other_position_hex"])
            offset = bytes.fromhex(entry["source_vector_hex"])
            if len(position) != 12 or len(offset) != 8:
                raise ValueError("invalid input size")
            source = (ctypes.c_uint8 * 12).from_buffer_copy(position)
            displacement = (ctypes.c_uint8 * 8).from_buffer_copy(offset)
            output = (ctypes.c_uint8 * 12)()
            if compose(source, displacement, output) != 1 or \
                    bytes(output).hex() != returned["output_hex"]:
                raise ValueError("native target differs from original")
            unsupported = (ctypes.c_uint8 * 8).from_buffer_copy(
                bytes.fromhex("00000000bf800000"))
            untouched = (ctypes.c_uint8 * 12)(*([0xA5] * 12))
            if compose(source, unsupported, untouched) != 0 or \
                    bytes(untouched) != bytes([0xA5] * 12):
                raise ValueError("unobserved offset changed native state")
            total += 1
        print(json.dumps({"trace": path.name, "trace_sha256":
                          hashlib.sha256(raw).hexdigest(),
                          "native_targets_matched": len(entries)}, sort_keys=True))
    print(f"matched {total} native targets")


if __name__ == "__main__":
    main()
