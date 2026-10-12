#!/usr/bin/env python3
"""Export original gear texture-plan data privately; the native runtime reads GTC2, not a DOL."""

import hashlib
import struct
from pathlib import Path

from verify_controller_checks import BASELINES, PRIVATE, private

ORIGINAL_SHA1 = "46e3de854998a0f66c9be8c72a1164d22cd29eb4"
WINDOW_START, WINDOW_SIZE = 0x802714F0, 0x868


def span(dol: bytes, address: int, length: int) -> bytes:
    """Return original bytes for one loaded DOL address range."""
    for section in range(18):
        offset, start, size = (struct.unpack_from(">I", dol, base + 4 * section)[0] for base in (0, 0x48, 0x90))
        if start <= address and address + length <= start + size:
            at = offset + address - start
            if at + length > len(dol):
                raise ValueError("Invalid original section bounds")
            return dol[at : at + length]
    raise ValueError("Original range is not in DOL sections")


def export(output: Path) -> str:
    """Write the GTC2 catalog and return its SHA256."""
    dol = BASELINES["dol"][0].read_bytes()
    if hashlib.sha1(dol).hexdigest() != ORIGINAL_SHA1:
        raise ValueError("Unexpected original revision")
    # Window: 802714F0 default names .. 80271C58 GRSI color words; then
    # 802859F8 categories, 80285968 GEAR query tags, 80252CF8 excluded name.
    result = (
        b"GTC2"
        + struct.pack(">III", 2, WINDOW_START, WINDOW_SIZE)
        + span(dol, WINDOW_START, WINDOW_SIZE)
        + span(dol, 0x802859F8, 48)
        + span(dol, 0x80285968, 104)
        + span(dol, 0x80252CF8, 32)
    )
    output = private(output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(result)
    return hashlib.sha256(result).hexdigest()


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=PRIVATE / "private/nflstreet/GNNE69-r0/native/gear-texture.gtc2")
    args = parser.parse_args()
    print("Private gear texture catalog SHA256 " + export(args.output))
