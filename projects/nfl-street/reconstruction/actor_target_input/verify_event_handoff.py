"""Verify native descriptor -> event -> incoming actor -> record changes."""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
from pathlib import Path

from verify_handle_calls import verify
from verify_handle_writer import check_dol
from verify_record_handoff import WORDS as RECORD_WORDS
from verify_target_selection import dol_word

WORDS = {
    0x8007C014: 0x38000000, 0x8007C018: 0x3BBF0150,
    0x8007C01C: 0xB01D0004, 0x8007C020: 0x813F0310,
    0x8007C024: 0x88090002, 0x8007C028: 0x901D0008,
    0x8007C02C: 0x813F0310, 0x8007C030: 0x88090003,
    0x8007C034: 0x981D0006, 0x8007C038: 0x813F0310,
    0x8007C03C: 0x887F0002, 0x8007C040: 0x88890001,
    0x8007C044: 0x4BFBE45D, 0x8007C048: 0x7C7E1B78,
    0x8007C04C: 0x7FA4EB78, 0x8007C050: 0x4BFA4939,
    0x8007C150: 0x801F000C, 0x8007C154: 0x39200000,
    0x8007C158: 0x993F020E, 0x8007C15C: 0x38600000,
    0x8007C160: 0x540007B8, 0x8007C164: 0x5400003C,
    0x8007C168: 0x901F000C,
}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dol", required=True, type=Path)
    parser.add_argument("--trace", required=True, type=Path)
    parser.add_argument("--executable", required=True, type=Path)
    parser.add_argument("--windows", action="store_true")
    args = parser.parse_args()
    check_dol(args.dol)
    dol = args.dol.read_bytes()
    for pc, word in {**WORDS, **RECORD_WORDS}.items():
        if dol_word(dol, pc) != word:
            raise ValueError(f"original event/handoff instruction differs at {pc:#x}")
    report = verify(args.trace)
    if report["incomplete_call"] or report["completed_calls"] != 3:
        raise ValueError("need three complete original handle writes")
    rows = [json.loads(line) for line in args.trace.read_text().splitlines()]
    ready = rows[0]
    per_team, roster = ready["actors_per_team"], ready["roster_handles"]
    if len(roster) != 2 * per_team:
        raise ValueError("roster size differs")
    handoffs = [row for row in rows if row["event"] == "actor_handoff"]
    if len(handoffs) != 2:
        raise ValueError("need the two observed actor handoffs")
    handles = list(handoffs[0]["record_handles"])
    command = [str(args.executable), str(per_team), str(len(roster)), str(len(handles)),
               *map(hex, handles), *map(hex, roster)]
    expected, built = [], {}
    builds = 0
    for i, row in enumerate(rows):
        if row["event"] == "event_build_entry":
            prefix, returned = rows[i + 1:i + 3]
            if (prefix["event"] != "event_build_prefix" or returned["event"] != "event_build_return" or
                    prefix["actor"] != row["actor"] or returned["actor"] != row["actor"] or
                    prefix["event_hex"] != returned["event_hex"] or
                    [row["pc"], prefix["pc"], returned["pc"]] != [0x8007C000, 0x8007C054, 0x8007C16C]):
                raise ValueError("event builder lacks the observed complete boundaries")
            slot, remainder = divmod(row["actor"] - ready["roster_base"], 0x14D4)
            if remainder or not 0 <= slot < len(roster):
                raise ValueError("event actor outside roster")
            event = bytes.fromhex(prefix["event_hex"])
            source_slot, remainder = divmod(prefix["source_actor"] - ready["roster_base"], 0x14D4)
            if (remainder or not 0 <= source_slot < len(roster) or
                    int.from_bytes(event[:4], "big") != prefix["source_handle"] or
                    prefix["source_handle"] != roster[source_slot]):
                raise ValueError("event handle differs from resolved roster actor")
            command.extend(["B", row["header_hex"], row["descriptor_hex"], row["event_hex"]])
            expected.append(f"event {slot} {returned['event_hex']} {returned['flags']:08x} {returned['input_mode']}")
            built[row["actor"]] = event
            builds += 1
        elif row["event"] == "actor_handoff":
            if row["current_actor"] not in built or row["record_handles"] != handles:
                raise ValueError("handoff has no constructed event or record state changed")
            event = built[row["current_actor"]]
            if (int.from_bytes(event[:4], "big") != row["new_handle"] or
                    int.from_bytes(event[8:12], "big") != row["event_code"] or
                    row["event_pointer"] != row["current_actor"] + 0x150 or row["event_type"] != 3):
                raise ValueError("constructed event does not feed this handoff")
            call, returned = rows[i + 1:i + 3]
            if (call["event"] != "handle_call" or returned["event"] != "handle_return" or
                    call["source"] != row["new_actor"] or call["source_value"] != row["new_handle"] or
                    call["destination"] != row["record"] + 0xB4 or
                    returned["after"] != row["new_handle"]):
                raise ValueError("handoff does not feed original record write")
            index, remainder = divmod(row["record"] - 0x803B6F08, 0xF4)
            if remainder or not 0 <= index < len(handles):
                raise ValueError("handoff record outside manager")
            handles[index] = returned["after"]
            command.extend(["H", hex(row["current_handle"])])
            expected.append(f"record {index} " + " ".join(f"{handle:08x}" for handle in handles))
    if args.windows:
        exe = subprocess.run(["wslpath", "-w", str(args.executable.resolve())],
                             text=True, capture_output=True, check=True).stdout.strip()
        command = ["cmd.exe", "/c", exe, *command[1:]]
    result = subprocess.run(command, text=True, capture_output=True, check=True,
                            cwd=args.executable.resolve().parent, timeout=15)
    if result.stdout.splitlines() != expected:
        raise ValueError("native events/flags/input mode or record changes differ from original")
    print(json.dumps({"native_events_matched": builds, "native_handoffs_matched": len(handoffs),
                      "trace_sha256": hashlib.sha256(args.trace.read_bytes()).hexdigest()}, sort_keys=True))


if __name__ == "__main__":
    main()
