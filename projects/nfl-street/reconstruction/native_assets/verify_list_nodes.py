#!/usr/bin/env python3
"""Original-DLL integration gate for the GNNE69 list-nodes owner."""

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
OUT = PRIVATE / "research-temp/nflstreet-list-nodes-20261018"
CACHE = Path("/home/dan/daywater/cache/binaries/46e3de854998a0f66c9be8c72a1164d22cd29eb4")
ABI = PRIVATE / "tools/ModernGekko-Template/lib/ModernGekko/include/moderngekko"
FILES = (
    "list_nodes.c",
    "list_nodes.h",
    "list_nodes_original_check.c",
    "build_windows_list_nodes_original_check.sh",
    "verify_list_nodes.py",
    "list_nodes.md",
    "object_pool.c",
    "object_pool.h",
    "scene_owner.c",
    "scene_owner.h",
    "scene_insert.c",
    "scene_insert.h",
    "registered_pool.c",
    "registered_pool.h",
    "actor_factory.c",
    "actor_factory.h",
)
BUILD = FILES[3]
EXE = "list-nodes-original-check.exe"
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
)
SUMMARY = re.compile(
    r"Original list nodes: (\d+) cases \(1 saved \+ (\d+) edited\); "
    r"allocate/exhaust/publish (\d+)/(\d+)/(\d+); "
    r"prepend/append/middle/tie (\d+)/(\d+)/(\d+)/(\d+); "
    r"wrap/heap-zero/view-size (\d+)/(\d+)/(\d+); 8 host guards; "
    r"returns, ordered services and all RAM outside phase stacks match; digest ([0-9a-f]{8})\."
)
MUTANTS = (
    ("header_clear", "memset(n,0,16);", "memset(n,0,8);"),
    ("payload_identity", "*result=node+16u;", "*result=node+20u;"),
    ("payload_offset", "node=payload-16u", "node=payload-12u"),
    ("tie_order", "if(order>=0)break;", "if(order>0)break;"),
    ("compare_direction", "position+16u,payload,&order", "payload,position+16u,&order"),
    ("empty_tail", "if(!head){wr(p+28,node);", "if(!head){wr(p+28,0);"),
    ("prepend_link", "wr(h+8,node);", "wr(h+8,0);"),
    ("append_link", "wr(t+12,node);", "wr(t+12,0);"),
    ("count", "wr(p+4,rd(p+4)+1u);", "wr(p+4,rd(p+4)+2u);"),
    ("membership", "rd(n)|0x80000000u", "rd(n)|0x40000000u"),
)
BOUNDARY = (
    "Untouched original DLL list creator801805E0, allocator80180700, mode0 publisher80180784, "
    "type0 callbacks/link helpers/default comparator80194F04 and original pool execute unhooked. "
    "Only external heap8018B7E4 and error801AF57C return early. Exact ordered arguments/results "
    "and bounded list/object observations, actual nonvoid returns and all24MiB compare per phase. "
    "Stack exclusions: creation[80F46F90,80F47008), allocation[80F46FD8,80F47008), "
    "publication[80F46FC0,80F47008); synchronize only compared stack after each phase. "
    "Existing owner/pool/default-comparator reused unchanged; table bytes pinned from captures, "
    "initializer/comparator code pinned against DOL. Callback0 and default-comparator fixtures; "
    "no camera fields, actual camera ordering callback, mode1, teardown, heap implementation or live claim."
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
            "create": ["80f46f90", "80f47008"],
            "allocate": ["80f46fd8", "80f47008"],
            "publish": ["80f46fc0", "80f47008"],
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
        comparator_hex = (
            "8123000081640000a1290010a00b00107c090000418200187c0900403860ffff4d800020386000014e800020386000004e800020"
        )
        with dol.open("rb") as stream:
            stream.seek(0x191F04)
            if stream.read(52).hex() != comparator_hex:
                raise RuntimeError("DOL default comparator differs")
        table_hex = "80180c2080180c3080180d6480180c6c80180d8c80180e0c80180e6c80180f1880180fac000000200000001001000000"
        for context in contexts:
            with (PRIVATE / f"research-temp/position-neutral-{context}.mem1").open("rb") as stream:
                stream.seek(0x180C20)
                if stream.read(16).hex() != init_hex:
                    raise RuntimeError("Captured initializer differs")
                stream.seek(0x194F04)
                if stream.read(52).hex() != comparator_hex:
                    raise RuntimeError("Captured default comparator differs")
                stream.seek(0x389E88)
                if stream.read(48).hex() != table_hex:
                    raise RuntimeError("Captured type0 dispatch table differs")
        inputs = fingerprints([dol, *(PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in contexts)])
        record["raw_source_evidence"] = {
            "init_address": "80180c20",
            "dol_file_offset": "17dc20",
            "init_hex": init_hex,
            "decoded": ["li r0,0", "stw r0,0x1c(r3)", "stw r0,0x18(r3)", "blr"],
            "comparator_address": "80194f04",
            "comparator_dol_offset": "191f04",
            "comparator_hex": comparator_hex,
            "comparator_interpretation": "BE payload object pointers, BEu16 object+10, unsigned key -1/0/+1",
            "table_address": "80389e88",
            "table_hex": table_hex,
            "table_provenance": "Identical bounded 48-byte slices in pinned captures; table creation not reconstructed",
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
            if not all(int(value) for value in match.groups()[2:12]):
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
        f"List nodes: passed; mode={'fast' if fast else 'full'}; contexts={len(contexts)}; "
        f"processes={len(contexts) * repeats}; cases/process={edited + 1}; "
        f"mutants={len(mutants)}/{len(mutants)}; private report {report}"
    )


if __name__ == "__main__":
    main()
