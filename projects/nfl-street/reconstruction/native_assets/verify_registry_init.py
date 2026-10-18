#!/usr/bin/env python3
"""Original-DLL integration gate for the GNNE69 registry table initialization and bounded composition."""

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
OUT = PRIVATE / "research-temp/nflstreet-registry-init-20261018"
CACHE = Path("/home/dan/daywater/cache/binaries/46e3de854998a0f66c9be8c72a1164d22cd29eb4")
ABI = PRIVATE / "tools/ModernGekko-Template/lib/ModernGekko/include/moderngekko"
FILES = (
    "registry_init.c",
    "registry_init.h",
    "registry_init_original_check.c",
    "build_windows_registry_init_original_check.sh",
    "verify_registry_init.py",
    "registry_init.md",
    "registered_pool.c",
    "registered_pool.h",
    "scene_owner.c",
    "scene_owner.h",
    "object_pool.c",
    "object_pool.h",
    "actor_factory.c",
    "actor_factory.h",
)
BUILD = FILES[3]
EXE = "registry-init-original-check.exe"
ADDRESSES = (
    "80195234",
    "80195368",
    "80195400",
    "801956a4",
    "801954e4",
    "801805e0",
    "80180388",
    "80180340",
    "80181184",
    "801811f0",
    "80181040",
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
    r"Original registry init: (\d+) cases \(1 saved \+ (\d+) edited\); guarded blocked/open/fail (\d+)/(\d+)/(\d+); "
    r"failure-errors zero/high (\d+)/(\d+); low-level success/fail (\d+)/(\d+); "
    r"zero-count publications (\d+); composed stages/constructors (\d+)/(\d+); 7 host guards; "
    r"returns, ordered services and all RAM outside phase stacks \[80f46fe8,80f47008\) or "
    r"\[80f46f60,80f47008\) match; digest ([0-9a-f]{8})\."
)
MUTANTS = (
    ("heap_owner", "heap(x->services->user,1,bytes,0)", "heap(x->services->user,0,bytes,0)"),
    (
        "cleared_record_tail",
        "for(uint32_t i=0;i<count;++i)wr(p+20u*i,0);",
        "for(uint32_t i=0;i<count;++i)for(uint32_t j=0;j<20;++j)p[20u*i+j]=0;",
    ),
    ("last_word", "i<count;++i)", "i+1u<count;++i)"),
    ("count_publication", "wr(count_word,count);", "wr(count_word,count+1u);"),
    ("root_publication", "wr(root_word,table);", "wr(root_word,table+4u);"),
    ("already_initialized", "*result=0x90001u;", "*result=0x90002u;"),
    ("error_propagation", "get_error(x->services->user);", "get_error(x->services->user)^1u;"),
    ("flag_publication", "*flag=1;", "*flag=2;"),
)
BOUNDARY = (
    "Original DLL init80195234 and guarded wrapper80195368 execute unhooked. Pure init phases compare "
    "all24MiB outside32-byte stack[80F46FE8,80F47008), real returns and ordered heap/geterror/state "
    "observations including exact dirty table bytes and adjacent flag bytes. Small composed stages use "
    "unchanged registered_pool/scene_owner/actor_factory/object_pool in order init->register->scene->factory, "
    "with corresponding original routines and all children unhooked; composition stack168bytes "
    "[80F46F60,80F47008). Only original heap8018B7E4/error801AF57C/geterror801AF630 and explicit constructor "
    "identity800E1F7C return early with controlled responses. Table initialization only, not teardown, "
    "natural heap implementation, actual class construction, insertion or fresh-match/emulator integration."
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
        "stack_exclusion": {"init": ["80f46fe8", "80f47008"], "composition": ["80f46f60", "80f47008"]},
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
        table_hex = "80180c2080180c3080180d6480180c6c80180d8c80180e0c80180e6c80180f1880180fac000000200000001001000000"
        for context in contexts:
            with (PRIVATE / f"research-temp/position-neutral-{context}.mem1").open("rb") as stream:
                stream.seek(0x180C20)
                if stream.read(16).hex() != init_hex:
                    raise RuntimeError("Captured initializer differs")
                stream.seek(0x389E88)
                if stream.read(48).hex() != table_hex:
                    raise RuntimeError("Captured type0 dispatch table differs")
        inputs = fingerprints([dol, *(PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in contexts)])
        record["raw_source_evidence"] = {
            "init_address": "80180c20",
            "dol_file_offset": "17dc20",
            "init_hex": init_hex,
            "decoded": ["li r0,0", "stw r0,0x1c(r3)", "stw r0,0x18(r3)", "blr"],
            "table_address": "80389e88",
            "table_hex": table_hex,
            "table_provenance": "Identical bounded 48-byte slices in pinned captures; table creation not reconstructed",
        }
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
                if not all(int(value) for value in match.groups()[2:12]):
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
        f"Registry init: passed; mode={'fast' if fast else 'full'}; contexts={len(contexts)}; "
        f"processes={len(contexts) * repeats}; cases/process={edited + 1}; "
        f"mutants={len(mutants)}/{len(mutants)}; private report {report}"
    )


if __name__ == "__main__":
    main()
