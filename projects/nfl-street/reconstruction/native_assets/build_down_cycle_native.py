#!/usr/bin/env python3
"""Transcribe the source-owned down state machine, including its indirect callbacks."""

import hashlib
import json
from pathlib import Path
from typing import Any

import build_ai_policy_native as builder

HERE = Path(__file__).resolve().parent
CPU_REGISTERS = Path(
    "/mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/"
    "vendor/dolphin/Source/Core/Core/PowerPC/Interpreter/Interpreter_SystemRegisters.cpp"
)
RUNTIME_CACHE = Path(
    "/mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/"
    "vendor/dolphin/GXRuntime/src/core/cpu_interpreter_integer.c"
)
# Audio, UI and animation resource-stream entry points, rather than DI/DSP/ARAM
# registers or shared event services. None of their source closures calls a
# football writer. 801405B8 reads replay/mirror flags but writes only its
# sound-history records. Preserve general event delivery and gameplay
# getters (including their cache writes) above these service requests.
# Numeric returns are disabled-audio / unavailable-resource / suppressed-request results;
# None denotes a void early return, preserving the incoming GPRs.
AUDIO_DISC_SERVICES = {
    0x8012AC68: (0, "UI resource-bank load request; unavailable-resource NULL result"),
    0x8013D4D4: (None, "sound-bank load request and audio bank-slot publication"),
    0x8013D8A4: (0x7FFFFFFF, "sound-cue start request; original audio-disabled handle"),
    0x8013D9C0: (None, "sound-cue stop request"),
    0x8013DAD4: (None, "streamed sound-bank load and audio stream-slot publication"),
    0x8013FFF8: (None, "ambient sound-bank transition and synchronous streaming wait"),
    0x801405B8: (None, "replay sound-cue request and audio history"),
    0x8014164C: (None, "music/stream channel control request"),
    0x8014368C: (None, "announcer/speech bank prefetch request"),
    0x80175448: (0, "animation-bank stream request; suppressed request, no bank publication"),
    0x801AB968: (0xFFFFFFFF, "streamed voice-start request; original audio-disabled handle"),
    0x801ABA64: (None, "streamed sound channel control request"),
}

# Actual writers/readers and their parents, not assumed football rules.
ROOTS = (
    "8002e3c8 8002e45c 800f9c50 800f946c 800815b4 800262b0 800263ec "
    "80024fb8 80024f58 80025028 80025840 80025f98 800260b4 8002676c "
    "8010397c 80103c30 80103a28 80103cbc 80103dac 80103cf0 80103ea8 "
    "8004ebf0 80025e60 80025e90 800c19fc 800c1d1c 80180fb4 80182b74 80181bcc 8022297c"
).split()


