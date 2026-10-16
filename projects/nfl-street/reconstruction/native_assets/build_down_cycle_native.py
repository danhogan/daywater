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
    "8004ebf0 80025e60 80025e90 800c19fc 800c1d1c"
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
    for name in ("init", "host_call", "call", "service"):
        source = source.replace(f"nflstreet_ai_policy_{name}", f"nflstreet_down_native_{name}")
    scope: dict[str, Any] = {"__file__": str(__file__), "__name__": "down_builder"}
    exec(compile(source, str(__file__), "exec"), scope)
    callbacks = {}
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
    scope["ROOTS"] = list(dict.fromkeys(scope["ROOTS"] + ROOTS + sorted(set(targets))))
    scope["main"]()
    manifest = HERE / "down_cycle_sources.json"
    provenance = json.loads(manifest.read_text())
    provenance["phase_and_sort_callbacks"] = callbacks
    manifest.write_text(json.dumps(provenance, indent=2, sort_keys=True) + "\n")


if __name__ == "__main__":
    main()
