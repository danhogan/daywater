#!/usr/bin/env python3
"""Emit instruction-ordered C for the GNNE69 callback call graph, without running it.

This is a source construction tool. The compiled module never reads disassembly,
decodes opcodes, loads the original DLL, or uses this Python program at runtime.
"""

import hashlib
import json
import re
from pathlib import Path

CACHE = Path("/home/dan/daywater/cache/binaries/46e3de854998a0f66c9be8c72a1164d22cd29eb4")
# All non-null functions in the 224-entry gameplay table (80382950), plus
# the program event and four indirect weight-curve functions.
ROOTS = """800082e8 8001532c 80019f58 8001f1d4 800210dc 800288ac
8004e308 80055ef4 80060fc4 80061658 800621e0 800635b8 80064260 800643d8
80065398 8006577c 80066490 800669b0 8006707c 800674d4 80067b2c 80067fa4
80068e8c 80071ee4 80079464 8008098c 800914dc 8009737c 8009cba8 8009ef58
800a7e38 80178e78 80178e7c 80178f04 80178f28""".split()


def num(v: str) -> int:
    return int(v, 0)


def reg(v: str) -> str:
    return f"s->r[{int(v[1:])}]"


def fp(v: str, pair: bool = False) -> str:
    return f"s->{'ps' if pair else 'f'}[{int(v[1:])}]"


def imm(v: str) -> str:
    return f"0x{num(v) & 0xFFFFFFFF:08x}u"


def crbit(v: str) -> int:
    if v in ("lt", "gt", "eq", "so"):
        return ("lt", "gt", "eq", "so").index(v)
    m = re.fullmatch(r"4\*cr(\d)\+(lt|gt|eq|so)", v)
    if m:
        return 4 * int(m[1]) + crbit(m[2])
    return num(v)


def ea(v: str) -> tuple[str, str]:
    match = re.fullmatch(r"(-?0x[\da-f]+)\((r\d+)\)", v)
    assert match is not None, v
    off, base = match.groups()
    return f"({('0u' if base == 'r0' else reg(base))}+{imm(off)})", base


