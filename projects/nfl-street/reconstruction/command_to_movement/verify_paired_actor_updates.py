"""Replay one live Stewart/Thomas pair sequence with native Windows actor code."""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
import subprocess
from pathlib import Path


def one(rows: list[dict], event: str, pair: int, slot: int | None = None) -> dict:
    found = [row for row in rows if row.get("event") == event and
             row.get("pair") == pair and (slot is None or row.get("slot") == slot)]
    if len(found) != 1:
        raise ValueError(f"expected one {event} for pair {pair}, slot {slot}")
    return found[0]


def native_outputs(executable: str, preparations: list[dict], path: str,
                   windows: bool) -> tuple[list[str], list[str]]:
    first = preparations[0]
    command = [executable, first["header_hex"],
               "0000" + first["rating_bb4_hex"], first["actor_hex"],
               first["state_hex"], str(len(preparations)),
               "0000" + first["rating_bbc_hex"], first["actor_408_hex"], path]
    if windows:
        windows_executable = subprocess.run(
            ["wslpath", "-w", str(Path(executable).resolve())],
            text=True, capture_output=True, check=True).stdout.strip()
        command = ["cmd.exe", "/c", windows_executable, *command[1:]]
    result = subprocess.run(command, text=True, capture_output=True, check=True,
                            cwd=Path(executable).resolve().parent if windows else None)
    lines = result.stdout.splitlines()
    actors = [line[6:] for line in lines if line.startswith("actor:")]
    states = [line for line in lines if len(line) == 0xA8]
    if len(actors) != len(preparations) or len(states) != len(preparations):
        raise ValueError("native executable returned the wrong update count")
    return actors, states


def target_from_position(position_hex: str) -> str:
    position = bytes.fromhex(position_hex)
    if len(position) != 12 or position[8:] != bytes(4):
        raise ValueError("unobserved selected-actor Z position")
    y = struct.unpack(">f", position[4:8])[0]
    return (position[:4] + struct.pack(">f", y - 2.0) + position[8:]).hex()


def verify_paired_native(executable: str, stewart: list[dict], thomas: list[dict],
                         vectors: list[str], targets: list[str], windows: bool,
                         rows: list[dict], handle: int, per_team: int) -> None:
    def fields(prep: dict) -> list[str]:
        return [prep["header_hex"], "0000" + prep["rating_bb4_hex"],
                prep["actor_hex"], prep["state_hex"],
                "0000" + prep["rating_bbc_hex"], prep["actor_408_hex"]]

    command = [executable, *fields(stewart[0]), *fields(thomas[0]),
               str(len(stewart)), ",".join(vectors),
               f"{handle:08x}", f"{per_team:08x}", "00000008"]
    if windows:
        windows_executable = subprocess.run(
            ["wslpath", "-w", str(Path(executable).resolve())],
            text=True, capture_output=True, check=True).stdout.strip()
        command = ["cmd.exe", "/c", windows_executable, *command[1:]]
    result = subprocess.run(command, text=True, capture_output=True, check=True,
                            cwd=Path(executable).resolve().parent if windows else None)
    lines = result.stdout.splitlines()
    expected: list[str] = []
    for pair in range(len(stewart)):
        if pair:
            expected.append("target:" + targets[pair - 1])
        expected.extend((
            "stewart_actor:" + stewart[pair]["actor_hex"],
            "stewart_state:" + one(rows, "movement_exit", pair, 7)["state_hex"],
            "thomas_actor:" + thomas[pair]["actor_hex"],
            "thomas_state:" + one(rows, "movement_exit", pair, 8)["state_hex"],
        ))
    if lines != expected:
        for index, (actual, wanted) in enumerate(zip(lines, expected)):
            if actual != wanted:
                raise ValueError(f"paired native output line {index} differs")
        raise ValueError("paired native output count differs")


