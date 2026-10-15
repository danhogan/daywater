#!/usr/bin/env python3
"""Construct native C policy functions from pinned GNNE69 disassembly; compilation needs no cache."""

import hashlib
import json
import re
from pathlib import Path

import build_channel_callbacks_native as transcription
import capstone  # type: ignore[import-not-found, import-untyped]

HERE = Path(__file__).resolve().parent
CACHE = transcription.CACHE
# Five requested services, admission/avoidance and saved assignment update.
# State callback targets are extracted from the actual saved table, below;
# they are concrete function addresses, never assumed gameplay semantics.
ROOTS = ("800aac08 800afc88 800a0d10 8006e2e4 8007c180 8005dcf8 800f8088 80094e60 800150e8 800f8214 800f86bc").split()


def main() -> None:
    saved = Path("/mnt/w/Daywater/research-temp/position-neutral-14.mem1").read_bytes()

    def word(a: int) -> int:
        return int.from_bytes(saved[a - 0x80000000 : a - 0x80000000 + 4], "big")

    descriptor = word(word(0x803A9A40 - 0x7860))
    table = word(descriptor + 4)
    rows = int.from_bytes(saved[descriptor - 0x80000000 : descriptor - 0x80000000 + 2], "big")
    targets = sorted({word(table + 24 * row + 4 * slot) for row in range(rows) for slot in range(5)} - {0})
    # Source-owned selector/movement/RNG tables and 8005E724's two
    # admission callbacks at r13-4E18. Actual saved function words supply
    # these indirect targets, which are absent from the direct call graph.
    indirect = {"80244d28": 11, "80254500": 11, "80291934": 3, "803a4c28": 2}
    for address, count in indirect.items():
        targets += [word(int(address, 16) + 4 * i) for i in range(count)]
    # 801EF208/2A0/33C own this event-manager registry; 801EF484 calls
    # every non-null function in each record's +10 array, bounded by +0C.
    # Those registered targets are not direct callgraph edges. Read all
    # three original captures, retaining the actual lists as provenance.
    registered_callbacks = {}
    driver_callbacks = {}
    for context in (14, 15, 16):
        capture = (
            saved
            if context == 14
            else Path(f"/mnt/w/Daywater/research-temp/position-neutral-{context}.mem1").read_bytes()
        )

        def event_word(address: int) -> int:
            offset = address - 0x80000000
            if offset < 0 or offset + 4 > len(capture):
                raise RuntimeError(f"Invalid event registry span {address:08x} in context {context}")
            return int.from_bytes(capture[offset : offset + 4], "big")

        managers, capacity = event_word(0x80350F1C), event_word(0x80350F20)
        if managers > capacity or capacity > len(capture) // 4:
            raise RuntimeError(f"Invalid event registry capacity in context {context}")
        registry = event_word(0x80350F24)
        records = {}
        for manager in range(managers):
            record = event_word(registry + 4 * manager)
            if not record:
                continue
            count = event_word(record + 12)
            if count > len(capture) // 4:
                raise RuntimeError(f"Invalid event callback count in context {context}")
            entries = [event_word(record + 16 + 4 * slot) for slot in range(count)]
            records[str(manager)] = [f"{target:08x}" for target in entries]
            targets += [target for target in entries if target]
        registered_callbacks[str(context)] = records
        # These callback globals are consumed by 80170D1C, 80171C30,
        # 80170C00/80171B2C, 8017199C and their filter helper. Preserve NULL
        # bindings; only actual saved non-null targets extend native code.
        bindings = {
            f"{address:08x}": event_word(address)
            for address in (0x8032B9F0, 0x8032B9F4, 0x8032B9F8, 0x8032B9FC, 0x8032BA00)
        }
        driver_callbacks[str(context)] = {address: f"{target:08x}" for address, target in bindings.items()}
        targets += [target for target in bindings.values() if target]
    targets = sorted(set(targets))
    graph = json.loads((CACHE / "callgraph.json").read_text())
    todo = ROOTS + [f"{pc:08x}" for pc in targets] + transcription.ROOTS
    known = sorted({int(p.stem, 16) for p in (CACHE / "disassembly").glob("*.txt")} | set(targets))
    decoder = capstone.Cs(capstone.CS_ARCH_PPC, capstone.CS_MODE_32 | capstone.CS_MODE_BIG_ENDIAN)
    funcs = {}
    hashes = {}
    captured = {}
    seen: set[str] = set()
    while todo:
        addr = todo.pop()
        if addr in seen:
            continue
        seen.add(addr)
        todo += [v["addr"] for v in graph.get(addr, {}).get("callees", [])]
        # Ghidra's direct graph omits these source-identified function tables.
        # Overapproximate their consecutive code-pointer words from the saved
        # image (including null holes), stopping at the first non-code word.
        # This adds possible native targets, never changes a game's selector,
        # and does NOT assert that the observed run is the table's enum size.
        decompiled = CACHE / "decompiled" / f"{addr}.txt"
        if decompiled.exists():
            for table_hex in re.findall(r"PTR_(?:FUN|LAB)_([0-9a-f]{8})", decompiled.read_text()):
                table_address = int(table_hex, 16)
                for slot in range(256):
                    candidate = word(table_address + 4 * slot)
                    if not candidate:
                        continue
                    if not 0x80004000 <= candidate < 0x80244000 or candidate & 3:
                        break
                    todo.append(f"{candidate:08x}")
                    if candidate not in known:
                        known.append(candidate)
                known.sort()
        path = CACHE / "disassembly" / f"{addr}.txt"
        entry = int(addr, 16)
        ops = []
        if path.exists():
            data = path.read_bytes()
            hashes[addr] = hashlib.sha256(data).hexdigest()
            for line in data.decode().splitlines():
                match = re.fullmatch(r"([\da-f]{8}): (\S+)(?: (.*))?", line)
                if match:
                    ops.append((int(match[1], 16), match[2], match[3].split(",") if match[3] else []))
        else:
            # Some actual table entries have no cached Ghidra function. Decode
            # ONLY at source construction, bounded by the next known entry.
            # Hash those private saved code bytes; do not retain RAM or opcodes.
            end = next(a for a in known if a > entry)
            if end - entry > 16384:
                raise RuntimeError(f"Unbounded uncached function {addr}")
            data = saved[entry - 0x80000000 : end - 0x80000000]
            captured[addr] = {"end": f"{end:08x}", "sha256": hashlib.sha256(data).hexdigest()}
            for ins in decoder.disasm(data, entry):
                op = ins.mnemonic
                args = ins.op_str.replace(" ", "").split(",") if ins.op_str else []
                if op == "mr":
                    op, args = "or", args + [args[1]]
                elif op in ("clrlwi", "clrrwi", "slwi", "srwi", "rotlwi"):
                    n = int(args[2], 0)
                    rot, lo, hi = {
                        "clrlwi": (0, n, 31),
                        "clrrwi": (0, 0, 31 - n),
                        "slwi": (n, 0, 31 - n),
                        "srwi": (32 - n, n, 31),
                        "rotlwi": (n, 0, 31),
                    }[op]
                    op, args = "rlwinm", args[:2] + [str(rot), str(lo), str(hi)]
                elif op in ("mflr", "mtlr", "mtctr", "mfctr"):
                    spr = "LR" if op.endswith("lr") else "CTR"
                    op, args = ("mfspr", args + [spr]) if op.startswith("mf") else ("mtspr", [spr] + args)
                args = [re.sub(r"(-?(?:0x[0-9a-f]+|[0-9]+))\(r", lambda m: hex(int(m[1], 0)) + "(r", a) for a in args]
                # Capstone spells CR subfields as cr7un etc.; canonical
                # Ghidra syntax uses numbered condition-register bits.
                normalized = []
                for operand in args:
                    condition = re.fullmatch(r"cr([0-7])(lt|gt|eq|un|so)", operand)
                    if condition:
                        field, bit_name = condition.groups()
                        operand = str(4 * int(field) + {"lt": 0, "gt": 1, "eq": 2, "un": 3, "so": 3}[bit_name])
                    normalized.append(operand)
                args = normalized
                if op in ("crclr", "crset", "crmove", "crnot"):
                    source_bit = args[-1]
                    op = {"crclr": "crxor", "crset": "creqv", "crmove": "cror", "crnot": "crnor"}[op]
                    args = [args[0], source_bit, source_bit]
                ops.append((ins.address, op, args))
            if not ops or ops[-1][0] + 4 != end:
                raise RuntimeError(f"Incomplete source decode {addr}")
        local_pcs = {pc for pc, _, _ in ops}
        for _, op, args in ops:
            if op in ("bl", "b") and int(args[0], 0) not in local_pcs:
                todo.append(f"{int(args[0], 0):08x}")
        if ops:
            funcs[entry] = ops
    funcs = dict(sorted(funcs.items()))
    # Reuse the already established native C arithmetic/memory transcription,
    # copying its helpers into this independent module; shared sources stay intact.
    helpers = (HERE / "channel_callbacks.c").read_text()
    helpers = helpers[helpers.index("static void cb_fail") : helpers.index("/* The source-derived")]
    helpers = helpers.replace("s->channels->ok=0;", "")
    helpers = helpers.replace("s->channels->span(s->channels->memory", "s->out->ram.span(s->out->ram.memory")
    helpers = helpers.replace("NFLSTREET_CALLBACK_STACK_", "NFLSTREET_AI_POLICY_STACK_")
    helpers = helpers.replace("static __attribute__((noinline))", "static AI_NOINLINE")
    out = [
        "/* Generated by build_ai_policy_native.py; candidate pending original check.",
        f"   {len(funcs)} statically transcribed source functions; no runtime opcodes.",
        "   Each operation retains its original disassembly address.",
        "   Input disassembly SHA256: " + json.dumps(hashes, sort_keys=True),
        "*/",
        '#include "ai_policy.h"',
        '#include "gekko_float.h"',
        "#if defined(_MSC_VER) && !defined(__clang__)",
        "#define AI_NOINLINE __declspec(noinline)",
        "#else",
        "#define AI_NOINLINE __attribute__((noinline))",
        "#endif",
        "typedef struct {nflstreet_ai_policy *out;uint32_t r[32],lr,ctr,cr,ca,pc;",
        " double f[32],ps[32];unsigned budget,depth;} cb_machine;",
        helpers,
    ]
    out += [f"static AI_NOINLINE void cb_{addr:08x}(cb_machine *s);" for addr in funcs]
    unsupported = {}
    for function_addr, ops in funcs.items():
        local = {pc for pc, _, _ in ops}
        labels = set()
        for _, op, args in ops:
            if op == "bctr":
                labels |= local
            if op.startswith("b") and op not in ("bl", "blrl", "blr", "bctr") and args:
                try:
                    target = int(args[-1], 0)
                    if target in local:
                        labels.add(target)
                except ValueError:
                    pass
        out.append(f"static AI_NOINLINE void cb_{function_addr:08x}(cb_machine *s){{")
        out.append(f"if(!cb_tick(s,0x{function_addr:08x}u))return;")
        for pc, op, args in ops:
            if pc in labels:
                out.append(f"L{pc:08x}: if(!cb_tick(s,0x{pc:08x}u))return;")
            if op == "bctrl":
                code = f"{{uint32_t target=s->ctr;s->lr=0x{pc + 4:08x}u;cb_dispatch(s,target);if(!s->out->ok)return;}}"
            elif op == "fnabs":
                code = f"{transcription.fp(args[0])}=-fabs({transcription.fp(args[1])});"
            elif op == "fcmpo":
                code = transcription.instruction(pc, "fcmpu", args.copy(), local)
            elif op == "creqv":
                bits = [transcription.crbit(a) for a in args]
                code = f"cb_setbit(s,{bits[0]},!(cb_bit(s,{bits[1]})^cb_bit(s,{bits[2]})));"
            elif op == "crnor":
                bits = [transcription.crbit(a) for a in args]
                code = f"cb_setbit(s,{bits[0]},!(cb_bit(s,{bits[1]})|cb_bit(s,{bits[2]})));"
            elif op == "fnmadds":
                d, a, b, v = [transcription.fp(a) for a in args]
                code = f"{d}=-nflstreet_gk_fmadds({a},{b},{v});{transcription.fp(args[0], True)}={d};"
            elif op in ("rlwnm", "rotlw"):
                d, a, rotate_reg = [transcription.reg(a) for a in args[:3]]
                mask = f"cb_mask({int(args[3], 0)},{int(args[4], 0)})" if op == "rlwnm" else "0xffffffffu"
                code = f"{d}=cb_rot({a},{rotate_reg})&{mask};"
            else:
                code = transcription.instruction(pc, op, args.copy(), local)
            if "unsupported" in code:
                unsupported[f"{pc:08x}"] = op
                code = code.replace("cb_fail(s,", f"s->out->unsupported=0x{pc:08x}u;cb_fail(s,")
            if any(
                token in code
                for token in (
                    "cb_load(",
                    "cb_store(",
                    "cb_lfs(",
                    "cb_lfd(",
                    "cb_stfs(",
                    "cb_stfd(",
                    "cb_dispatch(",
                    "cb_fail(",
                )
            ):
                code = f"s->pc=0x{pc:08x}u;" + code
            out.append(f"/* {pc:08x} */ {code}")
        out.append("}")
    out += [
        "static void cb_dispatch(cb_machine *s,uint32_t target){",
        "if(!s->out->ok)return;if(++s->depth>256){cb_fail(s,target);--s->depth;return;}",
        "switch(target){",
    ]
    out += [f"case 0x{addr:08x}u:cb_{addr:08x}(s);break;" for addr in funcs]
    out += [
        "default:s->out->unsupported=target;cb_fail(s,target);break;}--s->depth;}",
        "void nflstreet_ai_policy_init(nflstreet_ai_policy *c,nflstreet_actor_ai ram){",
        " if(c){memset(c,0,sizeof *c);c->ram=ram;c->ok=1;}}",
        "int nflstreet_ai_policy_call(nflstreet_ai_policy *c,const nflstreet_ai_call *call,"
        "double weight,uint32_t *result){",
        " if(!c||!call||!result||!c->ram.span)return 0;",
        " c->ok=1;c->failure_pc=c->failure_address=c->unsupported=0;c->failure_budget_exhausted=0;",
        " cb_machine s;memset(&s,0,sizeof s);s.out=c;s.r[1]=NFLSTREET_AI_POLICY_STACK_HI-0x40u;",
        " s.r[2]=0x803b9a40u;s.r[13]=c->ram.r13;s.f[1]=weight;s.budget=5000000u;",
        " for(unsigned k=0;k<6;++k)s.r[3+k]=call->args[k];++c->calls;",
        " static const uint32_t roots[]={0x800aac08u,0x800afc88u,0x800a0d10u,0x8006e2e4u,0x8007c180u};",
        " for(unsigned k=0;k<5;++k)c->entries[k]+=call->pc==roots[k];",
        " cb_dispatch(&s,call->pc);*result=s.r[3];return c->ok;}",
        "int nflstreet_ai_policy_service(void *user,const nflstreet_ai_call *call,uint32_t *result){",
        " return nflstreet_ai_policy_call(user,call,0,result);}",
    ]
    (HERE / "ai_policy.c").write_text("\n".join(out) + "\n")
    (HERE / "ai_policy_sources.json").write_text(
        json.dumps(
            {
                "roots": ROOTS,
                "registered_callbacks": registered_callbacks,
                "driver_callbacks": driver_callbacks,
                "state_callbacks": targets,
                "disassembly_sha256": hashes,
                "captured_functions": captured,
                "unsupported_ops": unsupported,
            },
            indent=2,
            sort_keys=True,
        )
        + "\n"
    )
    print(f"ai_policy.c: {len(funcs)} functions, {sum(len(v) for v in funcs.values())} operations")
    print(f"Unsupported system operations (fail if reached): {unsupported}")


if __name__ == "__main__":
    main()
