#!/usr/bin/env python3
"""Original-DLL integration gate for the bounded GNNE69 pool kernel."""

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
OUT = PRIVATE / "research-temp/nflstreet-object-pool-20261018"
CACHE = Path("/home/dan/daywater/cache/binaries/46e3de854998a0f66c9be8c72a1164d22cd29eb4")
ABI = PRIVATE / "tools/ModernGekko-Template/lib/ModernGekko/include/moderngekko"
FILES = (
    "object_pool.c",
    "object_pool.h",
    "object_pool_original_check.c",
    "build_windows_object_pool_original_check.sh",
    "verify_object_pool.py",
    "object_pool.md",
)
BUILD = FILES[3]
EXE = "object-pool-original-check.exe"
ADDRESSES = ("80181040", "801811f0", "80181128", "801810ec", "801810a4", "801812d4", "80180328")
SUMMARY = re.compile(
    r"Original object pool: (\d+) cases \(1 saved \+ (\d+) edited\); supplied/heap/failure (\d+)/(\d+)/(\d+); "
    r"narrow/wide (\d+)/(\d+); pops/allocations/exhaustion (\d+)/(\d+)/(\d+); 11 invalid host inputs; "
    r"returns, ordered services and all RAM outside \[80f46fc8,80f47008\) match; "
    r"saved pool ([0-9a-f]{8}) alignment 4; digest ([0-9a-f]{8})\."
)
MUTANTS = (
    ("width255", "return rd(p)<=255u?1u:2u;", "return rd(p)<255u?1u:2u;"),
    ("alignment", "return(n-1u+a)&(0u-a);", "return(n+a)&(0u-a);"),
    ("index_offset", "wr(p+16,pool+indices);", "wr(p+16,pool+indices+4u);"),
    ("reset_order", "nflstreet_pool_push(r,pool,i)", "nflstreet_pool_push(r,pool,n-1u-i)"),
    ("push_count", "wr(p+8,count+1u);", "wr(p+8,count+2u);"),
    ("pop_count", "wr(p+8,n-1u);", "wr(p+8,n);"),
    ("allocation_stride", "index*rd(p+4)", "index*(rd(p+4)+1u)"),
    ("exhaustion_error", "s->error(s->user,0x30003u);", "s->error(s->user,0x30002u);"),
)
BOUNDARY = (
    "Unchanged original DLL runs 80181040/801811F0/80181128/801810EC/801810A4/801812D4/80180328. "
    "Only original heap 8018B7E4 and error 801AF57C return early with controlled responses; exact ordered "
    "service arguments/results and all 24MiB outside the 64-byte original stack [80F46FC8,80F47008) compare. "
    "Real returns: alignment, layout total plus guest outputs, constructor, pop, allocation. Reset/push are void. "
    "Saved-as-is case derives type-zero pool from registry and checks exhaustion; edited scratch cases use "
    "dirty bytes, relative alignments, capacities 0..65536 including255/256, supplied blocks and heap failure. "
    "No registry/scene/actor constructors, original heap implementation, emulator or live startup claim."
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
        for file in FILES[:4]:
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
    contexts, repeats, edited = ((15,), 1, 39) if fast else ((14, 15, 16), 2, 390)
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
        "stack_exclusion": ["80f46fc8", "80f47008"],
    }
    try:
        protected = baseline_hashes()
        sources = [HERE / file for file in FILES]
        sources += [HERE.parent / "menus/verify_controller_checks.py"]
        sources += [ABI / file for file in ("module_abi.h", "cpu_state.h", "cpu_abi_version.h")]
        sources += [CACHE / kind / f"{address}.txt" for address in ADDRESSES for kind in ("decompiled", "disassembly")]
        before = fingerprints(sources)
        inputs = fingerprints([PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in contexts])
        record.update(baselines=protected, source_sha256=before, input_sha256=inputs)
        build(HERE, out)
        executable_hash = digest(out / EXE)
        runs: list[dict[str, Any]] = []
        record["runs"] = runs
        for context in contexts:
            texts = []
            for repeat in range(repeats):
                log = out / f"context{context}-{repeat}.log"
                code, text = execute(command(out / EXE, context, edited), log)
                match = SUMMARY.fullmatch(text)
                if code or not match or tuple(map(int, match.groups()[:2])) != (edited + 1, edited):
                    raise RuntimeError(f"Original comparison failed ({code}): {log}; {text}")
                if not all(int(value) for value in match.groups()[2:10]):
                    raise RuntimeError(f"Missing branch coverage: {log}")
                texts.append(text)
                runs.append({"context": context, "repeat": repeat, "summary": text, "log_sha256": digest(log)})
                print(text, flush=True)
            if len(set(texts)) != 1:
                raise RuntimeError(f"Fresh processes differ in context {context}")
        mutants = []
        if not fast:
            workers = min(len(MUTANTS), max(1, (os.cpu_count() or 2) // 2))
            with ThreadPoolExecutor(max_workers=workers) as pool:
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
        f"Object pool: passed; mode={'fast' if fast else 'full'}; contexts={len(contexts)}; "
        f"processes={len(contexts) * repeats}; cases/process={edited + 1}; "
        f"mutants={len(mutants)}/{len(mutants)}; private report {report}"
    )


if __name__ == "__main__":
    main()