def verify_snapshot_native(executable: str, memory_path: Path,
                           rows: list[dict], count: int,
                           vector: bytes, windows: bool) -> None:
    snapshot_rows = [row for row in rows if row.get("event") == "mem1_snapshot"]
    memory = memory_path.read_bytes()
    if (len(snapshot_rows) != 1 or snapshot_rows[0].get("pair") != 0 or
            snapshot_rows[0].get("slot") != 7 or
            snapshot_rows[0].get("size") != len(memory) or
            snapshot_rows[0].get("sha256") != hashlib.sha256(memory).hexdigest() or
            len(memory) != 0x1800000 or
            snapshot_rows[0].get("frame") !=
            one(rows, "preparation", 0, 7)["frame"]):
        raise ValueError("MEM1 snapshot differs from paired capture")
    stewart = one(rows, "preparation", 0, 7)
    address = int(rows[0]["actors"]["7"], 16) - 0x80000000
    if (memory[address + 0x200:address + 0x230].hex() != stewart["actor_hex"] or
            memory[address + 0x1A8:address + 0x1FC].hex() != stewart["state_hex"]):
        raise ValueError("Stewart initial state not in MEM1 snapshot")
    command = [executable, "--mem1", str(memory_path), str(count),
               ",".join([vector.hex()] * count)]
    if windows:
        executable_windows = subprocess.run(
            ["wslpath", "-w", str(Path(executable).resolve())],
            text=True, capture_output=True, check=True).stdout.strip()
        memory_windows = subprocess.run(
            ["wslpath", "-w", str(memory_path.resolve())],
            text=True, capture_output=True, check=True).stdout.strip()
        command = ["cmd.exe", "/c", executable_windows, "--mem1",
                   memory_windows, str(count), ",".join([vector.hex()] * count)]
    result = subprocess.run(command, text=True, capture_output=True, check=True,
                            cwd=Path(executable).resolve().parent if windows else None)
    expected: list[str] = []
    for pair in range(count):
        if pair:
            expected.append("target:" + one(rows, "target_source", pair)["target_hex"])
        for slot, name in ((7, "stewart"), (8, "thomas")):
            expected.append(name + "_actor:" + one(rows, "preparation", pair, slot)["actor_hex"])
            expected.append(name + "_state:" + one(rows, "movement_exit", pair, slot)["state_hex"])
    if result.stdout.splitlines() != expected:
        raise ValueError("snapshot-start native loop differs from live pair")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--trace", required=True, type=Path)
    parser.add_argument("--loop-executable", required=True)
    parser.add_argument("--paired-executable", help="integrated two-actor loop")
    parser.add_argument("--choice-trace", type=Path,
                        help="live handle capture for integrated pair selection")
    parser.add_argument("--mem1", type=Path,
                        help="private MEM1 snapshot captured at first Stewart preparation")
    parser.add_argument("--snapshot-executable",
                        help="paired Windows loop with native MEM1 startup")
    parser.add_argument("--windows", action="store_true")
    args = parser.parse_args()
    raw = args.trace.read_bytes()
    rows = [json.loads(line) for line in raw.splitlines()]
    done = [row for row in rows if row.get("event") == "done"]
    if len(done) != 1 or done[0]["captured"] != done[0]["requested"]:
        raise ValueError("incomplete paired trace")
    count = done[0]["captured"]
    if not 2 <= count <= 8:
        raise ValueError("unexpected pair count")
    names = [row["event"] for row in rows if row["event"] not in
             ("ready", "other_stop", "mem1_snapshot", "done")]
    expected = []
    for pair in range(count):
        if pair:
            expected.append("target_source")
        expected.extend(("preparation", "movement_entry", "movement_exit") * 2)
    if names != expected:
        raise ValueError("unexpected paired call order")
    stewart = [one(rows, "preparation", pair, 7) for pair in range(count)]
    thomas = [one(rows, "preparation", pair, 8) for pair in range(count)]
    for pair in range(count):
        for slot, prep in ((7, stewart[pair]), (8, thomas[pair])):
            entry = one(rows, "movement_entry", pair, slot)
            end = one(rows, "movement_exit", pair, slot)
            if (prep["pc"] != "0x800F705C" or entry["pc"] != "0x80036B28" or
                    end["pc"] != "0x80036E58" or
                    prep["frame"] != entry["frame"] or
                    prep["frame"] != end["frame"]):
                raise ValueError(f"pair {pair}, slot {slot} boundary differs")
        if stewart[pair]["frame"] != thomas[pair]["frame"]:
            raise ValueError(f"pair {pair} crossed a dumped frame")
        if pair and stewart[pair]["frame"] != stewart[pair - 1]["frame"] + 1:
            raise ValueError("paired updates were not on consecutive dumped frames")
    vector = bytearray(0x20)
    stick = rows[0].get("stick", [1.0, 0.5])
    if stick == [1.0, 0.5]:
        vector[4:8] = bytes.fromhex("3f800000")
    elif stick == [0.5, 0.0]:
        vector[8:12] = bytes.fromhex("3f800000")
    else:
        raise ValueError("unverified paired stick input")
    thomas_path = "V:" + ",".join(vector.hex() for _ in range(count))
    thomas_actors, thomas_states = native_outputs(
        args.loop_executable, thomas, thomas_path, args.windows)
    for pair in range(count):
        if (thomas_actors[pair] != thomas[pair]["actor_hex"] or
                thomas_states[pair] != one(rows, "movement_exit", pair, 8)["state_hex"]):
            raise ValueError(f"native Thomas update {pair} differs")
    other_positions = []
    targets = []
    for pair in range(1, count):
        source = one(rows, "target_source", pair)
        prior_position = thomas_states[pair - 1][:24]
        if (source["pc"] != "0x8008D864" or
                source["selected_actor"] != rows[0]["actors"]["8"] or
                source["selected_position_hex"] != prior_position or
                source["offset_hex"] != "00000000c0000000" or
                source["frame"] != stewart[pair]["frame"]):
            raise ValueError(f"pair {pair} selected-actor dependency differs")
        target = target_from_position(prior_position)
        if target != source["target_hex"]:
            raise ValueError(f"pair {pair} native target differs")
        other_positions.append(prior_position)
        targets.append(target)
    stewart_path = "P:" + ",".join(other_positions)
    stewart_actors, stewart_states = native_outputs(
        args.loop_executable, stewart, stewart_path, args.windows)
    for pair in range(count):
        if (stewart_actors[pair] != stewart[pair]["actor_hex"] or
                stewart_states[pair] != one(rows, "movement_exit", pair, 7)["state_hex"]):
            raise ValueError(f"native Stewart update {pair} differs")
    if args.paired_executable:
        if not args.choice_trace:
            raise ValueError("integrated pair replay needs a live choice trace")
        choice_rows = [json.loads(line) for line in
                       args.choice_trace.read_bytes().splitlines()]
        choice_ready = choice_rows[0]
        choice_done = choice_rows[-1]
        selections = [row for row in choice_rows
                      if row.get("event") == "selector_entry"]
        if (choice_ready.get("savestate_sha256") != rows[0]["savestate_sha256"] or
                choice_done.get("event") != "done" or
                choice_done.get("captured") != choice_done.get("requested") or
                len(selections) != choice_done["captured"] or
                choice_ready.get("thomas") != rows[0]["actors"]["8"] or
                any(row.get("other_pointer") != choice_ready["thomas"] or
                    row.get("other_slot") != 8 for row in selections)):
            raise ValueError("choice trace differs from paired saved match")
        handles = {int(row["selection_handle"], 16) for row in selections}
        if len(handles) != 1 or choice_ready["roster_count"] != 7:
            raise ValueError("unobserved handle or roster layout")
        verify_paired_native(args.paired_executable, stewart, thomas,
                             [vector.hex()] * count, targets,
                             args.windows, rows, handles.pop(),
                             choice_ready["roster_count"])
    if args.snapshot_executable:
        if not args.mem1:
            raise ValueError("snapshot executable requires --mem1")
        verify_snapshot_native(args.snapshot_executable, args.mem1,
                               rows, count, vector, args.windows)
    print(json.dumps({"trace": args.trace.name,
                      "trace_sha256": hashlib.sha256(raw).hexdigest(),
                      "matched_pairs": count,
                      "matched_actor_updates": count * 2,
                      "native_inter_actor_targets": len(other_positions),
                      "integrated_native_loop": bool(args.paired_executable),
                      "snapshot_native_loop": bool(args.snapshot_executable),
                      "frames": [stewart[0]["frame"], stewart[-1]["frame"]]},
                     sort_keys=True))


if __name__ == "__main__":
    main()
