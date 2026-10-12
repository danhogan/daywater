"""Verify the observed other-actor choice call and target handoff."""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import struct
from pathlib import Path

from verify_target_selection import DOL_SHA1, dol_word

CALLS = {
    0x8008D99C: 0x800BE3F0,
    0x800BE3FC: 0x800BDDC4,
    0x800BE400: 0x800BE380,
    0x800BE39C: 0x80020944,
    0x8002096C: 0x8003A4A0,
}
WORDS = {
    0x800BDDC4: 0x812D8C30,  # lwz r9, -0x73d0(r13)
    0x800BDDC8: 0x88690001,  # lbz r3, 1(r9)
    0x800BDDCC: 0x1C6300F4,  # mulli r3, r3, 0xf4
    0x800BDDD0: 0x38630008,  # addi r3, r3, 8
    0x800BDDD4: 0x7C691A14,  # add r3, r9, r3
    0x800BE398: 0x386300B4,  # addi r3, r3, 0xb4
    0x80020950: 0x80030000,  # lwz r0, 0(r3)
    0x80020958: 0x5404863E,  # handle byte -> r4
    0x8002095C: 0x5403C63E,  # handle byte -> r3
    0x80020960: 0x5400063E,  # low handle byte
    0x80020964: 0x2C000001,  # tag 1
    0x8003A4A0: 0x812D83D0,  # roster manager at r13-0x7c30
    0x8003A4B4: 0xA0090008,  # actors per team
    0x8003A4B8: 0x81290000,  # roster base
    0x8003A4BC: 0x7C0B01D6,  # team * count
    0x8003A4C0: 0x7C002214,  # plus member
    0x8003A4C4: 0x1C0014D4,  # actor stride
    0x8003A4C8: 0x7C690214,  # final pointer
}
MEM1_SHA256 = "8c0152c92796e9783674be81703025ede544fcb71c1d62fe84ced9680facdd0a"


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


