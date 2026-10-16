#!/usr/bin/env python3
"""Transcribe the source-owned down state machine, including its indirect callbacks."""

import hashlib
import json
from pathlib import Path
from typing import Any

import build_ai_policy_native as builder

HERE = Path(__file__).resolve().parent
RUNTIME_MEMORY = Path(
    "/mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/vendor/dolphin/GXRuntime/include/core/cpu.h"
)
CPU_REGISTERS = Path(
    "/mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/"
    "vendor/dolphin/Source/Core/Core/PowerPC/Interpreter/Interpreter_SystemRegisters.cpp"
)
RUNTIME_CACHE = Path(
    "/mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/"
    "vendor/dolphin/GXRuntime/src/core/cpu_interpreter_integer.c"
)
# Actual writers/readers and their parents, not assumed football rules.
ROOTS = (
    "8002e3c8 8002e45c 800f9c50 800f946c 800815b4 800262b0 800263ec "
    "80024fb8 80024f58 80025028 80025840 80025f98 800260b4 8002676c "
    "8010397c 80103c30 80103a28 80103cbc 80103dac 80103cf0 80103ea8 "
    "8004ebf0 80025e60 80025e90 800c19fc 800c1d1c 80180fb4 80182b74 80181bcc 8022297c"
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
        '                code = f"cb_addcarry(s,{args[0][1:]},{a},0xffffffffu,s->ca);"\n'
        '            elif op == "mtspr" and args[0] == "DEC":\n'
        '                code = f"s->dec={transcription.reg(args[1])}; /* CPU state; no clock/interrupt delivery. */"\n'
        + marker,
    )
    # 801EF620 is MTSPR DEC,r3; BLR. 801F0258/801F0510/801F062C
    # use it after updating the original OS alarm list. Keep the value as
    # call-local architectural state, like MSR; the host does not advance
    # time or deliver decrementer interrupts. The original checker supplies
    # this ISA operation through CPUState.instruction_fallback because
    # the unmodified DLL's C emitter delegates MTDEC to that CPU boundary.
    source = source.replace("r[32],lr,ctr,cr,ca,pc,msr;", "r[32],lr,ctr,cr,ca,pc,msr,dec;")
    # The captured allocator vtable also names 802441A0, absent from the
    # Ghidra cache. It is exactly LIS r3,0x100; BLR in all three captures.
    # Bound construction-time decoding at the end of those two instructions,
    # rather than treating adjacent padding/data as part of the function.
    # The container comparator at 80182C00 is likewise absent from the
    # cache: its six instructions end at 80182C18, where another function
    # starts. 801830CC passes this callback to 801805E0, which writes it
    # at container+10; 80180C6C calls that field at 80180D14. Decode exactly
    # the actual comparator, including its unequal return / equal -1 path.
    source = source.replace(
        "            end = next(a for a in known if a > entry)",
        "            end = {0x802441A0: 0x802441A8, 0x80182C00: 0x80182C18}.get(entry) "
        "or next(a for a in known if a > entry)",
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
    # 801F73D4/801F94D8/801F9348 submit a real DVD request after the
    # touchdown whistle. Their CC006000..24 word accesses are external
    # memory, not invalid MEM1 pointers. The oracle has no external_read /
    # external_write callbacks: GXRuntime cpu.h mem_read32 returns zero
    # and mem_write32 discards the write. Preserve every source instruction
    # and all request/queue/alarm RAM work with that same explicit boundary.
    # This does not simulate a disc, DMA completion, interrupts, or any
    # other device; other out-of-span addresses must still fail closed.
    source = source.replace(
        "    out = [",
        '    helpers = helpers.replace("static uint32_t cb_load(cb_machine *s,uint32_t a,unsigned n){",\n'
        '        "static uint32_t cb_load(cb_machine *s,uint32_t a,unsigned n){"\n'
        '        "if(n==4&&a>=0xcc006000u&&a<=0xcc006024u&&!(a&3u))return 0;")\n'
        '    helpers = helpers.replace("static void cb_store(cb_machine *s,uint32_t a,uint32_t v,unsigned n){",\n'
        '        "static void cb_store(cb_machine *s,uint32_t a,uint32_t v,unsigned n){"\n'
        '        "if(n==4&&a>=0xcc006000u&&a<=0xcc006024u&&!(a&3u))return;")\n'
        '    helpers = ("/* DVD register words: zero reads / discarded writes, matching the oracle\\n"\n'
        '        "   with unset external device callbacks. No DMA or completion is simulated. */\\n" + helpers)\n'
        "    out = [",
    )
    for name in ("init", "host_call", "call", "service"):
        source = source.replace(f"nflstreet_ai_policy_{name}", f"nflstreet_down_native_{name}")
    scope: dict[str, Any] = {"__file__": str(__file__), "__name__": "down_builder"}
    exec(compile(source, str(__file__), "exec"), scope)
    callbacks = {}
    camera_callbacks = {}
    allocation_callbacks = {}
    allocator_vtables = {}
    callbacks_803569b8 = {}
    container_comparators = {}
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

        # 801830CC publishes this container through r13-4F3C after
        # registering LAB_80182C00 with 801805E0. Retain the captured
        # callback without rewriting the container, its records or cases.
        container = word(0x803A9A40 - 0x4F3C)
        comparator = word(container + 0x10)
        if comparator != 0x80182C00:
            raise RuntimeError(f"Changed container comparator {comparator:08x} in {context}")
        instructions = tuple(word(comparator + offset) for offset in range(0, 24, 4))
        if instructions != (0x81230020, 0x80040020, 0x7C604851, 0x4C820020, 0x3860FFFF, 0x4E800020):
            raise RuntimeError(f"Changed uncached comparator instructions in {context}")
        container_comparators[str(context)] = {
            "global": f"{0x803A9A40 - 0x4F3C:08x}",
            "container": f"{container:08x}",
            "callback_offset": "10",
            "callback": f"{comparator:08x}",
            "initializer": "801830cc -> 801805e0",
            "reader": "80180c6c, indirect call at 80180d14",
            "confidence": "candidate until original-module gate passes",
        }
        targets.append(f"{comparator:08x}")
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
        # 8022297C registers seven callbacks at 803569B8..D0.
        # 80227580 loads 803569CC before BLRL at 802276AC; the
        # captured target is 80224640. Preserve all seven registrations
        # and their initializer's source closure, not just the failed call.
        # 80224058 publishes the 0x60-byte row array through 803569AC;
        # its count is 803567D8's high byte. 80224640 walks row+40's list,
        # invoking node+4 and advancing through node+8 before freeing.
        # Capture those real callback bindings as well. This adds native
        # functions without changing the saved registry, lists, or cases.
        registrations_803569b8 = {}
        for address in range(0x803569B8, 0x803569D4, 4):
            target = word(address)
            if target and (target & 3 or not (builder.CACHE / "disassembly" / f"{target:08x}.txt").is_file()):
                raise RuntimeError(f"Invalid registered callback {address:08x} -> {target:08x} in {context}")
            registrations_803569b8[f"{address:08x}"] = f"{target:08x}"
            if target:
                targets.append(f"{target:08x}")
        # Related source consumers: 80224600 calls 803567DC;
        # 8022478C calls 803567E4/803569B0; 802316D8 calls 803569B4;
        # 80232220/24C and 802324D8/504 call 80362F04/08. Keep NULLs
        # and bounded captured-code targets (803569B0 names 802307F0,
        # which has no cache file) just like the existing closure decoder.
        related = {}
        for address in (0x803567DC, 0x803567E4, 0x803569B0, 0x803569B4, 0x80362F04, 0x80362F08):
            target = word(address)
            if target and (target & 3 or not 0x80004000 <= target < 0x80244000):
                raise RuntimeError(f"Invalid related callback {address:08x} -> {target:08x} in {context}")
            related[f"{address:08x}"] = f"{target:08x}"
            if target:
                targets.append(f"{target:08x}")
        count = word(0x803567D8) >> 24
        rows_address = word(0x803569AC)
        lists = {}
        for row in range(count):
            address = rows_address + 0x60 * row
            node = word(address + 0x40)
            visited: set[int] = set()
            nodes = []
            while node:
                if node in visited:
                    raise RuntimeError(f"Cyclic callback list {address:08x} in {context}")
                visited.add(node)
                target, successor = word(node + 4), word(node + 8)
                if target and (target & 3 or not (builder.CACHE / "disassembly" / f"{target:08x}.txt").is_file()):
                    raise RuntimeError(f"Invalid list callback {node:08x} -> {target:08x} in {context}")
                nodes.append({"node": f"{node:08x}", "callback": f"{target:08x}", "next": f"{successor:08x}"})
                if target:
                    targets.append(f"{target:08x}")
                node = successor
            lists[str(row)] = {"address": f"{address:08x}", "nodes": nodes}
        callbacks_803569b8[str(context)] = {
            "registrations": registrations_803569b8,
            "related_callbacks": related,
            "rows_address": f"{rows_address:08x}",
            "rows_count": count,
            "lists": lists,
        }
    scope["ROOTS"] = list(dict.fromkeys(scope["ROOTS"] + ROOTS + sorted(set(targets))))
    scope["main"]()
    manifest = HERE / "down_cycle_sources.json"
    provenance = json.loads(manifest.read_text())
    provenance["phase_and_sort_callbacks"] = callbacks
    provenance["camera_event_callbacks"] = camera_callbacks
    provenance["allocation_callbacks"] = allocation_callbacks
    provenance["allocator_vtables"] = allocator_vtables
    provenance["callback_registry_803569b8"] = callbacks_803569b8
    provenance["container_comparators"] = container_comparators
    provenance["disc_interface_boundary"] = {
        "registers": "cc006000..cc006024, aligned u32 only",
        "read": "zero, matching mem_read32 with external_read unset",
        "write": "discard, matching mem_write32 with external_write unset",
        "source": str(RUNTIME_MEMORY),
        "source_sha256": hashlib.sha256(RUNTIME_MEMORY.read_bytes()).hexdigest(),
        "scope": "synchronous host boundary only; no device, DMA completion or interrupt simulation",
        "confidence": "candidate until original-module gate passes",
    }
    provenance["decrementer_boundary"] = {
        "instruction": "801ef620: mtspr DEC,r3; SPR_DEC = 22",
        "native": "store operand in call-local architectural DEC; retain all original alarm RAM work",
        "oracle": "independent DEC write in CPU fallback; advance PC by 4, execute original BLR",
        "scope": "no clock advancement or decrementer interrupt delivery; only documented CPU fallbacks allowed",
        "source": str(CPU_REGISTERS),
        "source_sha256": hashlib.sha256(CPU_REGISTERS.read_bytes()).hexdigest(),
        "confidence": "candidate until original-module gate passes",
    }
    provenance["cache_maintenance_boundary"] = {
        "operations": ["dcbst", "dcbf", "dcbi", "icbi"],
        "observed_instruction": "801f0b90: dcbi 0,r3 (7c001bac)",
        "oracle": "no RAM effects when cache_control is unset; advance PC by 4 and retain original SDK loops",
        "native": "existing static cache-ordering operations leave the flat RAM span unchanged",
        "scope": "no cache simulation; DCBI requires supervisor mode; DCBZ and other fallbacks still reject",
        "source": str(RUNTIME_CACHE),
        "source_sha256": hashlib.sha256(RUNTIME_CACHE.read_bytes()).hexdigest(),
        "confidence": "candidate until original-module gate passes",
    }
    manifest.write_text(json.dumps(provenance, indent=2, sort_keys=True) + "\n")


if __name__ == "__main__":
    main()
