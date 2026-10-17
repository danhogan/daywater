#!/usr/bin/env python3
"""Integration gate for the controlled 80026C34/80026CCC venue scalar producer."""

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
OUT = PRIVATE / "research-temp/nflstreet-venue-setup-plan02-20261016"
CACHE = Path("/home/dan/daywater/cache/binaries/46e3de854998a0f66c9be8c72a1164d22cd29eb4")
ABI = PRIVATE / "tools/ModernGekko-Template/lib/ModernGekko/include/moderngekko"
FILES = (
    "venue_setup.c",
    "venue_setup.h",
    "venue_setup_original_check.c",
    "build_windows_venue_setup_original_check.sh",
    "verify_venue_setup.py",
    "venue_setup.md",
)
BUILD = "build_windows_venue_setup_original_check.sh"
EXE = "venue-setup-original-check.exe"
SUMMARY = re.compile(
    r"Original venue setup: (\d+) cases \(1 saved \+ (\d+) edited\); 8 raw keys; 9 ordered queries/case; "
    r"base return and all RAM outside \[80f46f00,80f47000\) match; "
    r"thresholds below/equal/above/unordered (\d+)/(\d+)/(\d+)/(\d+); digest ([0-9a-f]{8})\."
)
MUTANTS = (
    ("venue_branch", "case 1:return 0x2c;", "case 1:return 0x2d;"),
    ("resource_increment", "half(p+14,base+9);", "half(p+14,base+8);"),
    (
        "swapped_config",
        "wr(p+0x6c,s->word(s->user,0x8014b6d8u));",
        "wr(p+0x6c,s->word(s->user,0x8014b6d8u));wr(p+0x68,rd(p+0x6c));",
    ),
    ("omitted_state_clear", "p[0x901]=0;", "/* mutant: omitted state clear */"),
    (
        "threshold_inversion",
        "if(!(real(p+0x4c)<=real(mem1+(0x80245824u-BASE))))",
        "if(real(p+0x4c)<=real(mem1+(0x80245824u-BASE)))",
    ),
    ("missing_copy", "wr(p+0x5c,rd(p+0x54));", "/* mutant: omitted copied field */"),
    ("selector_width", "case 0:p[0x44]=0x1f;", "case 0:wr(p+0x44,0x1f);"),
    ("missing_scale", "wr(p+0x54,truncate_word(scaled));", "wr(p+0x54,truncate_word(scaled+(float)value));"),
)
BOUNDARY = (
    "Controlled scalar producer 80026C34 + 80026CCC; unchanged original reader wrappers 8014B588..8014B708, "
    "explicit identical responses at original FNIG 80149614/8014966C returning early. No native stand-ins. "
    "Base return and all MEM1 except [80F46F00,80F47000), plus ordered query/preceding-write observations. "
    "No GX calls or allocations in this closure. 80026CCC is void; host success is not an original return. "
    "Saved-as-is refers to input RAM, with controlled query responses; no claim of natural FNIG/config startup."
)


def fingerprints(paths: list[Path]) -> dict[str, str]:
    return {str(path): digest(path) for path in paths}


def execute(command: list[str], log: Path, timeout: int = 300) -> tuple[int, str]:
    with log.open("w", encoding="utf-8") as output:
        result = subprocess.run(command, stdout=output, stderr=subprocess.STDOUT, timeout=timeout, check=False)
    return result.returncode, log.read_text(encoding="utf-8").replace("\r\n", "\n").strip()


def build(source: Path, out: Path) -> None:
    code, text = execute(["bash", str(source / BUILD), str(out)], out / "build.log")
    if code:
        raise RuntimeError(f"Build failed ({code}); {out / 'build.log'}: {text[-1500:]}")


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
        source = scratch / "venue_setup.c"
        original = source.read_text()
        if original.count(old) != 1:
            raise RuntimeError(f"Mutation anchor {name} is not unique")
        source.write_text(original.replace(old, new))
        build(scratch, scratch)
        code, text = execute(command(scratch / EXE, 15, edited), scratch / "original.log")
        # Compiler errors, native rejection, timeouts, crashes and service-only
        # mismatches never count as kills. Exit 7 is a real RAM/return mismatch.
        caught = code == 7 and re.search(r"MISMATCH (RAM|RETURN) case \d+", text) is not None
        result.update(
            status="caught" if caught else "failed",
            returncode=code,
            diagnostic=text,
            executable_sha256=digest(scratch / EXE),
            source_sha256=digest(source),
            log_sha256=digest(scratch / "original.log"),
        )
    except Exception as error:
        result["error"] = str(error)
    return result