def verify(path: Path, select_slot: object | None = None) -> None:
    raw = path.read_bytes()
    rows = [json.loads(line) for line in raw.splitlines()]
    if not rows or rows[0].get("event") != "ready":
        raise ValueError("missing roster check")
    ready = rows[0]
    done = rows[-1]
    trace_internals = len(rows) > 2 and rows[2].get("event") == "record_return"
    width = 9 if trace_internals else 5
    if (ready["actor_slot"] != 7 or
            bytes.fromhex(ready["actor_name_hex"]) != b"Stewart" or
            done.get("event") != "done" or
            done.get("captured") != done.get("requested") or
            not 1 <= done["captured"] <= 8 or
            len(rows) != 2 + done["captured"] * width):
        raise ValueError("incomplete choice trace")
    for index in range(done["captured"]):
        group = rows[1 + index * width:1 + (index + 1) * width]
        expected = (["choice_call", "record_return", "lookup_call",
                     "lookup_return", "resolver_return", "choice_return",
                     "selector_entry", "selector_return", "input_entry"]
                    if trace_internals else
                    ["choice_call", "choice_return", "selector_entry",
                     "selector_return", "input_entry"])
        if [row["event"] for row in group] != expected:
            raise ValueError(f"call {index} event order differs")
        if trace_internals:
            (call, record, lookup_call, lookup_return, resolver_return,
             choice, entry, returned, input_entry) = group
            if (record["pc"] != "0x800BE400" or
                    lookup_call["pc"] != "0x800BE39C" or
                    lookup_return["pc"] != "0x800BE3A0" or
                    resolver_return["pc"] != "0x800BE404" or
                    record["record_pointer"] != entry["selection_record"] or
                    int(lookup_call["handle_pointer"], 16) !=
                    int(record["record_pointer"], 16) + 0xB4 or
                    lookup_call["handle"] != entry["selection_handle"] or
                    not lookup_return["other_pointer"] ==
                    resolver_return["other_pointer"] ==
                    choice["other_pointer"]):
                raise ValueError(f"call {index} internal lookup differs")
        else:
            call, choice, entry, returned, input_entry = group
        if (call["pc"] != "0x8008D99C" or
                choice["pc"] != "0x8008D9A0" or
                entry["pc"] != "0x8008D764" or
                not entry["return_pc"] == returned["pc"] == "0x8008DA64" or
                input_entry["pc"] != "0x8008D864" or
                input_entry["return_pc"] != "0x8008DA70" or
                not call["actor"] == entry["actor"] == ready["actor"] or
                not choice["other_pointer"] == entry["other_pointer"] ==
                ready["thomas"] or
                entry["other_slot"] != 8 or
                bytes.fromhex(entry["other_name_hex"]) != b"Thomas" or
                not entry["output_pointer"] == returned["output_pointer"] ==
                input_entry["input_pointer"] or
                returned["output_hex"] != input_entry["input_hex"]):
            raise ValueError(f"call {index} pointer handoff differs")
        position = bytes.fromhex(entry["other_position_hex"])
        target = bytes.fromhex(returned["output_hex"])
        if (len(position) != 12 or len(target) != 12 or
                entry["other_position_hex"] != returned["other_position_hex"] or
                entry["source_vector_hex"] != "00000000c0000000"):
            raise ValueError(f"call {index} vector source differs")
        x, y, z = struct.unpack(">fff", position)
        if struct.unpack(">fff", target) != (x, y - 2.0, z):
            raise ValueError(f"call {index} target vector differs")
        if (entry["small_data"] != "0x803A9A40" or
                entry["selection_manager"] != "0x803B6F00" or
                entry["selection_index"] != 0 or
                int(entry["selection_record"], 16) !=
                int(entry["selection_manager"], 16) + 8 +
                entry["selection_index"] * 0xF4):
            raise ValueError(f"call {index} selection record differs")
        handle = int(entry["selection_handle"], 16)
        per_team = ready["roster_count"]
        slot = ((handle >> 8) & 0xFF) * per_team + ((handle >> 16) & 0xFF)
        if ((handle & 0xFF) != 1 or slot >= 2 * per_team or
                slot != entry["other_slot"] or
                int(ready["roster_base"], 16) + slot * 0x14D4 !=
                int(choice["other_pointer"], 16)):
            raise ValueError(f"call {index} handle-to-actor lookup differs")
        if select_slot is not None:
            native_slot = ctypes.c_uint16(0xFFFF)
            if (select_slot(handle, per_team, 2 * per_team,
                            ctypes.byref(native_slot)) != 1 or
                    native_slot.value != slot):
                raise ValueError(f"call {index} native handle lookup differs")
            rejected_slot = ctypes.c_uint16(0xFFFF)
            if (select_slot(handle & ~0xFF, per_team, 2 * per_team,
                            ctypes.byref(rejected_slot)) != 0 or
                    rejected_slot.value != 0xFFFF):
                raise ValueError(f"call {index} invalid tag changed native state")
    print(json.dumps({"trace": path.name,
                      "trace_sha256": hashlib.sha256(raw).hexdigest(),
                      "matched_choices": done["captured"],
                      "observed_other_slot": 8}, sort_keys=True))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dol", required=True, type=Path)
    parser.add_argument("--mem1", type=Path,
                        help="private saved-match MEM1 reference")
    parser.add_argument("--library", type=Path,
                        help="compiled native target-selection library")
    parser.add_argument("traces", nargs="+", type=Path)
    args = parser.parse_args()
    check_dol(args.dol)
    if args.mem1:
        memory = args.mem1.read_bytes()
        if hashlib.sha256(memory).hexdigest() != MEM1_SHA256:
            raise ValueError("MEM1 reference hash differs")
        def be32(address: int) -> int:
            return int.from_bytes(memory[address - 0x80000000:
                                         address - 0x80000000 + 4], "big")
        def be16(address: int) -> int:
            return int.from_bytes(memory[address - 0x80000000:
                                         address - 0x80000000 + 2], "big")
        small_data = 0x803A9A40
        selection_manager = be32(small_data - 0x73D0)
        roster_manager = be32(small_data - 0x7C30)
        index = memory[selection_manager - 0x80000000 + 1]
        record = selection_manager + 8 + index * 0xF4
        handle = be32(record + 0xB4)
        base = be32(roster_manager)
        count = be16(roster_manager + 8)
        slot = ((handle >> 8) & 0xFF) * count + ((handle >> 16) & 0xFF)
        if ((selection_manager, index, record, handle, base, count, slot,
             base + slot * 0x14D4) !=
            (0x803B6F00, 0, 0x803B6F08, 0x00010101,
             0x803B7320, 7, 8, 0x803C19C0)):
            raise ValueError("MEM1 actor selection differs from live choice")
    select_slot = None
    if args.library:
        library = ctypes.CDLL(str(args.library.resolve()))
        select_slot = library.nflstreet_actor_slot_from_handle
        select_slot.argtypes = [ctypes.c_uint32, ctypes.c_uint16,
                                ctypes.c_uint16, ctypes.POINTER(ctypes.c_uint16)]
        select_slot.restype = ctypes.c_int
    for path in args.traces:
        verify(path, select_slot)


if __name__ == "__main__":
    main()
