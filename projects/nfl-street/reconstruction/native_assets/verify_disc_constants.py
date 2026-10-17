#!/usr/bin/env python3
"""Windows integration gate for host DOL byte preparation, not original startup execution."""

import concurrent.futures
import json
import os
import shutil
import subprocess
from pathlib import Path

from verify_controller_checks import PRIVATE, baseline_hashes, digest, private, windows
from verify_player_startup import source_hashes

SOURCES = (
    "dol_sections.c",
    "dol_sections.h",
    "disc_dol.c",
    "disc_dol.h",
    "disc_constants_original_check.c",
    "build_windows_disc_constants_check.sh",
    "verify_disc_constants.py",
    "disc_constants.md",
)
ISO_SHA1 = "813c4511ba3135059a13de424555647e8252254b"
DOL_SHA1 = "46e3de854998a0f66c9be8c72a1164d22cd29eb4"
BOUNDARY = (
    "Host nod raw_dol bytes versus pinned independent main.dol export; host memory preparation "
    "zeros BSS envelope then overlays initialized sections. All MEM1 bytes compared, including untouched "
    "dirty bytes. No original module, emulator boot, startup execution or catalog consumer integration."
)


def run(command: list[str], log: Path) -> str:
    process = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, check=False)
    log.write_text(process.stdout)
    if process.returncode:
        print(process.stdout, flush=True)
        raise subprocess.CalledProcessError(process.returncode, command, output=process.stdout)
    return process.stdout


# Every mutation changes production behavior; compiler failures never count as catches.
MUTANTS = [
    {"name": name, "file": "dol_sections.c", "old": old, "new": new}
    for name, old, new in (
        (
            "revision-pin",
            "return dol && sha1_equals(dol, dol_size, EXPECTED_DOL_SHA1);",
            "return dol && (sha1_equals(dol, dol_size, EXPECTED_DOL_SHA1) || dol_size > 0);",
        ),
        ("header-offset", "h.bss_address = read_be32(dol + 0xd8);", "h.bss_address = read_be32(dol + 0xdc);"),
        (
            "source-bounds",
            "if (a->offset < 256 || a->offset > dol_size || a->size > dol_size - a->offset) return 0;",
            "if (a->offset < 256) return 0;",
        ),
        (
            "guest-overlap",
            "(uint64_t)b->address < (uint64_t)a->address + a->size) return 0;",
            "(uint64_t)b->address < (uint64_t)a->address + a->size) { /* accept overlap */ }",
        ),
        (
            "file-overlap",
            "(uint64_t)b->offset < (uint64_t)a->offset + a->size) return 0;",
            "(uint64_t)b->offset < (uint64_t)a->offset + a->size) { /* accept overlap */ }",
        ),
        (
            "bss-zero",
            "if (header->bss_size) memset(target + (header->bss_address - target_base), 0, header->bss_size);",
            "if (header->bss_size) memset(target + (header->bss_address - target_base), 1, header->bss_size);",
        ),
        (
            "section-copy",
            "if (a->size) memcpy(target + (a->address - target_base), dol + a->offset, a->size);",
            "if (a->size) memcpy(target + (a->address - target_base), dol + a->offset, a->size - 1);",
        ),
        (
            "bss-wrap",
            "if ((uint64_t)h->bss_address + h->bss_size > UINT64_C(0x100000000)) return 0;",
            "if ((uint64_t)h->bss_address + h->bss_size > UINT64_C(0x100000000)) { /* accept wrap */ }",
        ),
    )
]