def instruction(pc: int, op: str, a: list[str], local: set[int]) -> str:
    d = reg(a[0]) if a and a[0].startswith("r") else ""

    def signed(v: str) -> str:
        return f"(int32_t){v}"

    if op in ("li", "lis"):
        return f"{d}={imm(a[1])}{'<<16' if op == 'lis' else ''};"
    if op in ("addi", "addis", "subi", "subis", "mulli"):
        src = "0u" if a[1] == "r0" and op in ("addi", "addis") else reg(a[1])
        k = imm(a[2]) + ("<<16" if op in ("addis", "subis") else "")
        operator = "*" if op == "mulli" else "-" if op.startswith("sub") else "+"
        return f"{d}={src}{operator}({k});"
    plain = op.rstrip(".")
    if plain in ("or", "xor", "and", "andc", "nor", "nand", "eqv", "add", "subf", "mullw"):
        x, y = reg(a[1]), reg(a[2])
        expr = {
            "or": f"{x}|{y}",
            "xor": f"{x}^{y}",
            "and": f"{x}&{y}",
            "andc": f"{x}&~{y}",
            "nor": f"~({x}|{y})",
            "nand": f"~({x}&{y})",
            "eqv": f"~({x}^{y})",
            "add": f"{x}+{y}",
            "subf": f"{y}-{x}",
            "mullw": f"{x}*{y}",
        }[plain]
        return f"{d}={expr};" + (f"cb_cmp(s,0,(int32_t){d},0);" if op.endswith(".") else "")
    if plain in ("ori", "oris", "xori", "xoris", "andi", "andis"):
        k = f"({imm(a[2])}{'<<16' if plain.endswith('s') else ''})"
        operator = "|" if plain.startswith("or") else "^" if plain.startswith("xor") else "&"
        return f"{d}={reg(a[1])}{operator}{k};" + (f"cb_cmp(s,0,(int32_t){d},0);" if op.endswith(".") else "")
    if plain in ("neg", "extsb", "extsh", "cntlzw"):
        src = reg(a[1])
        expr = {
            "neg": f"0u-{src}",
            "extsb": f"(uint32_t)(int32_t)(int8_t){src}",
            "extsh": f"(uint32_t)(int32_t)(int16_t){src}",
            "cntlzw": f"cb_clz({src})",
        }[plain]
        return f"{d}={expr};" + (f"cb_cmp(s,0,(int32_t){d},0);" if op.endswith(".") else "")
    if plain in ("rlwinm", "rlwimi"):
        mask = f"cb_mask({num(a[3])},{num(a[4])})"
        rot = f"cb_rot({reg(a[1])},{num(a[2])})"
        return (
            f"{d}="
            + (f"({d}&~{mask})|" if plain == "rlwimi" else "")
            + f"({rot}&{mask});"
            + (f"cb_cmp(s,0,(int32_t){d},0);" if op.endswith(".") else "")
        )
    if plain in ("slw", "srw", "sraw", "srawi"):
        n = str(num(a[2])) if plain == "srawi" else f"({reg(a[2])}&63u)"
        if plain.startswith("sra"):
            expr = f"cb_sra(s,{reg(a[1])},{n})"
        else:
            expr = f"({n}>=32?0u:{reg(a[1])}{'<<' if plain == 'slw' else '>>'}({n}&31u))"
        return f"{d}={expr};" + (f"cb_cmp(s,0,(int32_t){d},0);" if op.endswith(".") else "")
    if plain in ("divw", "divwu", "mulhw", "mulhwu"):
        x, y = reg(a[1]), reg(a[2])
        if plain.startswith("div"):
            return f"{d}=cb_div({x},{y},{int(plain == 'divw')});"
        expr = f"(int64_t)(int32_t){x}*(int64_t)(int32_t){y}" if plain == "mulhw" else f"(uint64_t){x}*{y}"
        return f"{d}=(uint32_t)(({expr})>>32);"
    if plain in ("addc", "adde", "addze", "subfc", "subfe", "subfze", "subfic", "addic", "subic"):
        x = reg(a[1])
        y = reg(a[2]) if len(a) > 2 and a[2].startswith("r") else imm(a[2]) if len(a) > 2 else "0u"
        carry = "s->ca" if plain in ("adde", "addze", "subfe", "subfze") else "0u"
        if plain.startswith("sub"):
            x = "~" + x
            carry = "s->ca" if plain in ("subfe", "subfze") else "1u"
            if plain == "subfze":
                y = "0u"
            if plain == "subic":
                x, y, carry = reg(a[1]), "~" + imm(a[2]), "1u"
        return f"cb_addcarry(s,{int(a[0][1:])},{x},{y},{carry});" + (
            f"cb_cmp(s,0,(int32_t){d},0);" if op.endswith(".") else ""
        )
    if op in ("cmpw", "cmpwi", "cmplw", "cmplwi", "fcmpu"):
        field = int(a.pop(0)[2:]) if a[0].startswith("cr") else 0
        if op == "fcmpu":
            return f"cb_fcmp(s,{field},{fp(a[0])},{fp(a[1])});"
        x = reg(a[0])
        y = imm(a[1]) if op.endswith("i") else reg(a[1])
        if not op.startswith("cmpl"):
            x, y = signed(x), signed(y)
        return f"cb_cmp(s,{field},{x},{y});"
    if op in ("cror", "crxor", "crand"):
        operator = {"cror": "|", "crxor": "^", "crand": "&"}[op]
        return f"cb_setbit(s,{crbit(a[0])},cb_bit(s,{crbit(a[1])}){operator}cb_bit(s,{crbit(a[2])}));"
    if op == "mcrf":
        cr_x, cr_y = int(a[0][2:]), int(a[1][2:])
        return f"s->cr=(s->cr&~(15u<<{28 - 4 * cr_x}))|(((s->cr>>{28 - 4 * cr_y})&15u)<<{28 - 4 * cr_x});"
    if op == "mfcr":
        return f"{d}=s->cr;"
    if op == "mtcrf":
        return f"cb_mtcr(s,{imm(a[0])},{reg(a[1])});"
    if op in ("mfspr", "mtspr"):
        spr = a[1] if op == "mfspr" else a[0]
        spr_target = {"LR": "s->lr", "CTR": "s->ctr", "XER": "(s->ca<<29)"}.get(spr)
        if spr_target:
            if op == "mtspr" and spr == "XER":
                return f"s->ca=({reg(a[1])}>>29)&1u;"
            return f"{d}={spr_target};" if op == "mfspr" else f"{spr_target}={reg(a[1])};"
    if op in ("lmw", "stmw"):
        addr, _ = ea(a[1])
        i = int(a[0][1:])
        body = f"s->r[j]=cb_load(s,t+4*(j-{i}),4);" if op == "lmw" else f"cb_store(s,t+4*(j-{i}),s->r[j],4);"
        return f"{{uint32_t t={addr};for(unsigned j={i};j<32;++j){{{body}}}}}"
    if re.fullmatch(r"(lwz|lbz|lhz|lha|stw|stb|sth|lfs|lfd|stfs|stfd)(u|x|ux)?", op):
        m = re.fullmatch(r"(lwz|lbz|lhz|lha|stw|stb|sth|lfs|lfd|stfs|stfd)(u|x|ux)?", op)
        assert m is not None
        baseop, suffix = m[1], m[2] or ""
        if "x" in suffix:
            base = a[1]
            addr = f"({('0u' if base == 'r0' else reg(base))}+{reg(a[2])})"
        else:
            addr, base = ea(a[1])
        expr = ""
        width = 1 if baseop in ("lbz", "stb") else 2 if baseop in ("lhz", "lha", "sth") else 4
        if baseop in ("lfs", "lfd"):
            expr = f"{fp(a[0])}=cb_{baseop}(s,t);" + (f"{fp(a[0], True)}={fp(a[0])};" if baseop == "lfs" else "")
        elif baseop in ("stfs", "stfd"):
            expr = f"cb_{baseop}(s,t,{fp(a[0])});"
        elif baseop.startswith("l"):
            cast = "(uint32_t)(int32_t)(int16_t)" if baseop == "lha" else ""
            expr = f"{d}={cast}cb_load(s,t,{width});"
        else:
            expr = f"cb_store(s,t,{d},{width});"
        if "u" in suffix:
            expr += f"{reg(base)}=t;"
        return f"{{uint32_t t={addr};{expr}}}"
    if op in ("psq_l", "psq_st") and a[3] == "GQR0":
        addr, _ = ea(a[1])
        one = num(a[2])
        x, y = fp(a[0]), fp(a[0], True)
        if op == "psq_l":
            return f"{{uint32_t t={addr};{x}=cb_lfs(s,t);{y}=" + ("1.0;" if one else "cb_lfs(s,t+4);") + "}"
        return f"{{uint32_t t={addr};cb_stfs(s,t,{x});" + ("" if one else f"cb_stfs(s,t+4,{y});") + "}"
    if op in ("fmr", "fneg", "fabs", "frsp", "fctiwz"):
        x, y = fp(a[0]), fp(a[1])
        expr = {
            "fmr": y,
            "fneg": "-" + y,
            "fabs": f"fabs({y})",
            "frsp": f"(double)(float){y}",
            "fctiwz": f"cb_fctiwz({y})",
        }[op]
        return f"{x}={expr};" + (f"{fp(a[0], True)}={x};" if op == "frsp" else "")
    if op in ("fadd", "fsub", "fmul", "fdiv", "fadds", "fsubs", "fmuls", "fdivs", "fmadd", "fmsub", "fmadds", "fmsubs"):
        x, y, z = fp(a[0]), fp(a[1]), fp(a[2])
        single = op.endswith("s")
        if len(a) == 4:
            b = fp(a[3])
            expr = f"nflstreet_gk_{op}({y},{z},{b})" if single else f"fma({y},{z},{'-' if op == 'fmsub' else ''}{b})"
        else:
            operator = {"fa": "+", "fs": "-", "fm": "*", "fd": "/"}[op[:2]]
            expr = f"nflstreet_gk_{op[1:]}({y},{z})" if single else f"{y}{operator}{z}"
        return f"{x}={expr};" + (f"{fp(a[0], True)}={x};" if single else "")
    if op.startswith("ps_"):
        x, y = fp(a[0]), fp(a[0], True)
        if op in ("ps_merge00", "ps_merge01", "ps_merge10", "ps_merge11"):
            v, w = fp(a[1], op[-2] == "1"), fp(a[2], op[-1] == "1")
        elif op in ("ps_sum0", "ps_sum1"):
            total = f"nflstreet_gk_adds({fp(a[1])},{fp(a[3], True)})"
            v, w = (total, fp(a[2], True)) if op == "ps_sum0" else (fp(a[2]), total)
        elif op in ("ps_mul", "ps_muls0", "ps_muls1", "ps_madd", "ps_madds0", "ps_madds1"):
            values = []
            for pair in (False, True):
                cpair = op.endswith("1") or pair and op in ("ps_mul", "ps_madd")
                args = [fp(a[1], pair), fp(a[2], cpair)]
                if len(a) == 4:
                    args.append(fp(a[3], pair))
                values.append(f"nflstreet_gk_{'fmadds' if len(a) == 4 else 'muls'}({','.join(args)})")
            v, w = values
        else:
            return f"cb_fail(s,0x{pc:08x}u);return; /* unsupported {op} */"
        return f"{{double x={v},y={w};{x}=x;{y}=y;}}"
    if op == "blr":
        return "return;"
    if op in ("b", "bl"):
        target = num(a[0])
        call = f"cb_dispatch(s,0x{target:08x}u);"
        if op == "bl":
            return f"s->lr=0x{pc + 4:08x}u;{call}if(!s->out->ok)return;"
        return f"goto L{target:08x};" if target in local else call + "return;"
    if op in ("blrl", "bctr"):
        if op == "blrl":
            return f"{{uint32_t t=s->lr;s->lr=0x{pc + 4:08x}u;cb_dispatch(s,t);if(!s->out->ok)return;}}"
        cases = "".join(f"case 0x{k:08x}u:goto L{k:08x};" for k in sorted(local))
        return f"switch(s->ctr){{{cases}default:cb_dispatch(s,s->ctr);return;}}"
    if op in ("bdnz", "bdz"):
        target = num(a[0])
        cond = "--s->ctr!=0" if op == "bdnz" else "--s->ctr==0"
        return f"if({cond})goto L{target:08x};"
    branch = re.fullmatch(r"b(eq|ne|lt|le|gt|ge|so|ns)(lr)?", op)
    if branch:
        field = int(a.pop(0)[2:]) if a and a[0].startswith("cr") else 0
        which, invert = {
            "eq": (2, 0),
            "ne": (2, 1),
            "lt": (0, 0),
            "ge": (0, 1),
            "gt": (1, 0),
            "le": (1, 1),
            "so": (3, 0),
            "ns": (3, 1),
        }[branch[1]]
        cond = f"{'!' if invert else ''}cb_bit(s,{4 * field + which})"
        if branch[2]:
            return f"if({cond})return;"
        target = num(a[-1])
        action = f"goto L{target:08x};" if target in local else f"cb_dispatch(s,0x{target:08x}u);return;"
        return f"if({cond}){{{action}}}"
    if op in ("sync", "isync", "dcbi", "dcbst", "dcbf", "icbi"):
        return "(void)s; /* cache ordering has no effect on the RAM span */"
    return f"cb_fail(s,0x{pc:08x}u);return; /* unsupported {op} */"


