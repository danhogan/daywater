"""Check researched score functions and optionally compare linked bytes.

The script reads a supplied DOL; it does not copy executable bytes into the
repository. A candidate must be relocated, raw PowerPC function bytes,
not an unlinked object file with unresolved SDA relocations.
"""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path

DOL_SHA1 = "46e3de854998a0f66c9be8c72a1164d22cd29eb4"
TARGETS = {
    "reader": (0x80104274, 32, "31ee22d7af9b9955dab8b962c614050e9d0333d84ca1808531a7aaa52806afe8"),
    "writer": (0x80104294, 52, "fdea0f82363ab97caa73b3abd6c03cb672a583f12cea6b4c5ada8534cb4aaeaf"),
    "clear": (0x801042C8, 16, "1285789de99233791d6b210a671239e912c303d856eb089e13eae8e64f6c3415"),
    "delta": (0x8002676C, 124, "618aa14b3cd00c22c34814efbf6d21536322e523c3ed8e0b70e158098a21d27b"),
    "record": (0x80025ECC, 204, "86026600d3bf3ad89e1783f215db5e8a7e899f661e13f19d892c04af22c8774d"),
    "table_get": (0x80053128, 88, "433e940737f3bb3a643f038a81bdd744e0ea8080b5c9afaf663e1e2c3bf383c5"),
    "table_set": (0x80053180, 76, "a0d9dbe790b5c6a12f128feb2b677eff22ade870996212ab35c71c7076a33f26"),
}


def original_function(dol: bytes, target_name: str) -> tuple[int, bytes]:
    if hashlib.sha1(dol).hexdigest() != DOL_SHA1:
        raise ValueError("DOL SHA-1 differs from the researched executable")
    function_address, function_size, function_sha256 = TARGETS[target_name]
    for index in range(7):
        file_offset = int.from_bytes(dol[4 * index : 4 * index + 4], "big")
        address = int.from_bytes(dol[0x48 + 4 * index : 0x4C + 4 * index], "big")
        size = int.from_bytes(dol[0x90 + 4 * index : 0x94 + 4 * index], "big")
        if address <= function_address and function_address + function_size <= address + size:
            start = file_offset + function_address - address
            target = dol[start : start + function_size]
            if hashlib.sha256(target).hexdigest() != function_sha256:
                raise ValueError("function bytes differ from the researched target")
            return start, target
    raise ValueError("function address is absent from DOL text sections")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dol", type=Path, help="original boot.dol; never copied by this tool")
    parser.add_argument("function", choices=TARGETS)
    parser.add_argument("--candidate-raw", type=Path, help="optional linked raw PowerPC candidate")
    args = parser.parse_args()

    offset, target = original_function(args.dol.read_bytes(), args.function)
    address, size, _ = TARGETS[args.function]
    print(f"{args.function} verified: 0x{address:08X}, DOL offset 0x{offset:X}, {size} bytes")
    if args.candidate_raw:
        candidate = args.candidate_raw.read_bytes()
        if len(candidate) != size:
            raise ValueError(f"candidate must contain exactly {size} relocated bytes")
        differences = [i for i, (actual, expected) in enumerate(zip(candidate, target)) if actual != expected]
        if differences:
            print(f"not matched: {len(differences)} differing bytes; first offset 0x{differences[0]:X}")
            raise SystemExit(1)
        print("byte-for-byte match")


if __name__ == "__main__":
    main()
