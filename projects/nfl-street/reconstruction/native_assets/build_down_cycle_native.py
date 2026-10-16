#!/usr/bin/env python3
"""Transcribe the source-owned down state machine, including its indirect callbacks."""

import json
from pathlib import Path
from typing import Any

import build_ai_policy_native as builder

HERE = Path(__file__).resolve().parent
# Actual writers/readers and their parents, not assumed football rules.
ROOTS = (
    "8002e3c8 8002e45c 800f9c50 800f946c 800815b4 800262b0 800263ec "
    "80024fb8 80024f58 80025028 80025840 80025f98 800260b4 8002676c "
    "8010397c 80103c30 80103a28 80103cbc 80103dac 80103cf0 80103ea8 "
    "8004ebf0 80025e60 80025e90 800c19fc 800c1d1c 80180fb4 80182b74 80181bcc"
).split()


def main() -> None:
    source = Path(builder.__file__).read_text()
    source = source.replace('"ai_policy.c"', '"down_cycle_native.inc"')
    source = source.replace('"ai_policy_sources.json"', '"down_cycle_sources.json"')
    direct = '        todo += [v["addr"] for v in graph.get(addr, {}).get("callees", [])]'
    source = source.replace(
        direct,
        direct
        + '\n        references = CACHE / "decompiled" / f"{addr}.txt"'
        + "\n        if references.exists():"
        + '\n            todo += [v for v in re.findall(r"(?:FUN_|&LAB_)([0-9a-f]{8})", references.read_text())'
        + "\n                     if 0x80004000 <= int(v, 16) < 0x80244000 and not int(v, 16) & 3]",
    )
    # CPUState at this host boundary starts with timebase zero; the oracle
    # does not deliver interrupts or advance emulated hardware time either.
    marker = '            elif op == "mfmsr":'
    source = source.replace(
        marker,
        '            elif op == "mftb":\n'
        '                code = f"{transcription.reg(args[0])}=0;"\n'
        '            elif op == "orc":\n'
        "                d, a, b = [transcription.reg(v) for v in args]\n"
        '                code = f"{d}={a}|~{b};"\n'
        '            elif op == "addme":\n'
        "                d, a = [transcription.reg(v) for v in args]\n"
        '                code = f"cb_addcarry(s,{args[0][1:]},{a},0xffffffffu,s->ca);"\n' + marker,
    )
    # The captured allocator vtable also names 802441A0, absent from the
    # Ghidra cache. It is exactly LIS r3,0x100; BLR in all three captures.
    # Bound construction-time decoding at the end of those two instructions,
    # rather than treating adjacent padding/data as part of the function.
    source = source.replace(
        "            end = next(a for a in known if a > entry)",
        "            end = 0x802441A8 if entry == 0x802441A0 else next(a for a in known if a > entry)",
    )
    # Ghidra splits the shared GPR save/restore entry points into fragments:
    # e.g. 80215458 contains one STW and falls into 8021545C; 802154A8
    # contains one LWZ and falls into 802154AC. A cache-file boundary is
    # not a BLR. Follow the actual next instruction and retain that edge
    # in the emitted C without synthesizing an original PPC instruction.
    source = source.replace("    captured = {}", "    captured = {}\n    fallthrough = {}")
    source = source.replace(
        "        local_pcs = {pc for pc, _, _ in ops}",
        '        if ops and ops[-1][1] not in ("blr", "b", "bctr", "rfi"):\n'
        "            successor = ops[-1][0] + 4\n"
        "            fallthrough[entry] = successor\n"
        '            todo.append(f"{successor:08x}")\n'
        "        local_pcs = {pc for pc, _, _ in ops}",
    )
    source = source.replace(
        '        out.append("}")',
        "        if function_addr in fallthrough:\n"
        '            out.append(f"/* Source fallthrough after cached fragment {function_addr:08x}. */ "\n'
        '                       f"cb_dispatch(s,0x{fallthrough[function_addr]:08x}u);return;")\n'
        '        out.append("}")',
    )
    source = source.replace(
        '                "captured_functions": captured,',
        '                "captured_functions": captured,\n'
        '                "fallthrough": {f"{a:08x}": f"{b:08x}" for a, b in fallthrough.items()},',
    )
    for name in ("init", "host_call", "call", "service"):
        source = source.replace(f"nflstreet_ai_policy_{name}", f"nflstreet_down_native_{name}")
    scope: dict[str, Any] = {"__file__": str(__file__), "__name__": "down_builder"}
    exec(compile(source, str(__file__), "exec"), scope)
    callbacks = {}
    camera_callbacks = {}
    allocation_callbacks = {}
    allocator_vtables = {}
    targets = []
    for context in (14, 15, 16):
        capture = Path(f"/mnt/w/Daywater/research-temp/position-neutral-{context}.mem1").read_bytes()
        bindings = {}
        # 8002E45C enters/exits and 8002E3C8 updates the 11-row phase
        # table. Also retain the source-registered 801AC670 sort callbacks.
        addresses = [0x80245A80 + 4 * i for i in range(33)] + [0x8039E2B4, 0x8039E2BC]
        for address in addresses:
            offset = address - 0x80000000
            target = int.from_bytes(capture[offset : offset + 4], "big")
            if target and (not 0x80004000 <= target < 0x80244000 or target & 3):
                raise RuntimeError(f"Invalid phase callback {address:08x} -> {target:08x}")
            bindings[f"{address:08x}"] = f"{target:08x}"
            if target:
                targets.append(f"{target:08x}")
        callbacks[str(context)] = bindings

        # 800C7160 resolves manager slots at *(r13-5BA4)+8+4*i;
        # 800C6FB8 visits slots 0..3. 800C33D8 BLRLs each camera's
        # +DC event callback. These actual registrations are not direct
        # callgraph edges, and are not a contiguous PTR_FUN source table.
        def word(address: int) -> int:
            offset = address - 0x80000000
            if offset < 0 or offset + 4 > len(capture):
                raise RuntimeError(f"Invalid camera registration span {address:08x} in {context}")
            return int.from_bytes(capture[offset : offset + 4], "big")

        manager = word(0x803A9A40 - 0x5BA4)
        cameras = {}
        for slot in range(4):
            camera = word(manager + 8 + 4 * slot)
            target = word(camera + 0xDC) if camera else 0
            if target and (not 0x80004000 <= target < 0x80244000 or target & 3):
                raise RuntimeError(f"Invalid camera event callback {camera:08x} -> {target:08x}")
            cameras[str(slot)] = {"camera": f"{camera:08x}", "callback": f"{target:08x}"}
            if target:
                targets.append(f"{target:08x}")
        camera_callbacks[str(context)] = {"manager": f"{manager:08x}", "slots": cameras}
        # 80180490 registers nine callback words in each 0x30-byte row
        # at 80389E88. Its source callers 80180FB4/80182B74/80181BCC
        # register rows 0/1/2. 80180700's BLRL at 8018074C reads row+8
        # (80180D64 in every capture), absent from the direct call graph.
        # Retain every registered callback, not just that first reached slot.
        registrations = {}
        for row in range(3):
            address = 0x80389E88 + 0x30 * row
            entries = [word(address + 4 * slot) for slot in range(9)]
            if any(target and (not 0x80004000 <= target < 0x80244000 or target & 3) for target in entries):
                raise RuntimeError(f"Invalid allocation callback row {row} in {context}")
            registrations[str(row)] = {
                "address": f"{address:08x}",
                "callbacks": [f"{target:08x}" for target in entries],
            }
            targets.extend(f"{target:08x}" for target in entries if target)
        allocation_callbacks[str(context)] = registrations
        # 80172E8C returns *(r13-57A0). 8013B4F8 reads its vtable's
        # signed this adjustment at +8 and allocator target at +C before
        # BLRL at 8013B550. 8013B5AC/80173464 use the +10/+14 pair.
        # Preserve the three populated method pairs of the captured table,
        # including the tail thunk, without the old approximate 80244000
        # code cutoff (both cached allocator methods lie above it).
        allocator = word(0x803A9A40 - 0x57A0)
        vtable = word(allocator)
        methods = {}
        for offset in (0x0C, 0x14, 0x1C):
            target = word(vtable + offset)
            if target and (
                target & 3
                or not ((builder.CACHE / "disassembly" / f"{target:08x}.txt").is_file() or target == 0x802441A0)
            ):
                raise RuntimeError(f"Unbounded allocator method {target:08x} in {context}")
            if target == 0x802441A0 and (word(target), word(target + 4)) != (0x3C600100, 0x4E800020):
                raise RuntimeError(f"Changed allocator tail thunk in {context}")
            adjustment = word(vtable + offset - 4) >> 16
            methods[f"{offset:02x}"] = {
                "target": f"{target:08x}",
                "this_adjustment": adjustment if adjustment < 0x8000 else adjustment - 0x10000,
            }
            if target:
                targets.append(f"{target:08x}")
        allocator_vtables[str(context)] = {
            "object": f"{allocator:08x}",
            "vtable": f"{vtable:08x}",
            "methods": methods,
        }
    scope["ROOTS"] = list(dict.fromkeys(scope["ROOTS"] + ROOTS + sorted(set(targets))))
    scope["main"]()
    manifest = HERE / "down_cycle_sources.json"
    provenance = json.loads(manifest.read_text())
    provenance["phase_and_sort_callbacks"] = callbacks
    provenance["camera_event_callbacks"] = camera_callbacks
    provenance["allocation_callbacks"] = allocation_callbacks
    provenance["allocator_vtables"] = allocator_vtables
    manifest.write_text(json.dumps(provenance, indent=2, sort_keys=True) + "\n")


if __name__ == "__main__":
    main()
