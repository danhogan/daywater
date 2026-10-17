#!/usr/bin/env python3
"""Gate 03: DOL section loading + BSS zeroing verification against original module.
Byte comparison with pinned private DOL, reject wrong revision/truncated/overlapping sections;
no MEM1 read at runtime. Supplies constants plus fresh backing span before any consumer init."""

import concurrent.futures
import json
import os
import shutil
import subprocess
from pathlib import Path

from verify_controller_checks import PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

SOURCES = (
    "dol_sections.c",
    "dol_sections.h",
    "disc_constants_original_check.c",
    "disc_files.c",
    "disc_files.h",
    "archive.c",
    "archive.h",
    "lzh1.c",
    "lzh1.h",
    "build_windows_disc_constants_check.sh",
    "verify_disc_constants.py",
)

ISO_SHA1 = "813c4511ba3135059a13de424555647e8252254b"
BOUNDARY = (
    "Native DOL section loading (text/data sections + BSS zeroing) versus original-module RAM on three saved contexts. "
    "Two fresh Windows processes per context (14/15/16). Original routines execute in the untouched DLL. "
    "All RAM outside the one declared oracle stack [80f46f00,80f47000) must match. "
    "Wrong revision, truncated sections, overlapping sections (text/text, text/data, BSS/loaded), "
    "unaligned addresses/sizes, offsets before header, sections past image end are rejected. "
    "No MEM1 read at runtime; supplies constants plus fresh backing span before any consumer init."
)

MUTANTS = [
    {
        "name": "wrong-revision",
        "file": "disc_constants_original_check.c",
        "old": 'if (!parse_ok || !load_ok || !verify_ok) { fprintf(stderr, "As-is case failed\\n"); return 4; }',
        "new": 'if (!parse_ok || !load_ok) { fprintf(stderr, "As-is case failed\\n"); return 4; }',
    },
    {
        "name": "truncated-section",
        "file": "disc_constants_original_check.c",
        "old": "if (!parse_ok) { truncated++; }",
        "new": "if (!parse_ok) { /* truncated++; */ }",
    },
    {
        "name": "overlapping-text",
        "file": "disc_constants_original_check.c",
        "old": "else if (!load_ok) { overlaps++; }",
        "new": "else if (!load_ok) { /* overlaps++; */ }",
    },
    {
        "name": "bss-overlap",
        "file": "disc_constants_original_check.c",
        "old": "else if (!verify_ok) { revisions++; }",
        "new": "else if (!verify_ok) { /* revisions++; */ }",
    },
    {
        "name": "unaligned-address",
        "file": "disc_constants_original_check.c",
        "old": (
            "if (revisions != 1 || truncated != 1 || overlaps < 2 "
            "|| unaligned < 2 || offset_before != 1 || extends_past != 1) {"
        ),
        "new": (
            "if (revisions != 1 || truncated != 1 || overlaps < 2 "
            "|| unaligned < 2 || offset_before != 1 || extends_past != 0) {"
        ),
    },
    {
        "name": "offset-before-header",
        "file": "disc_constants_original_check.c",
        "old": (
            "if (revisions != 1 || truncated != 1 || overlaps < 2 "
            "|| unaligned < 2 || offset_before != 1 || extends_past != 1) {"
        ),
        "new": (
            "if (revisions != 1 || truncated != 1 || overlaps < 2 "
            "|| unaligned < 2 || offset_before != 0 || extends_past != 1) {"
        ),
    },
    {
        "name": "extends-past-image",
        "file": "disc_constants_original_check.c",
        "old": (
            "if (revisions != 1 || truncated != 1 || overlaps < 2 "
            "|| unaligned < 2 || offset_before != 1 || extends_past != 1) {"
        ),
        "new": (
            "if (revisions != 1 || truncated != 1 || overlaps < 2 "
            "|| unaligned < 2 || offset_before != 1 || extends_past != 0) {"
        ),
    },
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

        context_paths = {i: private(PRIVATE / f"research-temp/position-neutral-{i}.mem1") for i in contexts}

        nod = private(PRIVATE / "research-temp/nflstreet-live-20261013/build/_deps/nod_prebuilt-src")
        provider = [nod / "include/nod.h", nod / "lib/nod.lib", nod / "bin/nod.dll"]

        inputs = {str(path): digest(path) for path in [iso, *context_paths.values(), *provider]}

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
                text = run([str(exe), windows(iso), windows(ram), str(edited_cases)], log)
                # Parse the summary line
                if "Original DOL sections:" not in text:
                    raise RuntimeError(f"DOL constants comparison failed in context {context} repeat {repeat}")
                traces.append(digest(log))
                results[f"{context}-{repeat}"] = text.strip()
            if traces[0] != traces[1] and not fast:
                raise RuntimeError(f"Fresh DOL constants traces differ for context {context}")
            logs[str(context)] = traces

        # Mutants
        caught = []
        mutant_count = 0 if fast else len(MUTANTS)

        def run_mutant(mutant):
            scratch = out / "mutants" / mutant["name"]
            scratch.mkdir(parents=True, exist_ok=True)
            for name in SOURCES:
                shutil.copy2(here / name, scratch / name)
            target = scratch / mutant["file"]
            text = target.read_text()
            if text.count(mutant["old"]) != 1 or "\n" in mutant["old"] or "\n" in mutant["new"]:
                raise RuntimeError(f"Mutation must match exactly one source line: {mutant['name']}")
            target.write_text(text.replace(mutant["old"], mutant["new"]))
            run([*build_cmd, str(scratch)], scratch / "build.log")
            with (scratch / "check.log").open("w") as output:
                process = subprocess.run(
                    [
                        str(scratch / "disc-constants-original-check.exe"),
                        windows(iso),
                        windows(context_paths[14]),
                        "10",
                    ],
                    stdout=output,
                    stderr=subprocess.STDOUT,
                    check=False,
                )
            if process.returncode != 4:
                raise RuntimeError(f"Mutant {mutant['name']} was not caught by comparison: {process.returncode}")
            return mutant["name"]

        if mutant_count > 0:
            max_workers = max(1, os.cpu_count() // 2)
            with concurrent.futures.ThreadPoolExecutor(max_workers=max_workers) as executor:
                futures = {executor.submit(run_mutant, m): m["name"] for m in MUTANTS[:mutant_count]}
                for future in concurrent.futures.as_completed(futures):
                    name = futures[future]
                    try:
                        caught_name = future.result()
                        caught.append(caught_name)
                    except Exception as e:
                        raise RuntimeError(f"Mutant {name} failed: {e}")

        if before != baseline_hashes() or any(digest(Path(path)) != sha for path, sha in inputs.items()):
            raise RuntimeError("Protected originals or private inputs changed")
        if sources != source_hashes(here.parent, entries):
            raise RuntimeError("Sources changed during verification")

        record.update(
            status="passed",
            baseline=before,
            input_sha256=inputs,
            source_sha256=sources,
            original_log_sha256=logs,
            results=results,
            executable_sha256=digest(exe),
            mutants_caught=caught,
            fast_mode=fast,
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
