"""Check the GNNE69 direct index store and one private watchpoint trace."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

from verify_target_selection import DOL_SHA1, dol_word

WORDS = {
    0x800BDD74: 0x7C7F1B78,  # mr r31, r3
    0x800BDD90: 0x816D8C30,  # lwz r11, -0x73d0(r13)
    0x800BDD98: 0x9BEB0001,  # stb r31, 1(r11)
    0x8000AFC4: 0x887D059D,  # lbz r3, 0x59d(r29)
    0x800BDF08: 0x38600000,  # li r3, 0
}
CALLS = {
    0x8000AFC8: 0x800BDD64,
    0x800BDF0C: 0x800BDD64,
}


def check_dol(path: Path) -> None:
    dol = path.read_bytes()
    if hashlib.sha1(dol).hexdigest() != DOL_SHA1:
        raise ValueError("DOL hash differs from GNNE69 revision 0")
    for address, expected in WORDS.items():
        if dol_word(dol, address) != expected:
            raise ValueError(f"{address:#x} instruction differs")
    for caller, target in CALLS.items():
        word = dol_word(dol, caller)
        if word >> 26 != 18 or word & 3 != 1:
            raise ValueError(f"{caller:#x} is not a relative call")
        displacement = word & 0x03FFFFFC
        if displacement & 0x02000000:
            displacement -= 0x04000000
        if caller + displacement != target:
            raise ValueError(f"{caller:#x} calls a different target")
    found = set()
    for section in range(7):
        offset = int.from_bytes(dol[4 * section:4 * section + 4], "big")
        base = int.from_bytes(dol[0x48 + 4 * section:
                                  0x4C + 4 * section], "big")
        size = int.from_bytes(dol[0x90 + 4 * section:
                                  0x94 + 4 * section], "big")
        for index in range(0, size, 4):
            word = int.from_bytes(dol[offset + index:offset + index + 4], "big")
            if word >> 26 != 18 or word & 3 != 1:
                continue
            displacement = word & 0x03FFFFFC
            if displacement & 0x02000000:
                displacement -= 0x04000000
            address = base + index
            if address + displacement == 0x800BDD64:
                found.add(address)
    if found != set(CALLS):
        raise ValueError("direct index-writer call sites differ")


def check_trace(path: Path) -> None:
    raw = path.read_bytes()
    rows = [json.loads(line) for line in raw.splitlines()]
    if len(rows) < 2 or rows[0].get("event") != "ready" or \
            rows[-1].get("event") != "done":
        raise ValueError("incomplete watchpoint trace")
    ready, done = rows[0], rows[-1]
    if (ready["index_address"] != "0x803B6F01" or
            ready["initial_index"] != 0 or
            ready["savestate_sha256"] !=
            "ea3d1fa29797d1521b2ab83aee866c190ed092865b7739ceb537d15aba5e7f32"):
        raise ValueError("watchpoint state differs")
    if done["hit"]:
        hits = [row for row in rows if row.get("event") == "write_hit"]
        if len(hits) != 1 or f"{int(ready['index_address'], 16):08x}" not in \
                hits[0]["reply"].lower():
            raise ValueError("write hit lacks watchpoint evidence")
        outcome = "write_hit"
    else:
        if len(rows) != 2 or done["final_index"] != 0:
            raise ValueError("no-hit watchpoint trace changed state")
        outcome = "no_write_observed"
    print(json.dumps({"trace": path.name,
                      "trace_sha256": hashlib.sha256(raw).hexdigest(),
                      "outcome": outcome}, sort_keys=True))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dol", required=True, type=Path)
    parser.add_argument("traces", nargs="+", type=Path)
    args = parser.parse_args()
    check_dol(args.dol)
    for trace in args.traces:
        check_trace(trace)


if __name__ == "__main__":
    main()
