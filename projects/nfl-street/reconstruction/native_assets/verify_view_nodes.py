#!/usr/bin/env python3
"""Original-DLL integration gate for the GNNE69 view-nodes owner."""

from __future__ import annotations

import json
import os
import re
import shutil
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from typing import Any

from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, windows

HERE = Path(__file__).resolve().parent
OUT = PRIVATE / "research-temp/nflstreet-view-nodes-20261018"
CACHE = Path("/home/dan/daywater/cache/binaries/46e3de854998a0f66c9be8c72a1164d22cd29eb4")
ABI = PRIVATE / "tools/ModernGekko-Template/lib/ModernGekko/include/moderngekko"
FILES = (
    "view_nodes.c",
    "view_nodes.h",
    "view_nodes_original_check.c",
    "build_windows_view_nodes_original_check.sh",
    "verify_view_nodes.py",
    "view_nodes.md",
    "list_nodes.c",
    "list_nodes.h",
    "object_pool.c",
    "object_pool.h",
    "scene_owner.c",
    "scene_owner.h",
)
BUILD = FILES[3]
EXE = "view-nodes-original-check.exe"
ADDRESSES = (
    "801805e0",
    "80180328",
    "80180340",
    "80180388",
    "80181184",
    "80181040",
    "801811f0",
    "80181128",
    "801810ec",
    "80180700",
    "801812d4",
    "801810a4",
    "80180784",
    "80180c6c",
    "80180b94",
    "80180b3c",
    "80180b5c",
    "80180b7c",
    "80180d64",
    "8017bc54",
    "801af57c",
    "801defd8",
    "801df14c",
    "801df39c",
    "801df8b4",
    "801879b4",
    "801af630",
)
SUMMARY = re.compile(
    r"Original view nodes: (\d+) cases \(1 saved \+ (\d+) edited\); "
    r"view/empty/callback/exhaust/error-zero (\d+)/(\d+)/(\d+)/(\d+)/(\d+); "
    r"large-copy/compare/wrap/heap-zero (\d+)/(\d+)/(\d+)/(\d+); 8 host guards; "
    r"returns, ordered services and all RAM outside phase stacks match; digest ([0-9a-f]{8})\."
)
MUTANTS = (
    ("view_key", "(uint32_t)d[0]<<8|d[1]", "(uint32_t)d[0]"),
    ("copy_count", "i<count;++i", "i+1<count;++i"),
    ("count_byte", "p[0x2f]=(uint8_t)count;", "p[0x2f]=(uint8_t)(count+1);"),
    ("centering", "centered=delta*half", "centered=delta*(half+half)"),
    ("dimension", "putf(p+20,second);", "putf(p+20,first);"),
    ("flags", "wr(p+0x1c,1);", "wr(p+0x1c,0);"),
    ("dirty_payload", "wr(p,(uint32_t)d[0]", "memset(p,0,0xf8);wr(p,(uint32_t)d[0]"),
    ("view_publication", "*result=payload;return 1;}", "*result=payload+4u;return 1;}"),
    ("callback_fields", "wr(p+12,arg1);", "wr(p+12,arg1^1u);"),
    (
        "exhaustion_return",
        "*result=x->get_error(x->lists.services->user);",
        "x->get_error(x->lists.services->user);*result=0;",
    ),
    ("ordering_wrap", "difference=rd(a)-rd(b)", "difference=rd(a)<rd(b)?0xffffffffu:rd(a)>rd(b)?1u:0u"),
)
BOUNDARY = (
    "Untouched original view801DF14C, callback801DF39C, no-op801DF8B4, ordering801DEF40/801DEE3C, "
    "display read801879B4 and list/pool children execute unhooked. Only heap8018B7E4/error801AF57C/"
    "geterror801AF630 return early; exact ordered arguments/results and both dirty arenas observed. "
    "Actual returns and all24MiB compare after each phase. Creation/view stack[80F46F90,80F47008), "
    "callback stack[80F46FA0,80F47008); leaf comparator has no exclusion. Only compared stack "
    "synchronized before next phase. Native list/owner/pool reused unchanged; original leaves and "
    "constants pinned to DOL/captures. Descriptor count0..50, original constant bytes, signed-display "
    "conversion and dirty payload tested. No projection fields, bootstrap, field/HUD constructors, "
    "viewport-stack publication or live/fresh-match claim."
)


def fingerprints(paths: list[Path]) -> dict[str, str]:
    return {str(path): digest(path) for path in paths}


