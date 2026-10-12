"""Compare bounded live runner probes with the two portable C candidates.

Compile both candidate C files into one Linux shared library first, then pass
that library and the private W: probe log to this script.
"""

from __future__ import annotations

import argparse
import ctypes
import re
import struct
from pathlib import Path


PROBE = re.compile(
    r"\[staticrecomp\] pair-probe pc=([0-9a-f]{8}) seq=(\d+) "
    r"input=([0-9a-f,]+) output=([0-9a-f,]+)"
)


def bits_to_float(bits: int) -> float:
    return struct.unpack(">f", bits.to_bytes(4, "big"))[0]


def float_to_bits(value: float) -> int:
    return int.from_bytes(struct.pack(">f", value), "big")


def signed16(value: int) -> int:
    value &= 0xFFFF
    return value - 0x10000 if value & 0x8000 else value


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    parser.add_argument("library", type=Path)
    args = parser.parse_args()
    module = ctypes.CDLL(str(args.library.resolve()))
    float_pointer = ctypes.POINTER(ctypes.c_float)
    module.nflstreet_odd_polynomial_pair.argtypes = [
        float_pointer, float_pointer, float_pointer
    ]
    module.nflstreet_paired_product.argtypes = [
        float_pointer, float_pointer, float_pointer, float_pointer
    ]
    counts = {"80188bb0": 0, "801ffdf0": 0}
    mismatches = []
    for match in PROBE.finditer(args.log.read_text(encoding="utf-8")):
        pc = match.group(1)
        values = [int(word, 16) for word in match.group(3).split(",")]
        actual = [int(word, 16) for word in match.group(4).split(",")]
        counts[pc] += 1
        if pc == "80188bb0":
            if len(values) != 9 or len(actual) != 2 or values[1:3] != [0, 0x0F070007]:
                mismatches.append((pc, match.group(2), "unexpected GQR/configuration"))
                continue
            raw = values[0]
            t = (ctypes.c_float * 2)(
                signed16(raw) / 32768.0,
                signed16(raw + 0x4000) / 32768.0,
            )
            coefficients = (ctypes.c_float * 6)(
                *(bits_to_float(word) for word in values[3:])
            )
            result = (ctypes.c_float * 2)()
            module.nflstreet_odd_polynomial_pair(t, coefficients, result)
        elif pc == "801ffdf0":
            if len(values) != 27 or len(actual) != 12 or values[0] != 0:
                mismatches.append((pc, match.group(2), "unexpected GQR/configuration"))
                continue
            inputs = (ctypes.c_float * 12)(
                *(bits_to_float(word) for word in values[1:13])
            )
            factors = (ctypes.c_float * 12)(
                *(bits_to_float(word) for word in values[13:25])
            )
            constant = (ctypes.c_float * 2)(
                *(bits_to_float(word) for word in values[25:])
            )
            result = (ctypes.c_float * 12)()
            module.nflstreet_paired_product(inputs, factors, constant, result)
        else:
            continue
        candidate = [float_to_bits(word) for word in result]
        if candidate != actual:
            differing = [i for i, (a, b) in enumerate(zip(actual, candidate)) if a != b]
            mismatches.append((pc, match.group(2), differing[:5]))
    print(f"probe_counts={counts} mismatches={len(mismatches)}")
    for item in mismatches[:10]:
        print(f"mismatch={item}")
    if not all(counts.values()):
        return 2
    return 1 if mismatches else 0


if __name__ == "__main__":
    raise SystemExit(main())