def audio_disc_write_audit() -> dict[str, Any]:
    """Pin service closures, UI resource-cache writers and read-only game leaves."""
    graph = json.loads((builder.CACHE / "callgraph.json").read_text())
    readonly = {"8000c360", "80103e64"}
    # These seven exact functions operate on *(r13-6CE8)'s UI resource
    # manager/cache records (8012BBD8 initialization; 8012AA2C registration).
    # They are not general permission to skip game-code address ranges.
    ui_cache = {"80129d04", "80129d94", "80129e3c", "8012a168", "8012a360", "8012a3b0", "8012ac68"}
    # 80175448 supplies 80174B20 to the resource reader. 80173B10,
    # 80016278 and the three captures pin its nested asset callbacks.
    # Audit these as well as direct calls; none may reach a football writer.
    animation_callbacks = {"80174b20", "80175fac", "80151928", "80151968"}

    def closure(roots: set[str]) -> set[str]:
        seen: set[str] = set()
        todo = list(roots)
        while todo:
            current = todo.pop()
            if current not in seen:
                seen.add(current)
                todo += [edge["addr"] for edge in graph.get(current, {}).get("callees", [])]
        return seen

    audit = {}
    for address in AUDIO_DISC_SERVICES:
        seen = closure({f"{address:08x}"})
        callbacks = {0x80175448: animation_callbacks, 0x801AB968: {"801add94"}}.get(address, set())
        callback_closure = closure(callbacks)
        low_entries = {entry for entry in seen | callback_closure if int(entry, 16) < 0x80130000}
        permitted = readonly | (ui_cache if address == 0x8012AC68 else set())
        if low_entries - permitted:
            raise RuntimeError(f"Unreviewed game entry in audio/disc closure {address:08x}: {low_entries}")
        game_leaves = low_entries & readonly
        # Neither leaf contains stores, calls or indirect branches. Keep
        # phase/config getters that publish caches ABOVE the boundary.
        for entry in game_leaves:
            instructions = (builder.CACHE / "disassembly" / f"{entry}.txt").read_text().splitlines()
            for instruction in instructions:
                op = instruction.split(": ", 1)[1].split()[0]
                if op.startswith("st") or op in ("bl", "blrl", "bctrl", "bctr"):
                    raise RuntimeError(f"Read-only game leaf changed: {entry}: {instruction}")
        audit[f"{address:08x}"] = {
            "direct_closure_functions": len(seen),
            "direct_closure_sha256": hashlib.sha256("\n".join(sorted(seen)).encode()).hexdigest(),
            "read_only_game_leaves": sorted(game_leaves),
            "ui_resource_cache_functions": sorted(low_entries & ui_cache),
            "write_scope": "audio banks/voices/history, UI resource cache/requests, heap/OS/device state only",
            "source_review": "down_cycle.md audio/disc write ledger; shared game event dispatch is retained",
        }
        if callbacks:
            audit[f"{address:08x}"].update(
                reviewed_callback_roots=sorted(callbacks),
                callback_closure_functions=len(callback_closure),
                callback_closure_sha256=hashlib.sha256("\n".join(sorted(callback_closure)).encode()).hexdigest(),
                callback_disassembly_sha256={
                    entry: hashlib.sha256((builder.CACHE / "disassembly" / f"{entry}.txt").read_bytes()).hexdigest()
                    for entry in sorted(callbacks)
                },
                write_scope=(
                    "animation-bank/cache/request records, asset-local relocations, heap/OS/device state only"
                    if address == 0x80175448
                    else "audio voice queues/handles, STRM ring buffers/file requests, heap/OS/device state only"
                ),
            )
        if address == 0x801AB968:
            audit[f"{address:08x}"].update(
                disabled_return="ffffffff from 801ab968's original disabled-audio branch",
                retained_caller="80143acc; includes 801439b0 -> 801436ac -> 8002e4f8 phase-cache publication",
                completion_callback="801adee8 passes 801add94 to 80183824; updates only the STRM owner",
            )
    return audit


