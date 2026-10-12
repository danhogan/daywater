"""Verify consecutive native actor updates and the observed intercall handoff."""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
from pathlib import Path


def indexed(rows: list[dict], name: str, index: int) -> dict:
    found = [row for row in rows if row.get("event") == name and row.get("index") == index]
    if len(found) != 1:
        raise ValueError(f"expected one {name} event for update {index}")
    return found[0]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--trace", required=True, type=Path)
    parser.add_argument("--loop-executable", required=True)
    parser.add_argument("--single-executable", required=True)
    parser.add_argument("--reset-executable",
                        help="verify and use the calculated 0x800F7C90 intercall reset")
    parser.add_argument("--windows", action="store_true")
    parser.add_argument("--varying-input", action="store_true",
                        help="pass each live actor input block to the native loop")
    parser.add_argument("--stick-path",
                        help="bounded native cardinal path, e.g. RFF; requires varying input")
    parser.add_argument("--vector-path",
                        help="run the native 0x20-byte vector bridge for R/F/L path")
    parser.add_argument("--vector-from-trace", action="store_true",
                        help="use later original translator inputs and infer the initial cardinal vector")
    parser.add_argument("--translator-executable",
                        help="also verify each captured original input-function return")
    parser.add_argument("--target-from-trace", action="store_true",
                        help="drive later native calls from captured original target positions")
    parser.add_argument("--target-executable",
                        help="verify each captured target-position function return")
    args = parser.parse_args()
    data = args.trace.read_bytes()
    rows = [json.loads(line) for line in data.splitlines()]
    done = [row for row in rows if row.get("event") == "done"]
    if len(done) != 1 or done[0]["captured"] != done[0]["requested"]:
        raise ValueError("incomplete live update sequence")
    count = done[0]["captured"]
    if args.stick_path and (not args.varying_input or
                            len(args.stick_path) != count or
                            any(ch not in "RFL" for ch in args.stick_path)):
        raise ValueError("stick path must be R/F/L for each varying-input update")
    if args.vector_path and (not args.varying_input or args.stick_path or
                             len(args.vector_path) != count or
                             any(ch not in "RFL" for ch in args.vector_path)):
        raise ValueError("vector path must be R/F/L for each varying-input update")
    if args.vector_from_trace and (not args.varying_input or args.stick_path or
                                   args.vector_path or args.target_from_trace):
        raise ValueError("vector-from-trace requires varying input only")
    if args.target_from_trace and (not args.varying_input or args.stick_path or
                                   args.vector_path or args.vector_from_trace):
        raise ValueError("target-from-trace requires varying input only")
    if args.target_executable and not args.target_from_trace:
        raise ValueError("target executable requires target-from-trace")
    preparations = [indexed(rows, "preparation", i) for i in range(count)]
    entries = [indexed(rows, "movement_entry", i) for i in range(count)]
    exits = [indexed(rows, "movement_exit", i) for i in range(count)]
    first = preparations[0]
    fields = ("actor_header_hex", "actor_bb4_hex")
    if not args.varying_input:
        fields += ("actor_inputs_hex",)
    if any(any(row[field] != first[field] for field in fields)
           for row in preparations[1:]):
        raise ValueError("actor inputs changed across updates")
    prior_exit = None
    reset_offsets = []
    for i, (prep, entry, exit_event) in enumerate(zip(preparations, entries, exits)):
        if (prep["pc"] != "0x800F705C" or entry["pc"] != "0x80036B28" or
                exit_event["pc"] != "0x80036E58"):
            raise ValueError("unexpected function boundary")
        before = bytes.fromhex(prep["state_hex"])
        expected_entry = bytes.fromhex(entry["state_hex"])
        expected_exit = bytes.fromhex(exit_event["state_hex"])
        if any(len(x) != 0x54 for x in (before, expected_entry, expected_exit)):
            raise ValueError("wrong movement block length")
        if prior_exit is not None:
            different = [j for j in range(0, 0x54, 4)
                         if prior_exit[j:j + 4] != before[j:j + 4]]
            reset_offsets.append(different)
            allowed_changes = ([0x34], [0x24, 0x34]) if args.varying_input else ([0x34],)
            if different not in allowed_changes or (not args.varying_input and
                    before[0x34:0x38] != bytes.fromhex(
                        first["state_hex"])[0x34:0x38]):
                raise ValueError(f"unexpected intercall state change: {different}")
        argv = [args.single_executable, prep["actor_header_hex"],
                "0000" + prep["actor_bb4_hex"], prep["actor_inputs_hex"],
                prep["state_hex"], "--command-only"]
        if args.windows:
            argv = ["cmd.exe", "/c", *argv]
        result = subprocess.run(argv, capture_output=True, text=True, check=True)
        actual_entry = bytes.fromhex(result.stdout.strip().splitlines()[-1])
        if actual_entry != expected_entry:
            raise ValueError(f"update {i} command state differs")
        prior_exit = expected_exit
    reset = None
    reset_count = 0
    if args.reset_executable:
        found = [row for row in rows if row.get("event") == "reset_entry"]
        returned = [row for row in rows if row.get("event") == "reset_return"]
        if len(found) not in (1, count - 1) or len(returned) != len(found) or count < 2:
            raise ValueError("expected the first reset or every intercall reset")
        reset_count = len(found)
        reset = found[0]
        for index, (entry, returned_event) in enumerate(zip(found, returned)):
            if (entry["pc"] != "0x800F7C90" or
                    returned_event["pc"] != "0x800F8710" or
                    entry["after_index"] != index or
                    returned_event["after_index"] != index):
                raise ValueError("unexpected reset-function boundary")
            reset_input = bytes.fromhex(entry["state_hex"])
            expected_reset = bytes.fromhex(returned_event["state_hex"])
            prior_exit = bytes.fromhex(exits[index]["state_hex"])
            cleared = bytearray(prior_exit)
            cleared[0x34:0x38] = bytes(4)
            cleared[0x3c:0x40] = bytes.fromhex("3b6bedfa")
            cleared[0x48:0x4c] = bytes(4)
            if (reset_input != cleared or
                    expected_reset != bytes.fromhex(preparations[index + 1]["state_hex"]) or
                    entry["actor_inputs_hex"] != preparations[index + 1]["actor_inputs_hex"]):
                raise ValueError(f"reset {index} does not connect live calls")
            argv = [args.reset_executable, entry["actor_header_hex"],
                    "0000" + entry["actor_bbc_hex"], entry["actor_408_hex"],
                    entry["actor_inputs_hex"], entry["state_hex"]]
            if args.windows:
                argv = ["cmd.exe", "/c", *argv]
            result = subprocess.run(argv, capture_output=True, text=True, check=True)
            actual_reset = bytes.fromhex(result.stdout.strip().splitlines()[-1])
            if actual_reset != expected_reset:
                raise ValueError(f"native reset {index} differs from live return")
    argv = [args.loop_executable, first["actor_header_hex"],
            "0000" + first["actor_bb4_hex"], first["actor_inputs_hex"],
            first["state_hex"], str(count)]
    if reset is not None:
        argv.extend(("0000" + reset["actor_bbc_hex"], reset["actor_408_hex"]))
    if args.varying_input:
        if reset is None:
            raise ValueError("varying inputs require calculated reset")
        if args.target_from_trace:
            targets = []
            for i in range(1, count):
                originals = [row for row in rows if row.get("event") == "actor_source_entry"
                             and row.get("before_index") == i]
                returns = [row for row in rows if row.get("event") == "actor_source_return"
                           and row.get("before_index") == i]
                if len(originals) != 1 or len(returns) != 1:
                    raise ValueError(f"expected one actor target source call for update {i}")
                original, returned = originals[0], returns[0]
                if (original["pc"] != "0x8008D864" or
                        returned["pc"] != original["return_pc"] or
                        original["position_hex"] != exits[i - 1]["state_hex"][:24]):
                    raise ValueError(f"update {i} target source boundary differs")
                if args.target_executable:
                    command = [args.target_executable, original["vector_hex"],
                               original["position_hex"], original["actor_inputs_hex"]]
                    if args.windows:
                        command = ["cmd.exe", "/c", *command]
                    result = subprocess.run(command, capture_output=True,
                                            text=True, check=True)
                    if result.stdout.strip().splitlines()[-1].lower() != \
                            returned["actor_inputs_hex"]:
                        raise ValueError(f"update {i} target source differs")
                targets.append(original["vector_hex"])
            argv.append("T:" + ",".join(targets))
        elif args.vector_path or args.vector_from_trace:
            vectors = []
            if args.vector_from_trace:
                import struct
                initial_actor = bytes.fromhex(first["actor_inputs_hex"])
                strength_bits = initial_actor[0:4]
                strength = struct.unpack(">f", strength_bits)[0]
                heading = int.from_bytes(initial_actor[4:8], "big")
                if initial_actor[0:4] != initial_actor[0x10:0x14] or not 0 < strength <= 1:
                    raise ValueError("initial actor lacks equal cardinal strength")
                initial_vector = bytearray(0x20)
                if heading == 0:
                    initial_vector[4:8] = strength_bits
                elif heading == 0x3fffff:
                    initial_vector[8:12] = strength_bits
                elif heading == 0x7fffff:
                    initial_vector[4:8] = struct.pack(">f", -strength)
                else:
                    raise ValueError("initial actor heading is not a tested cardinal")
                vectors.append(bytes(initial_vector))
                for i in range(1, count):
                    original = [row for row in rows if row.get("event") == "input_entry"
                                and row.get("before_index") == i]
                    if len(original) != 1:
                        raise ValueError(f"expected original input for update {i}")
                    vectors.append(bytes.fromhex(original[0]["input_hex"]))
            else:
                for ch in args.vector_path:
                    vector = bytearray(0x20)
                    if ch == "R":
                        vector[4:8] = bytes.fromhex("3f800000")
                    elif ch == "L":
                        vector[4:8] = bytes.fromhex("bf800000")
                    else:
                        vector[8:12] = bytes.fromhex("3f800000")
                    vectors.append(bytes(vector))
            for i in range(1, count):
                originals = [row for row in rows if row.get("event") == "input_entry"
                             and row.get("before_index") == i]
                returns = [row for row in rows if row.get("event") == "input_written"
                           and row.get("before_index") == i]
                if len(originals) != len(returns) or (
                        args.translator_executable and not originals):
                    raise ValueError(f"missing original input boundary for update {i}")
                for original, returned in zip(originals, returns):
                    if bytes.fromhex(original["input_hex"]) != vectors[i]:
                        raise ValueError(f"update {i} original input vector differs")
                    if args.translator_executable:
                        command = [args.translator_executable, original["input_hex"],
                                   original["actor_inputs_hex"]]
                        if args.windows:
                            command = ["cmd.exe", "/c", *command]
                        result = subprocess.run(command, capture_output=True,
                                                text=True, check=True)
                        if result.stdout.strip().splitlines()[-1].lower() != \
                                returned["actor_inputs_hex"]:
                            raise ValueError(f"update {i} native translator differs")
            argv.append("V:" + ",".join(vector.hex() for vector in vectors))
        elif args.stick_path:
            base = bytearray.fromhex(first["actor_inputs_hex"])
            for ch, row in zip(args.stick_path, preparations):
                heading = bytes.fromhex({"R": "00000000", "F": "003fffff",
                                         "L": "007fffff"}[ch])
                projected = bytearray(base)
                for offset in (4, 8, 0x14, 0x18):
                    projected[offset:offset + 4] = heading
                if projected != bytes.fromhex(row["actor_inputs_hex"]):
                    raise ValueError("live actor input differs from bounded stick projection")
            argv.append(args.stick_path)
        else:
            argv.append(",".join(row["actor_inputs_hex"] for row in preparations))
    if args.windows:
        argv = ["cmd.exe", "/c", *argv]
    result = subprocess.run(argv, capture_output=True, text=True, check=True)
    if args.vector_path or args.vector_from_trace or args.target_from_trace:
        actor_outputs = [line[6:] for line in result.stdout.splitlines()
                         if line.startswith("actor:")]
        if len(actor_outputs) != count:
            raise ValueError("native vector bridge returned wrong actor count")
        for i, (actual, expected) in enumerate(zip(actor_outputs, preparations)):
            if actual != expected["actor_inputs_hex"]:
                raise ValueError(f"update {i} native actor input block differs")
    outputs = [bytes.fromhex(line) for line in result.stdout.strip().splitlines()
               if len(line) == 0xa8]
    if len(outputs) != count:
        raise ValueError(f"native loop returned {len(outputs)} updates, expected {count}")
    for i, (actual, expected_event) in enumerate(zip(outputs, exits)):
        if actual != bytes.fromhex(expected_event["state_hex"]):
            raise ValueError(f"update {i} native movement differs")
    print(json.dumps({"trace": args.trace.name,
                      "trace_sha256": hashlib.sha256(data).hexdigest(),
                      "matched_updates": count,
                      "varying_input": args.varying_input,
                      "stick_path": args.stick_path,
                      "vector_path": args.vector_path,
                      "vector_from_trace": args.vector_from_trace,
                      "target_from_trace": args.target_from_trace,
                      "calculated_reset": reset is not None,
                      "verified_reset_calls": reset_count,
                      "intercall_changes": [[f"0x{j:02X}" for j in offsets]
                                            for offsets in reset_offsets]}, sort_keys=True))


if __name__ == "__main__":
    main()