def main() -> None:
    graph = json.loads((CACHE / "callgraph.json").read_text())
    seen, todo = set(), list(ROOTS)
    while todo:
        graph_addr = todo.pop()
        if graph_addr in seen:
            continue
        seen.add(graph_addr)
        todo += [v["addr"] for v in graph.get(graph_addr, {}).get("callees", [])]
    funcs: dict[int, list[tuple[int, str, list[str]]]] = {}
    hashes: dict[str, str] = {}
    for graph_addr in sorted(seen):
        path = CACHE / "disassembly" / (graph_addr + ".txt")
        if not path.is_file():
            continue
        data = path.read_bytes()
        hashes[graph_addr] = hashlib.sha256(data).hexdigest()
        ops = []
        for line in data.decode().splitlines():
            m = re.fullmatch(r"([\da-f]{8}): (\S+)(?: (.*))?", line)
            if m:
                ops.append((int(m[1], 16), m[2], (m[3] or "").split(",") if m[3] else []))
        if ops:
            funcs[int(graph_addr, 16)] = ops
    out = [
        "/* Generated by build_channel_callbacks_native.py. Candidate until oracle comparison.",
        f"   {len(funcs)} source functions; disassembly SHA256 manifest follows.",
        "   " + json.dumps(hashes, sort_keys=True),
        "*/",
    ]
    out += [f"static __attribute__((noinline)) void cb_{addr:08x}(cb_machine *s);" for addr in funcs]
    for addr, ops in funcs.items():
        local = {pc for pc, _, _ in ops}
        labels = {addr}
        for _, op, args in ops:
            if op == "bctr":
                labels |= local
            if op.startswith("b") and op not in ("bl", "blrl", "blr", "bctr") and args:
                try:
                    target = num(args[-1])
                    if target in local:
                        labels.add(target)
                except ValueError:
                    pass
        out.append(f"static void cb_{addr:08x}(cb_machine *s){{")
        for pc, op, args in ops:
            if pc in labels:
                out.append(f"L{pc:08x}: if(!cb_tick(s,0x{pc:08x}u))return;")
            code = instruction(pc, op, args.copy(), local)
            out.append(f"/* {pc:08x} */ {code}")
        out.append("}")
    group_test = "||".join(f"target==0x{addr}u" for addr in ROOTS if addr.startswith("800") and addr != "8001532c")
    out += [
        "static void cb_dispatch(cb_machine *s,uint32_t target){",
        "if(!s->out->ok)return;if(++s->depth>256){cb_fail(s,target);--s->depth;return;}",
        "if(target==0x8001532cu)++s->out->events;",
        f"if(s->lr==0x8017793cu&&({group_test}))++s->out->groups;",
        "if(target==0x801777a0u)++s->out->operands;",
        "switch(target){",
    ]
    out += [f"case 0x{addr:08x}u:cb_{addr:08x}(s);break;" for addr in funcs]
    out += ["default:cb_fail(s,target);break;}--s->depth;}"]
    output_path = Path(__file__).with_name("channel_callbacks_native.inc")
    output_path.write_text("\n".join(out) + "\n")
    print(f"Wrote {output_path.name}: {len(funcs)} functions; {sum(len(v) for v in funcs.values())} operations")


if __name__ == "__main__":
    main()