def main() -> None:
    service_audit = audio_disc_write_audit()
    source = Path(builder.__file__).read_text()
    source = source.replace('"ai_policy.c"', '"down_cycle_native.inc"')
    source = source.replace('"ai_policy_sources.json"', '"down_cycle_sources.json"')
    entry = '        out.append(f"if(!cb_tick(s,0x{function_addr:08x}u))return;")'
    source = source.replace(
        entry,
        entry
        + "\n        if function_addr in AUDIO_DISC_SERVICES:"
        + "\n            result, description = AUDIO_DISC_SERVICES[function_addr]"
        + '\n            out.append(f"/* Not modeled: audio/disc service {function_addr:08x}: {description}. */")'
        + '\n            out.append("return;}" if result is None else f"s->r[3]={result}u;return;}}")'
        + "\n            continue",
    )
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
    # Audio/disc services return above their hardware layer. Keep ordinary
    # cb_mem failure for every out-of-span device access: this module does
    # not supply register values, DMA data, completion IRQs or device timing.
    for name in ("init", "host_call", "call", "service"):
        source = source.replace(f"nflstreet_ai_policy_{name}", f"nflstreet_down_native_{name}")
    scope: dict[str, Any] = {"__file__": str(__file__), "__name__": "down_builder"}
    exec(compile(source, str(__file__), "exec"), scope)
    callbacks = {}
    ui_resource_services = {}
    animation_resource_services = {}
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

        # 8012AA2C registers the UI load callback through 801CFD50.
        # 801CFD90 calls UI+14 and checks NULL before constructing an
        # instance. Its resource manager is independent of football state.
        ui = word(0x803A9A40 - 0x6C94)
        resource_callback = word(ui + 0x14)
        if resource_callback != 0x8012AC68:
            raise RuntimeError(f"Changed UI resource callback {resource_callback:08x} in {context}")
        ui_resource_services[str(context)] = {
            "ui_manager": f"{ui:08x}",
            "load_callback_offset": "14",
            "load_callback": f"{resource_callback:08x}",
            "resource_cache_manager": f"{word(0x803A9A40 - 0x6CE8):08x}",
            "registration": "8012aa2c -> 801cfd50",
            "consumer": "801cfd90; NULL follows unavailable-resource branch",
        }

        # 80016278 opens animdata.dat; 80174FD8 owns the 1C-byte cache
        # and 8-byte request records. Pin the actual indirect callbacks,
        # rather than assuming all resource callbacks are non-gameplay.
        asset_callbacks = {
            "completion": (0x6724, 0x80175FAC),
            "relocation": (0x5798, 0x80151928),
            "release": (0x6720, 0x80151968),
        }
        for name, (offset, target) in asset_callbacks.items():
            if word(0x803A9A40 - offset) != target:
                raise RuntimeError(f"Changed animation resource {name} callback in {context}")
        # 801A735C/801A4C40 can also notify a resource observer or timeout
        # handler. Neither is installed in the checked contexts; do not
        # silently extend the write audit to arbitrary callback code.
        resource_observers = {"observer": word(0x803A9A40 - 0x652C), "timeout": word(0x803A9A40 - 0x6524)}
        if any(resource_observers.values()):
            raise RuntimeError(f"Unreviewed resource observer in {context}: {resource_observers}")
        if word(0x80175448) != 0x9421FFE0:
            raise RuntimeError(f"Changed animation stream request prologue in {context}")
        animation_resource_services[str(context)] = {
            "bank": f"{word(0x803A9A40 - 0x6728):08x}",
            "bank_cache": f"{word(0x803A9A40 - 0x4F6C):08x}",
            "request_records": f"{word(0x803A9A40 - 0x4F5C):08x}",
            "callbacks": {name: f"{target:08x}" for name, (_, target) in asset_callbacks.items()},
            "resource_observers": {name: f"{target:08x}" for name, target in resource_observers.items()},
            "initialization": "80016278 -> 80173b10 / 80174fd8; animdata.dat",
            "request_completion": "80175448 -> 801a735c(callback 80174b20)",
            "boundary": "return zero without submitting a request or publishing bank readiness",
        }

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
    scope["AUDIO_DISC_SERVICES"] = AUDIO_DISC_SERVICES
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
    provenance["audio_disc_services"] = {
        f"{address:08x}": {
            "description": description,
            "return": "preserve incoming GPRs (void)" if result is None else result,
            "native": "early return from the statically named source function",
            "oracle": "listed original entry returns to LR; no native implementation is invoked",
            "disassembly_sha256": hashlib.sha256(
                (builder.CACHE / "disassembly" / f"{address:08x}.txt").read_bytes()
            ).hexdigest(),
            "confidence": "source-audited service boundary; original-module gate pending",
        }
        for address, (result, description) in AUDIO_DISC_SERVICES.items()
    }
    provenance["audio_disc_write_audit"] = service_audit
    provenance["ui_resource_service_bindings"] = ui_resource_services
    provenance["animation_resource_service_bindings"] = animation_resource_services
    provenance["device_boundary"] = {
        "native": "all external device addresses fail the MEM1 span check",
        "scope": "no DVD/DSP/ARAM/audio register emulation, DMA completion, interrupts or streaming",
        "preserved": "shared game/menu event delivery, general asset readers, phase callbacks and all football writes",
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
    provenance["cycle_coverage_schedule"] = {
        "queue": "800266a4 queues 45 ticks, then 80025e90 commits through 800263ec",
        "commit_signal": "800263ec -> 80043b9c sets whistle-state +151 and appends a 120-tick queue entry",
        "check": "24 x 96 retained update windows, then up to 256 checked phase-4 tail updates per window",
        "tail_scope": "all RAM compared per update; no timer edits or callback bypass; lineup/snap after selection",
        "contact": "800f9c50 requires actor +1b0 <= 0, +c bit 800 and either +c bit 10000 or +265 nonzero",
        "fixture": "contact cases set actor +c bits 10800 and height zero, alongside the existing ball state edits",
        "confidence": "candidate until original-module gate passes",
    }
    manifest.write_text(json.dumps(provenance, indent=2, sort_keys=True) + "\n")


if __name__ == "__main__":
    main()
