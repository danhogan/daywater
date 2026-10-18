#!/usr/bin/env python3
"""Original-DLL integration gate for fresh GNNE69 matrix storage and existing stack consumers."""

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
OUT = PRIVATE / "research-temp/nflstreet-matrix-storage-20261018"
CACHE = Path("/home/dan/daywater/cache/binaries/46e3de854998a0f66c9be8c72a1164d22cd29eb4")
ABI = PRIVATE / "tools/ModernGekko-Template/lib/ModernGekko/include/moderngekko"
FILES = (
    "matrix_storage.c",
    "matrix_storage.h",
    "matrix_storage_original_check.c",
    "build_windows_matrix_storage_original_check.sh",
    "verify_matrix_storage.py",
    "matrix_storage.md",
    "matrix_stack.c",
    "matrix_stack.h",
    "draw_engine.c",
    "draw_engine.h",
    "player_draw.h",
    "object_pool.h",
)
BUILD = FILES[3]
EXE = "matrix-storage-original-check.exe"
ADDRESSES = (
    "8018908c",
    "80189664",
    "80189068",
    "801891e8",
    "80189328",
    "801893c8",
    "8018939c",
    "801893ec",
    "8018941c",
    "8018937c",
    "80189460",
    "8018949c",
    "8018b7e4",
    "801af630",
    "801af57c",
    "80155da8",
)
SUMMARY = re.compile(
    r"Original matrix storage: (\d+) cases \(1 saved \+ (\d+) edited\); "
    r"table open/blocked/fail (\d+)/(\d+)/(\d+); stack allocations/fail/zero-depth (\d+)/(\d+)/(\d+); "
    r"startup-sequences/replacements/pop-dirty/underflow (\d+)/(\d+)/(\d+)/(\d+); 7 host guards; "
    r"returns, ordered services and all RAM outside \[80f46fe8,80f47008\) match; digest ([0-9a-f]{8})\."
)
MUTANTS = (
    ("reserved_entries", "count=requested+8u,", "count=requested+7u,"),
    ("table_request", "count*12u,0)", "count*16u,0)"),
    ("failed_table_publication", "wr(table_slot,table);", "if(table)wr(table_slot,table);"),
    ("last_table_entry", "memset(p,0,(size_t)count*12u);", "memset(p,0,(size_t)(count-1u)*12u);"),
    ("slot_request", "depth*64u,0)", "depth*48u,0)"),
    ("clear_all_slots", "for(unsigned i=0;i<16;++i)", "for(unsigned i=0;i<(depth?depth*16u:16u);++i)"),
    ("stack_capacity", "wr(p,depth);", "wr(p,depth+1u);"),
    ("identity_diagonal", "i%5u==0?o:z", "i%4u==0?o:z"),
    ("current_pointer", "*flag=1;", "*flag=1;wr(word(x,x->current_slot),0);"),
    ("consumer_binding", "out->table=rd(table);", "out->table=rd(table)+12u;"),
)
BOUNDARY = (
    "Original DLL table8018908C, actual no-op80189664, lookup80189068, per-stack801891E8 and "
    "identity80189328 execute unhooked. Existing matrix_stack.c select/reset/top/push/copy/pushidentity/pop "
    "consume newly allocated storage and compare against their original functions. Only heap8018B7E4, "
    "geterror801AF630 and error801AF57C return early with controlled responses; returned table/slot "
    "identities are explicit service results. All24MiB compare outside exact32-byte stack "
    "[80F46FE8,80F47008), with real nonvoid returns and ordered service/state observations. "
    "Includes the matrix-only sequence evidenced in80155DA8: open4 then allocate(3,12),(1,12),(2,12), "
    "not the full bootstrap function. No palette-pool implementation, heap implementation, teardown, "
    "fresh match or live/emulator claim."
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
        "stack_exclusion": ["80f46fe8", "80f47008"],
    }
    try:
        protected = baseline_hashes()
        sources = [HERE / file for file in FILES]
        sources += [HERE.parent / "menus/verify_controller_checks.py"]
        sources += [ABI / file for file in ("module_abi.h", "cpu_state.h", "cpu_abi_version.h")]
        sources += [CACHE / kind / f"{address}.txt" for address in ADDRESSES for kind in ("decompiled", "disassembly")]
        before = fingerprints(sources)
        dol = PRIVATE / "research-temp/nflstreet-native-windows-local/game/sys/main.dol"
        inputs = fingerprints([dol, *(PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in contexts)])
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
        f"Matrix storage: passed; mode={'fast' if fast else 'full'}; contexts={len(contexts)}; "
        f"processes={len(contexts) * repeats}; cases/process={edited + 1}; "
        f"mutants={len(mutants)}/{len(mutants)}; private report {report}"
    )


if __name__ == "__main__":
    main()
