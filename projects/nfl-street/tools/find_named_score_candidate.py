"""Find the FNIG-linked score-shaped record in a GNNE69 MEM1 capture.

The descriptor-to-record pointer and score bytes are observed in retained
captures. Live side-1 reads and writes were traced; side 0 remains untraced.
"""

from __future__ import annotations

import argparse
from pathlib import Path


MEM1_BASE = 0x80000000
MEM1_SIZE = 0x01800000
PREFIX_AT_MINUS_10 = bytes.fromhex("41 20 00 00")
SUFFIX_AT_PLUS_2 = bytes.fromhex("ff ff ff 00 7f")
SUFFIX_AT_PLUS_10 = bytes.fromhex("a0 00 00 00 00 20 21 e6")
SCORE_SCHEMA_AT_TAG_PLUS_70 = bytes.fromhex(
    "00 00 00 80 43 53 41 47 00 00 00 08 00 00 00 03 "
    "00 00 00 88 43 53 48 47 00 00 00 08 00 00 00 03"
)


def find_candidates(image: bytes, side_1: int | None = None, side_0: int | None = None) -> list[tuple[int, int, int, int]]:
    if len(image) != MEM1_SIZE:
        raise ValueError(f"expected {MEM1_SIZE} MEM1 bytes, got {len(image)}")
    for score in (side_1, side_0):
        if score is not None and not 0 <= score <= 255:
            raise ValueError("scores must fit in unsigned bytes")

    found: list[tuple[int, int, int, int]] = []
    start = 0
    while True:
        offset = image.find(b"FNIG", start)
        if offset < 0:
            break
        if offset % 4 == 0 and offset >= 8:
            record = int.from_bytes(image[offset - 8 : offset - 4], "big")
            pair = record - MEM1_BASE + 0x10
            if (
                0x10 <= pair <= MEM1_SIZE - 0x18
                and image[offset + 0x70 : offset + 0x90] == SCORE_SCHEMA_AT_TAG_PLUS_70
                and image[pair - 0x10 : pair - 0x0C] == PREFIX_AT_MINUS_10
                and image[pair + 2 : pair + 7] == SUFFIX_AT_PLUS_2
                and image[pair + 0x10 : pair + 0x18] == SUFFIX_AT_PLUS_10
                and (side_1 is None or image[pair] == side_1)
                and (side_0 is None or image[pair + 1] == side_0)
            ):
                found.append((MEM1_BASE + offset, record, image[pair], image[pair + 1]))
        start = offset + 1
    return found


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("capture", type=Path, help="24 MiB MEM1 image outside Git")
    parser.add_argument("--side-1", type=int, help="optional expected left score")
    parser.add_argument("--side-0", type=int, help="optional expected right score")
    args = parser.parse_args()
    records = find_candidates(args.capture.read_bytes(), args.side_1, args.side_0)
    print(f"{args.capture.name}: {len(records)} FNIG-linked structural matches")
    for descriptor, record, left, right in records:
        print(
            f"  FNIG tag 0x{descriptor:08X}; record 0x{record:08X}; "
            f"side-1 0x{record + 0x10:08X}={left}; "
            f"side-0 0x{record + 0x11:08X}={right}"
        )


if __name__ == "__main__":
    main()
