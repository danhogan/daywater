#!/usr/bin/env python3
"""Export the original slot material names and settings privately; the native runtime reads SMC1, not a DOL."""

import hashlib
import struct
from pathlib import Path

from export_gear_texture_catalog import ORIGINAL_SHA1, span
from verify_controller_checks import BASELINES, PRIVATE, private

BINDING_TABLE = 0x80276C6C
SETTINGS = 0x8028DD90
MATERIAL_LIST = 0x8028DD10
SHADERS = ((0x80293ACC, b"OnePass"), (0x80293B40, b"TwoPass"))
SLOTS, NAME_BYTES, MAX_SETTINGS = 25, 16, 64


def text(dol: bytes, pointer: int) -> bytes:
    """Return one NUL-padded original name."""
    for length in range(1, NAME_BYTES):
        if span(dol, pointer + length, 1) == b"\0":
            return span(dol, pointer, length).ljust(NAME_BYTES, b"\0")
    raise ValueError("Material name is not terminated")


def material_name(dol: bytes, slot: int) -> bytes:
    """Return one slot's material name from the 80276C6C binding table."""
    (pointer,) = struct.unpack(">I", span(dol, BINDING_TABLE + 0x20 * slot, 4))
    return text(dol, pointer)


def settings(dol: bytes) -> bytes:
    """Return the 80164DD8 default list (8028DD90): count, then per entry the name
    and the six following words (clamp byte, min/mag filter, bias, aniso, bytes)."""
    rows: list[bytes] = []
    for item in range(MAX_SETTINGS + 1):
        entry = span(dol, SETTINGS + 28 * item, 28)
        (pointer,) = struct.unpack(">I", entry[:4])
        if not pointer:
            return struct.pack(">I", len(rows)) + b"".join(rows)
        rows.append(text(dol, pointer) + entry[4:])
    raise ValueError("Material settings list is not terminated")


def descriptors(dol: bytes) -> bytes:
    """Return the 8028DD10 player material list: count, then per 0x30-byte
    descriptor the material, shader and (single) texture names."""
    count, table = struct.unpack(">II", span(dol, MATERIAL_LIST, 8))
    rows = []
    for item in range(count):
        entry = span(dol, table + 0x30 * item, 0x30)
        name, shader, textures, texture = struct.unpack(">IIhxxI", entry[:16])
        if textures != 1:
            raise ValueError("Unexpected material texture count")
        rows.append(text(dol, name) + text(dol, shader) + text(dol, texture))
    return struct.pack(">I", count) + b"".join(rows)


def shaders(dol: bytes) -> bytes:
    """Return the OnePass/TwoPass shader definitions: name, stage count and the
    20-byte stage records (name, type byte, flag byte, stage index)."""
    result = struct.pack(">I", len(SHADERS))
    for address, name in SHADERS:
        if text(dol, address).rstrip(b"\0") != name:
            raise ValueError("Unexpected shader definition")
        (stages,) = struct.unpack(">I", span(dol, address + 0x18, 4))
        result += text(dol, address) + struct.pack(">I", stages) + span(dol, address + 0x1C, 0x14 * stages)
    return result


def export(output: Path) -> str:
    """Write the SMC1 catalog and return its SHA256."""
    dol = BASELINES["dol"][0].read_bytes()
    if hashlib.sha1(dol).hexdigest() != ORIGINAL_SHA1:
        raise ValueError("Unexpected original revision")
    names = b"".join(material_name(dol, slot) for slot in range(SLOTS))
    result = b"SMC1" + struct.pack(">I", 1) + names + settings(dol) + descriptors(dol) + shaders(dol)
    output = private(output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(result)
    return hashlib.sha256(result).hexdigest()


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--output", type=Path, default=PRIVATE / "private/nflstreet/GNNE69-r0/native/slot-materials.smc1"
    )
    args = parser.parse_args()
    print("Private slot material catalog SHA256 " + export(args.output))