def execute(command: list[str], log: Path) -> tuple[int, str]:
    with log.open("w", encoding="utf-8") as output:
        result = subprocess.run(command, stdout=output, stderr=subprocess.STDOUT, timeout=600, check=False)
    text = log.read_text(encoding="utf-8").replace("\r\n", "\n").strip()
    if result.returncode:
        print(text, flush=True)
    return result.returncode, text


def build(source: Path, out: Path) -> None:
    code, text = execute(["bash", str(source / BUILD), str(out)], out / "build.log")
    if code:
        raise RuntimeError(f"Build failed ({code}): {text}")


def command(exe: Path, context: int, edited: int) -> list[str]:
    return [
        str(exe),
        windows(PRIVATE / f"research-temp/position-neutral-{context}.mem1"),
        windows(BASELINES["original_dll"][0]),
        str(edited),
    ]


def mutant(task: tuple[str, str, str], out: Path, edited: int) -> dict[str, Any]:
    name, old, new = task
    scratch = private(out / "mutants" / name)
    scratch.mkdir(parents=True, exist_ok=True)
    result: dict[str, Any] = {"name": name, "status": "failed", "old": old, "new": new}
    try:
        for file in (*FILES[:4], *FILES[6:]):
            shutil.copyfile(HERE / file, scratch / file)
        source = scratch / FILES[0]
        original = source.read_text()
        if original.count(old) != 1:
            raise RuntimeError(f"Nonunique mutation anchor: {name}")
        source.write_text(original.replace(old, new))
        build(scratch, scratch)
        code, text = execute(command(scratch / EXE, 15, edited), scratch / "original.log")
        # Only explicit oracle discrepancies count. Build failures, host
        # rejections, timeouts, exceptions and crashes are never kills.
        caught = (code == 7 and re.search(r"MISMATCH (RAM|RETURN) case \d+", text) is not None) or (
            code == 8 and re.search(r"MISMATCH TRACE case \d+", text) is not None
        )
        result.update(
            status="caught" if caught else "failed",
            returncode=code,
            diagnostic=text,
            source_sha256=digest(source),
            executable_sha256=digest(scratch / EXE),
            log_sha256=digest(scratch / "original.log"),
        )
    except Exception as error:
        result["error"] = str(error)
    return result


