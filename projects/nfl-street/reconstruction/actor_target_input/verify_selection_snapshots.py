"""Check bounded GNNE69 record-handle changes in private MEM1 snapshots."""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
from pathlib import Path

CASES = {
    "position-longforward-00.mem1": ("0b7bbd1b65ae90925e06c44108f6955fdaf2496fdfa57a6642bf46068d6e2011", 0, None, None),
    "position-longforward-01.mem1": ("809c3c01c2f0d6b6147d359328b86bb40cc8777aa9d56db2bdac4166b487adfc", 0x00050101, 12, "Urlacher"),
    "position-longforward-02.mem1": ("b4684f454ad7ee2a043089303dbcae1bb7d443ed022f6668629566e2729e47c1", 0x00000101, 7, "Stewart"),
    "position-longforward-07.mem1": ("73712ce6085475654f42a56c8ab4940a3c049a13496a85af5305024f9e99a6bc", 0x00010101, 8, "Thomas"),
    "position-longright-00.mem1": ("bf8d474b63b9924515e93ecab67bfba2b649cdd0e5224cdf12acd63ef69437f2", 0, None, None),
    "position-longright-01.mem1": ("2a66432522498e4c4d18002f6b6e813ed4d12383273fc2745b8ea610d7313585", 0x00050101, 12, "Urlacher"),
    "position-longright-02.mem1": ("256c0647c63bb7092e544cab5fd5f44303d49f2489698646da5ae65e0d544809", 0x00000101, 7, "Stewart"),
    "position-longright-07.mem1": ("7391c03f2b48dd92d731e10679d98a7ef8b25b99f619c7c80bd3599d859f57e5", 0x00010101, 8, "Thomas"),
}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--snapshot-root", required=True, type=Path)
    parser.add_argument("--library", required=True, type=Path)
    args = parser.parse_args()
    library = ctypes.CDLL(str(args.library.resolve()))
    select_slot = library.nflstreet_actor_slot_from_handle
    select_slot.argtypes = [ctypes.c_uint32, ctypes.c_uint16,
                            ctypes.c_uint16, ctypes.POINTER(ctypes.c_uint16)]
    select_slot.restype = ctypes.c_int
    for name, (sha256, expected_handle, expected_slot, expected_name) in CASES.items():
        memory = (args.snapshot_root / name).read_bytes()
        if len(memory) != 0x1800000 or hashlib.sha256(memory).hexdigest() != sha256:
            raise ValueError(f"snapshot identity differs: {name}")

        def data(address: int, size: int) -> bytes:
            if not 0x80000000 <= address <= 0x81800000 - size:
                raise ValueError(f"MEM1 address outside range: {address:#x}")
            return memory[address - 0x80000000:address - 0x80000000 + size]

        def u32(address: int) -> int:
            return int.from_bytes(data(address, 4), "big")

        r13 = 0x803A9A40
        manager = u32(r13 - 0x73D0)
        roster = u32(r13 - 0x7C30)
        index = data(manager + 1, 1)[0]
        record = manager + 8 + index * 0xF4
        handle = u32(record + 0xB4)
        base = u32(roster)
        per_team = int.from_bytes(data(roster + 8, 2), "big")
        slot = ctypes.c_uint16(0xFFFF)
        accepted = select_slot(handle, per_team, 2 * per_team,
                               ctypes.byref(slot))
        if (manager != 0x803B6F00 or index != 0 or per_team != 7 or
                handle != expected_handle or
                bool(accepted) != (expected_slot is not None)):
            raise ValueError(f"record or native handle result differs: {name}")
        if expected_slot is not None:
            actor = base + slot.value * 0x14D4
            observed_name = data(actor + 0xB80, 16).split(b"\0", 1)[0].decode("ascii")
            if slot.value != expected_slot or observed_name != expected_name:
                raise ValueError(f"actor identity differs: {name}")
        print(json.dumps({"snapshot": name, "index": index,
                          "handle": f"0x{handle:08X}",
                          "native_slot": slot.value if accepted else None,
                          "actor_name": expected_name if accepted else None},
                         sort_keys=True))


if __name__ == "__main__":
    main()
