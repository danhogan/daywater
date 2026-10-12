"""Check the GNNE69 selection-record handle copy and clear instructions."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path

from verify_target_selection import DOL_SHA1, dol_word


WORDS = {
    0x80020988: 0x7C631B79,  # mr. r3, r3
    0x80020990: 0x80030000,  # lwz r0, 0(r3)
    0x80020994: 0x90040000,  # stw r0, 0(r4)
    0x8002099C: 0x90640000,  # stw r3, 0(r4), where r3 is zero
    0x800BE288: 0x389E00B4,  # addi r4, r30, 0xb4
    0x800BE28C: 0x38600000,  # li r3, 0
    0x800BE2C4: 0x7C9E2378,  # mr r30, r4: incoming source pointer
    0x800BE2B4: 0x9421FFF0,  # stwu r1, -0x10(r1)
    0x800BE2B8: 0x7C0802A6,  # mflr r0
    0x800BE2C0: 0x90010014,  # stw r0, 0x14(r1): saved incoming LR
    0x800BE2C8: 0x7C7F1B78,  # mr r31, r3: incoming record pointer
    0x800BE2F0: 0x7FC3F378,  # mr r3, r30
    0x800BE2F4: 0x389F00B4,  # addi r4, r31, 0xb4
}
CALLS = (0x800BE29C, 0x800BE2F8)


def check_dol(path: Path) -> None:
    dol = path.read_bytes()
    if hashlib.sha1(dol).hexdigest() != DOL_SHA1:
        raise ValueError("DOL hash differs from GNNE69 revision 0")
    for address, expected in WORDS.items():
        if dol_word(dol, address) != expected:
            raise ValueError(f"{address:#x} instruction differs")
    for caller in CALLS:
        word = dol_word(dol, caller)
        if word >> 26 != 18 or word & 3 != 1:
            raise ValueError(f"{caller:#x} is not a relative call")
        displacement = word & 0x03FFFFFC
        if displacement & 0x02000000:
            displacement -= 0x04000000
        if caller + displacement != 0x80020988:
            raise ValueError(f"{caller:#x} calls a different target")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dol", required=True, type=Path)
    args = parser.parse_args()
    check_dol(args.dol)
    print("verified: DOL handle clear and source-copy instructions")


if __name__ == "__main__":
    main()