def main() -> None:
    fast = os.environ.get("NFLSTREET_GATE_FAST") == "1"
    contexts = (15,) if fast else (14, 15, 16)
    repeats, edited = (1, 32) if fast else (2, 512)
    out = private(Path(os.environ.get("NFLSTREET_VENUE_SETUP_OUT", str(OUT))))
    if not out.is_relative_to(PRIVATE / "research-temp"):
        raise ValueError("Evidence must stay in private research-temp")
    out.mkdir(parents=True, exist_ok=True)
    lock = out / ".running"
    descriptor = os.open(lock, os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o600)
    os.close(descriptor)
    report = out / "verification.json"
    record: dict[str, Any] = {
        "status": "running",
        "mode": "fast" if fast else "full",
        "boundary": BOUNDARY,
        "stack_exclusion": ["80f46f00", "80f47000"],
        "capture_provenance": {
            "game": "NFL Street GNNE69 North America revision 0",
            "dol_sha1": "46e3de854998a0f66c9be8c72a1164d22cd29eb4",
            "dolphin": "2603a in existing constructor docs; unavailable in some newer gameplay docs",
            "contexts": "position-neutral captures are controlled regression states, not fresh boots",
        },
    }
    report.write_text(json.dumps(record, indent=2) + "\n")
    try:
        originals = baseline_hashes()
        sources = [HERE / file for file in FILES]
        sources.extend(
            [HERE.parents[3] / ".plan_gate", Path(__file__).parents[1] / "menus/verify_controller_checks.py"]
        )
        # No generated native closure: the pinned original DLL is the executable oracle.
        sources.extend(ABI / file for file in ("module_abi.h", "cpu_state.h", "cpu_abi_version.h"))
        for address in ("80026c34", "80026ccc", "80027270", *(f"{0x8014B588 + 0x30 * i:08x}" for i in range(9))):
            sources.extend(CACHE / kind / f"{address}.txt" for kind in ("decompiled", "disassembly"))
        before = fingerprints(sources)
        inputs = fingerprints([PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in contexts])
        record.update(baselines=originals, source_sha256=before, input_sha256=inputs)
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
                    raise RuntimeError(f"Original comparison failed ({code}): {log}; {text[-1500:]}")
                if not all(int(count) for count in match.groups()[2:6]):
                    raise RuntimeError(f"Missing threshold coverage: {log}")
                texts.append(text)
                runs.append({"context": context, "repeat": repeat, "summary": text, "log_sha256": digest(log)})
                print(text, flush=True)
            if len(set(texts)) != 1:
                raise RuntimeError(f"Fresh processes differ in context {context}")
        mutants = []
        if not fast:
            workers = max(1, (os.cpu_count() or 2) // 2)
            with ThreadPoolExecutor(max_workers=workers) as pool:
                mutants = list(pool.map(lambda task: mutant(task, out, edited), MUTANTS))
            record["mutants"] = mutants
            for item in mutants:
                print(f"Mutant {item['name']}: {item['status']}", flush=True)
            if any(item["status"] != "caught" for item in mutants):
                raise RuntimeError("A mutant was not caught by a real RAM/return mismatch")
        if originals != baseline_hashes() or before != fingerprints(sources):
            raise RuntimeError("Protected originals, source/ABI or gate changed while running")
        if inputs != fingerprints([Path(path) for path in inputs]) or executable_hash != digest(out / EXE):
            raise RuntimeError("Private input or comparison executable changed while running")
        record.update(
            status="passed",
            executable_sha256=executable_hash,
            contexts=list(contexts),
            fresh_processes=len(contexts) * repeats,
            cases_per_process=edited + 1,
            mutants_caught=len(mutants),
            mutants_total=len(mutants),
            workers=0 if fast else workers,
        )
    except Exception as error:
        record.update(status="failed", error=str(error))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
        lock.unlink()
    print(
        f"Venue setup: passed; mode={'fast' if fast else 'full'}; contexts={len(contexts)}; "
        f"processes={len(contexts) * repeats}; cases/process={edited + 1}; "
        f"mutants={len(mutants)}/{len(mutants)}; private report {report}"
    )


if __name__ == "__main__":
    main()
