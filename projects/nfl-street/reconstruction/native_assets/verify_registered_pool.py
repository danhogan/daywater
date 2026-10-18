#!/usr/bin/env python3
"""Original-DLL integration gate for the GNNE69 registry/factory pool join."""

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
OUT = PRIVATE / "research-temp/nflstreet-registered-pool-20261018"
CACHE = Path("/home/dan/daywater/cache/binaries/46e3de854998a0f66c9be8c72a1164d22cd29eb4")
ABI = PRIVATE / "tools/ModernGekko-Template/lib/ModernGekko/include/moderngekko"
FILES = (
    "registered_pool.c",
    "registered_pool.h",
    "registered_pool_original_check.c",
    "build_windows_registered_pool_original_check.sh",
    "verify_registered_pool.py",
    "registered_pool.md",
    "object_pool.c",
    "object_pool.h",
    "actor_factory.c",
    "actor_factory.h",
)
BUILD = FILES[3]
EXE = "registered-pool-original-check.exe"
ADDRESSES = (
    "80195400",
    "801956a4",
    "80181040",
    "801811f0",
    "80181128",
    "801810ec",
    "801810a4",
    "801812d4",
    "80180328",
    "8017bc54",
    "801af630",
    "801af57c",
)
SUMMARY = re.compile(
    r"Original registered pool: (\d+) cases \(1 saved \+ (\d+) edited\); registrations/failures (\d+)/(\d+); "
    r"objects/nullctor/ctor/exhaustion (\d+)/(\d+)/(\d+)/(\d+); callback reloads (\d+); wide pools (\d+); "
    r"header bytes (\d+); 9 host guards; returns, ordered services and all RAM outside "
    r"\[80f46fb8,80f47008\) match; digest ([0-9a-f]{8})\."
)
MUTANTS = (
    ("registry_capacity", "wr(p+4,capacity);", "wr(p+4,capacity+1u);"),
    ("registry_destructor", "wr(p+12,dtor);", "wr(p+12,dtor^1u);"),
    ("heap_owner", "nflstreet_pool_construct(x->ram,1,capacity", "nflstreet_pool_construct(x->ram,2,capacity"),
    ("pool_publication", "wr(p+16,pool);", "wr(p+16,pool+4u);"),
    ("failure_propagation", "uint32_t code=pool?0:x->services->get_error(x->services->pool.user);", "uint32_t code=0;"),
    ("size_reload", "j->host->object_size=size;", "j->host->object_size=minimum;"),
    (
        "constructor_reload",
        "j->host->construct=ctor?construct:NULL;",
        "j->host->construct=ctor==0x800e1f7cu?construct:NULL;",
    ),
    ("constructor_input", "memory->token,*(const uint32_t *)input);", "memory->token,*(const uint32_t *)input^1u);"),
)
BOUNDARY = (
    "Original DLL registration80195400 and factory801956A4 execute unchanged, including pool closure and "
    "original memset8017BC54. Only original heap8018B7E4/error801AF57C/geterror801AF630 and explicit "
    "constructor identities800E1F7C/800D2E94 return early with controlled responses. All24MiB compare "
    "outside exact80-byte stack[80F46FB8,80F47008), with original registry/factory returns and ordered "
    "service arguments/results plus exact registry/object observations. Constructor service writes and "
    "allocation-error registry mutations are explicit identical fixture effects. Production uses unchanged "
    "actor_factory.c via the pool allocator bridge. Table memory is borrowed; no natural table allocation, "
    "registry lifecycle, destructor call, scene owner/insertion, fresh-match or emulator claim."
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
    contexts, repeats, edited = ((15,), 1, 64) if fast else ((14, 15, 16), 2, 512)
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
        "stack_exclusion": ["80f46fb8", "80f47008"],
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
                if not all(int(value) for value in match.groups()[2:11]):
                    raise RuntimeError(f"Missing branch coverage: {log}")
                if not fast and int(match.group(11)) != 256:
                    raise RuntimeError(f"Missing header coverage: {log}")
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
        f"Registered pool: passed; mode={'fast' if fast else 'full'}; contexts={len(contexts)}; "
        f"processes={len(contexts) * repeats}; cases/process={edited + 1}; "
        f"mutants={len(mutants)}/{len(mutants)}; private report {report}"
    )


if __name__ == "__main__":
    main()
