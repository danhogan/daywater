#!/usr/bin/env python3
"""Original-DLL integration gate for the GNNE69 type-zero scene insertion and bounded composition."""

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
OUT = PRIVATE / "research-temp/nflstreet-scene-insert-20261018"
CACHE = Path("/home/dan/daywater/cache/binaries/46e3de854998a0f66c9be8c72a1164d22cd29eb4")
ABI = PRIVATE / "tools/ModernGekko-Template/lib/ModernGekko/include/moderngekko"
FILES = (
    "scene_insert.c",
    "scene_insert.h",
    "scene_insert_original_check.c",
    "build_windows_scene_insert_original_check.sh",
    "verify_scene_insert.py",
    "scene_insert.md",
    "registry_init.c",
    "registry_init.h",
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
EXE = "scene-insert-original-check.exe"
ADDRESSES = (
    "8019573c",
    "801957a4",
    "80180700",
    "80180784",
    "80180d64",
    "80180c6c",
    "80180b94",
    "80180b3c",
    "80180b5c",
    "80180b7c",
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
    r"Original scene insertion: (\d+) cases \(1 saved-input setup \+ (\d+) edited\); "
    r"inserted/exhausted/wrapped (\d+)/(\d+)/(\d+); default/custom/none (\d+)/(\d+)/(\d+); "
    r"head/middle/tail/ties (\d+)/(\d+)/(\d+)/(\d+); composed/factory-return-on-failure (\d+)/(\d+); "
    r"7 host guards; real returns, ordered services and all RAM match outside setup \[80f46f60,80f47008\) or "
    r"insertion \[80f46fb0,80f47008\); digest ([0-9a-f]{8})\."
)
MUTANTS = (
    ("unsigned_order", "*result=a<b?-1:a>b?1:0;", "*result=(int16_t)a<(int16_t)b?-1:(int16_t)a>(int16_t)b?1:0;"),
    ("signed_scan", "if(order>=0)break;", "if(order<=0)break;"),
    ("equal_tie", "if(order>=0)break;", "if(order>0)break;"),
    ("node_prefix", "memset(n,0,16);", "memset(n,0,4);"),
    ("back_link", "wr(next+8,base);", "wr(next+8,prev);"),
    ("node_mark", "wr(n,rd(n)|0x80000000u);", "wr(n,rd(n)|0x40000000u);"),
    ("membership", "obj[2]+1u", "obj[2]+2u"),
    (
        "ignored_insert_status",
        "if(object){uint32_t ignored;if(!nflstreet_scene_insert(x,scene,object,&ignored))return 0;}",
        "if(object){uint32_t ignored;if(!nflstreet_scene_insert(x,scene,object,&ignored))return 0;"
        "if(ignored)object=0;}",
    ),
    ("exhaustion_error", "*result=x->get_error(x->pool->user);", "*result=x->get_error(x->pool->user)^1u;"),
)
BOUNDARY = (
    "Original DLL insertion8019573C/factoryinsert801957A4 and reached node/list/defaultcomparator children "
    "execute unhooked. Default80194F04 is native and original, never a controlled service. Only explicit "
    "external heap8018B7E4/error801AF57C/geterror801AF630, constructor800E1F7C and custom comparator80194FE4 "
    "return early with controlled responses. Exact node/list/object bytes, links, ordering, counterwrap, "
    "returns and ordered service/state observations compare over all24MiB except accounted setup168byte "
    "stack[80F46F60,80F47008) or insertion88byte stack[80F46FB0,80F47008). Setup stack alone is synchronized "
    "before narrower insertion comparisons. Fresh registry->register->scene->factoryinsert composition "
    "uses unchanged adapters. Factory object survives guest insertion error. Pure custom signed comparator "
    "supports INT_MIN/MAX/zero. Scene/list/table/pool stay stable during services. Removal/reordering, "
    "teardown, real class constructors, fresh-match and emulator/live integration remain outside scope."
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
        "stack_exclusion": {"setup_composition": ["80f46f60", "80f47008"], "insertion": ["80f46fb0", "80f47008"]},
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
        for context in contexts:
            texts = []
            for repeat in range(repeats):
                log = out / f"context{context}-{repeat}.log"
                code, text = execute(command(out / EXE, context, edited), log)
                match = SUMMARY.fullmatch(text)
                if code or not match or tuple(map(int, match.groups()[:2])) != (edited + 1, edited):
                    raise RuntimeError(f"Original comparison failed ({code}): {log}; {text}")
                if not all(int(value) for value in match.groups()[2:14]):
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
        f"Scene insertion: passed; mode={'fast' if fast else 'full'}; contexts={len(contexts)}; "
        f"processes={len(contexts) * repeats}; cases/process={edited + 1}; "
        f"mutants={len(mutants)}/{len(mutants)}; private report {report}"
    )


if __name__ == "__main__":
    main()
