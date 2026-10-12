"""Find candidate copies of the GNNE69 match object in a MEM1 capture.

This compares bytes only. It does not establish why a copy exists or which
object owns it. Captures stay outside the repository.
"""

import argparse
from pathlib import Path


MEM1_BASE = 0x80000000
MEM1_SIZE = 0x01800000
SCORE_POINTER_SLOT = 0x803A29C8
PREFIX_SIZE = 0x30
COMPARISON_SIZE = 0x200


def be_u16(data, offset):
    return int.from_bytes(data[offset : offset + 2], "big")


def be_u32(data, offset):
    return int.from_bytes(data[offset : offset + 4], "big")


def find_copies(data):
    if len(data) != MEM1_SIZE:
        raise ValueError(f"expected {MEM1_SIZE} MEM1 bytes, got {len(data)}")
    pointer = be_u32(data, SCORE_POINTER_SLOT - MEM1_BASE)
    source = pointer - MEM1_BASE
    if pointer % 4 or source < 0 or source + COMPARISON_SIZE > len(data):
        raise ValueError(f"invalid score object pointer: 0x{pointer:08X}")

    prefix = data[source : source + PREFIX_SIZE]
    found = []
    offset = 0
    while True:
        offset = data.find(prefix, offset)
        if offset < 0:
            break
        if offset % 4 == 0 and offset + COMPARISON_SIZE <= len(data):
            differences = sum(
                data[source + i] != data[offset + i]
                for i in range(COMPARISON_SIZE)
            )
            found.append(
                (
                    MEM1_BASE + offset,
                    be_u16(data, offset + 0x3C),
                    be_u16(data, offset + 0x3E),
                    be_u32(data, offset + 0x40),
                    differences,
                )
            )
        offset += 1
    return pointer, found


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("captures", nargs="+", type=Path, help="raw 24 MiB MEM1 captures")
    args = parser.parse_args()
    for capture in args.captures:
        pointer, found = find_copies(capture.read_bytes())
        print(f"{capture.name}: score object 0x{pointer:08X}, {len(found)} prefix matches")
        for address, right, left, field_40, differences in found:
            print(
                f"  0x{address:08X}: right={right} left={left} "
                f"+0x40={field_40} differences_in_0x200={differences}"
            )


if __name__ == "__main__":
    main()