def main() -> None:
    fast = os.environ.get("NFLSTREET_GATE_FAST") == "1"
    contexts, repeats, edited = ((15,), 1, 40) if fast else ((14, 15, 16), 2, 400)
    out = private(OUT / ("fast" if fast else "full"))
    out.mkdir(parents=True, exist_ok=True)
    lock = out / ".running"
    descriptor = os.open(lock, os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o600)
    os.close(descriptor)
    report = out / "verification.json"
    record: dict[str, Any] = {
        "status": "running",
        "mode": "fast" if fast else "full",
        "boundary": BOUNDARY,
        "phase_stack_exclusions": {
            "create_and_view": ["80f46f90", "80f47008"],
            "callback": ["80f46fa0", "80f47008"],
            "leaf_comparator": [],
        },
    }
    try:
        protected = baseline_hashes()
        sources = [HERE / file for file in FILES]
        sources += [HERE.parent / "menus/verify_controller_checks.py"]
        sources += [ABI / file for file in ("module_abi.h", "cpu_state.h", "cpu_abi_version.h")]
        sources += [CACHE / kind / f"{address}.txt" for address in ADDRESSES for kind in ("decompiled", "disassembly")]
        before = fingerprints(sources)
        dol = PRIVATE / "research-temp/nflstreet-native-windows-local/game/sys/main.dol"
        if digest(dol, "sha1") != "46e3de854998a0f66c9be8c72a1164d22cd29eb4":
            raise RuntimeError("DOL revision changed")
        init_hex = "380000009003001c900300184e800020"
        with dol.open("rb") as stream:
            stream.seek(0x17DC20)
            if stream.read(16).hex() != init_hex:
                raise RuntimeError("DOL type0 initializer differs")
        raw = (
            (0x801DEF40, 0x1DBF40, "80030000806400007c6300504e800020"),
            (0x801DEE3C, 0x1DBE3C, "80030000806400007c6300504e800020"),
            (0x80263CC8, 0x260CC8, "0000000000000000433000008000000043300000000000003f0000003f800000"),
        )
        for guest, offset, expected in raw:
            with dol.open("rb") as stream:
                stream.seek(offset)
                if stream.read(len(expected) // 2).hex() != expected:
                    raise RuntimeError(f"Original DOL evidence changed: {guest:08x}")
        table_hex = "80180c2080180c3080180d6480180c6c80180d8c80180e0c80180e6c80180f1880180fac000000200000001001000000"
        for context in contexts:
            with (PRIVATE / f"research-temp/position-neutral-{context}.mem1").open("rb") as stream:
                for guest, _, expected in (*raw, (0x80180C20, 0x17DC20, init_hex), (0x80389E88, 0, table_hex)):
                    stream.seek(guest - 0x80000000)
                    if stream.read(len(expected) // 2).hex() != expected:
                        raise RuntimeError(f"Capture evidence changed: {guest:08x}")
        inputs = fingerprints([dol, *(PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in contexts)])
        record["raw_source_evidence"] = {
            "leaves_and_constants": [{"guest": f"{g:08x}", "dol_offset": f"{o:x}", "bytes": h} for g, o, h in raw],
            "init": {"guest": "80180c20", "dol_offset": "17dc20", "bytes": init_hex},
            "type_table": {
                "guest": "80389e88",
                "bytes": table_hex,
                "provenance": "Matching captures; table producer not reconstructed",
            },
        }
        record.update(baselines=protected, source_sha256=before, input_sha256=inputs)
        build(HERE, out)
        executable_hash = digest(out / EXE)
        runs: list[dict[str, Any]] = []
        record["runs"] = runs
        jobs = [(context, repeat) for context in contexts for repeat in range(repeats)]

        def run_original(job: tuple[int, int]) -> dict[str, Any]:
            context, repeat = job
            log = out / f"context{context}-{repeat}.log"
            code, output = execute(command(out / EXE, context, edited), log)
            match = SUMMARY.fullmatch(output)
            if code or not match or tuple(map(int, match.groups()[:2])) != (edited + 1, edited):
                raise RuntimeError(f"Original comparison failed ({code}): {log}; {output}")
            if not all(int(value) for value in match.groups()[2:11]):
                raise RuntimeError(f"Missing branch coverage: {log}")
            return {"context": context, "repeat": repeat, "summary": output, "log_sha256": digest(log)}

        # Up to three independent processes, each with its own input RAM/log.
        # map yields stable report order; repeats still run in fresh processes.
        with ThreadPoolExecutor(max_workers=min(3, len(jobs))) as pool:
            for run in pool.map(run_original, jobs):
                runs.append(run)
                print(run["summary"], flush=True)
        for context in contexts:
            if len({run["summary"] for run in runs if run["context"] == context}) != 1:
                raise RuntimeError(f"Fresh processes differ in context {context}")
        mutants = []
        if not fast:
            workers = min(len(MUTANTS), max(1, (os.cpu_count() or 2) // 2))
            with ThreadPoolExecutor(max_workers=min(3, workers)) as pool:
                mutants = list(pool.map(lambda task: mutant(task, out, edited), MUTANTS))
            record["mutants"] = mutants
            for item in mutants:
                print(f"Mutant {item['name']}: {item['status']}", flush=True)
            if any(item["status"] != "caught" for item in mutants):
                raise RuntimeError("A production mutant was not caught by an oracle discrepancy")
        if protected != baseline_hashes() or before != fingerprints(sources):
            raise RuntimeError("Protected originals or source changed during verification")
        if inputs != fingerprints([Path(path) for path in inputs]) or executable_hash != digest(out / EXE):
            raise RuntimeError("Private inputs or comparison executable changed")
        record.update(
            status="passed",
            contexts=list(contexts),
            fresh_processes=len(contexts) * repeats,
            cases_per_process=edited + 1,
            mutants_caught=len(mutants),
            mutants_total=len(mutants),
            executable_sha256=executable_hash,
            provenance={
                "game": "NFL Street GNNE69 North America revision 0",
                "dol_sha1": "46e3de854998a0f66c9be8c72a1164d22cd29eb4",
                "dolphin": "Existing captures; build not established by this gate",
                "captures": "Position-neutral 14/15/16; not fresh boot captures",
            },
        )
    except Exception as error:
        record.update(status="failed", error=str(error))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
        lock.unlink()
    print(
        f"View nodes: passed; mode={'fast' if fast else 'full'}; contexts={len(contexts)}; "
        f"processes={len(contexts) * repeats}; cases/process={edited + 1}; "
        f"mutants={len(mutants)}/{len(mutants)}; private report {report}"
    )


if __name__ == "__main__":
    main()