def main() -> None:
    here = Path(__file__).resolve().parent
    fast = os.environ.get("NFLSTREET_GATE_FAST") == "1"
    contexts = (15,) if fast else (14, 15, 16)
    repeats = 1 if fast else 2
    edited_cases = 10 if fast else 200

    out = private(PRIVATE / "research-temp/nflstreet-disc-constants-20261017")
    out.mkdir(parents=True, exist_ok=True)
    report = out / "verification.json"
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY, "fast": fast}
    report.write_text(json.dumps(record, indent=2) + "\n")

    try:
        before = baseline_hashes()
        entries = tuple("native_assets/" + name for name in SOURCES)
        sources = source_hashes(here.parent, entries)

        iso = private(PRIVATE / "research-temp/nflstreet-GNNE69.iso")
        if digest(iso, "sha1") != ISO_SHA1:
            raise RuntimeError("Disc image differs from preserved GNNE69 rev0 ISO")

        dol = private(PRIVATE / "research-temp/nflstreet-native-windows-local/game/sys/main.dol")
        if digest(dol, "sha1") != DOL_SHA1:
            raise RuntimeError("Exported DOL differs from pinned GNNE69 revision")

        context_paths = {i: private(PRIVATE / f"research-temp/position-neutral-{i}.mem1") for i in contexts}

        nod = private(PRIVATE / "research-temp/nflstreet-live-20261013/build/_deps/nod_prebuilt-src")
        provider = [nod / "include/nod.h", nod / "lib/nod.lib", nod / "bin/nod.dll"]

        inputs = {str(path): digest(path) for path in [iso, dol, *context_paths.values(), *provider]}

        build_cmd = [
            "env",
            f"DAYWATER_NOD={nod}",
            "bash",
            str(here / "build_windows_disc_constants_check.sh"),
            str(out),
        ]
        run(build_cmd, out / "build.log")

        exe = out / "disc-constants-original-check.exe"
        logs = {}
        results = {}

        for context in contexts:
            ram = context_paths[context]
            traces = []
            for repeat in range(repeats):
                log = out / f"constants-{context}-{repeat}.log"
                text = run([str(exe), windows(iso), windows(dol), windows(ram), str(edited_cases)], log)
                # Parse the summary line
                if "DOL constants:" not in text:
                    raise RuntimeError(f"DOL constants comparison failed in context {context} repeat {repeat}")
                traces.append(digest(log))
                results[f"{context}-{repeat}"] = text.strip()
            if not fast and traces[0] != traces[1]:
                raise RuntimeError(f"Fresh DOL constants traces differ for context {context}")
            logs[str(context)] = traces

        # Mutants
        caught = []
        mutant_count = 0 if fast else len(MUTANTS)

        def run_mutant(mutant: dict[str, str]) -> str:
            scratch = out / "mutants" / mutant["name"]
            scratch.mkdir(parents=True, exist_ok=True)
            for name in SOURCES:
                shutil.copy2(here / name, scratch / name)
            target = scratch / mutant["file"]
            text = target.read_text()
            if text.count(mutant["old"]) != 1 or "\n" in mutant["old"] or "\n" in mutant["new"]:
                raise RuntimeError(f"Mutation must match exactly one source line: {mutant['name']}")
            target.write_text(text.replace(mutant["old"], mutant["new"]))
            run(
                [
                    "env",
                    f"DAYWATER_NOD={nod}",
                    "bash",
                    str(scratch / "build_windows_disc_constants_check.sh"),
                    str(scratch),
                ],
                scratch / "build.log",
            )
            with (scratch / "check.log").open("w") as output:
                process = subprocess.run(
                    [
                        str(scratch / "disc-constants-original-check.exe"),
                        windows(iso),
                        windows(dol),
                        windows(context_paths[14]),
                        "10",
                    ],
                    stdout=output,
                    stderr=subprocess.STDOUT,
                    check=False,
                )
            if process.returncode != 4 or "comparison failed at line" not in (scratch / "check.log").read_text():
                print((scratch / "check.log").read_text(), flush=True)
                raise RuntimeError(f"Mutant {mutant['name']} was not caught by comparison: {process.returncode}")
            return mutant["name"]

        if mutant_count > 0:
            max_workers = max(1, (os.cpu_count() or 2) // 2)
            with concurrent.futures.ThreadPoolExecutor(max_workers=max_workers) as executor:
                futures = {executor.submit(run_mutant, m): m["name"] for m in MUTANTS[:mutant_count]}
                for future in concurrent.futures.as_completed(futures):
                    name = futures[future]
                    try:
                        caught_name = future.result()
                        caught.append(caught_name)
                    except Exception as e:
                        raise RuntimeError(f"Mutant {name} failed: {e}")

        caught = [m["name"] for m in MUTANTS if m["name"] in caught]

        if before != baseline_hashes() or any(digest(Path(path)) != sha for path, sha in inputs.items()):
            raise RuntimeError("Protected originals or private inputs changed")
        if sources != source_hashes(here.parent, entries):
            raise RuntimeError("Sources changed during verification")

        record.update(
            status="passed",
            baseline=before,
            input_sha256=inputs,
            source_sha256=sources,
            check_log_sha256=logs,
            results=results,
            executable_sha256=digest(exe),
            mutants_caught=caught,
            fast_mode=fast,
            cases_per_process=edited_cases + 1,
            fresh_processes=len(contexts) * repeats,
            mutants_total=mutant_count,
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")

    print(
        f"DOL constants: contexts {', '.join(map(str, contexts))} "
        f"x {repeats} runs; {len(caught)}/{mutant_count} mutants caught."
    )
    print("Private report " + str(report))


if __name__ == "__main__":
    main()
