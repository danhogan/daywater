#!/usr/bin/env python3
"""Original-DLL integration gate for the GNNE69 palette-pool owner."""

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
OUT = PRIVATE / "research-temp/nflstreet-palette-pool-20261018"
CACHE = Path("/home/dan/daywater/cache/binaries/46e3de854998a0f66c9be8c72a1164d22cd29eb4")
ABI = PRIVATE / "tools/ModernGekko-Template/lib/ModernGekko/include/moderngekko"
FILES = (
    "palette_pool.c",
    "palette_pool.h",
    "palette_pool_original_check.c",
    "build_windows_palette_pool_original_check.sh",
    "verify_palette_pool.py",
    "palette_pool.md",
    "object_pool.h",
)
BUILD = FILES[3]
EXE = "palette-pool-original-check.exe"
ADDRESSES = ("801ca3f4", "801ca494", "801ca4b0", "8018b7b8", "8018b7e4", "80155c40")
SUMMARY = re.compile(
    r"Original palette pool: (\d+) cases \(1 saved \+ (\d+) edited\); "
    r"create/heap-zero/two-root/other (\d+)/(\d+)/(\d+)/(\d+); "
    r"reserve/flip/wrap/payload (\d+)/(\d+)/(\d+)/(\d+); startup/recreate (\d+)/(\d+); "
    r"8 host guards; void entries, ordered heap observations and all RAM outside "
    r"\[80f46ff0,80f47008\) match; digest ([0-9a-f]{8})\."
)
MUTANTS = (
    ("heap_owner", "heap(x->services->user,0,", "heap(x->services->user,1,"),
    ("request_size", "(uint32_t)matrices*48u,0)", "(uint32_t)matrices*64u,0)"),
    ("heap_zero_publication", "wr(g,base);wr(g+4,base);", "if(!base)return 1;wr(g,base);wr(g+4,base);"),
    ("second_root", "base+capacity*48u", "base+capacity*48u+48u"),
    ("flip_index", "previous==0", "previous!=0"),
    ("flip_cursor", "wr(g+16,0);", "wr(g+16,1);"),
    ("rounding", "(requested+1u)&0xfffffffeu", "requested&0xfffffffeu"),
    ("cursor_advance", "wr(g+16,cursor+rounded);", "wr(g+16,cursor+requested);"),
    ("matrix_stride", "data=base+cursor*48u", "data=base+cursor*64u"),
    ("opaque_descriptor_word", "wr(p+8,rounded);", "wr(p+8,rounded);wr(p+4,0);"),
    (
        "payload_clear",
        "wr(g+12,capacity);",
        "wr(g+12,capacity);uint8_t *wipe=span(x->ram,base,48);if(wipe)for(unsigned i=0;i<48;++i)wipe[i]=0;",
    ),
)
BOUNDARY = (
    "Untouched original DLL creator801CA3F4, flip801CA494 and reserve801CA4B0 "
    "execute unhooked. Only external heap API8018B7B8 (source-derived owner0 mapping) returns early "
    "with controlled arguments/results and "
    "global/descriptor/arena observations. All24MiB compare after each component outside exact24-byte "
    "stack[80F46FF0,80F47008). Original routines are void: no incidental guest return assertions. "
    "Includes caller-evidenced create(0x1E78,1), raw cursor/count wrap and heap-zero publication. "
    "Controlled caller payload writes are identical fixture effects, not original palette math. "
    "Host span helper bounds-checks RAM only. No palette transform writer, actual heap, teardown, "
    "fresh match, live or new emulator claim."
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
        "stack_exclusion": ["80f46ff0", "80f47008"],
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
        f"Palette pool: passed; mode={'fast' if fast else 'full'}; contexts={len(contexts)}; "
        f"processes={len(contexts) * repeats}; cases/process={edited + 1}; "
        f"mutants={len(mutants)}/{len(mutants)}; private report {report}"
    )


if __name__ == "__main__":
    main()
