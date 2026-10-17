#!/usr/bin/env python3
"""Host disc catalog / existing skeleton consumer composition gate; no oracle execution."""

import concurrent.futures
import json
import os
import shutil
import subprocess
from pathlib import Path

from verify_controller_checks import PRIVATE, baseline_hashes, digest, private, windows
from verify_player_startup import source_hashes

SOURCES = (
    "skeleton_disc.c",
    "skeleton_disc.h",
    "skeleton_disc_check.c",
    "build_windows_skeleton_disc_check.sh",
    "verify_skeleton_disc.py",
    "skeleton_disc.md",
    "skeleton_startup.c",
    "skeleton_startup.h",
    "skeleton_resource.c",
    "skeleton_resource.h",
    "disc_files.c",
    "disc_files.h",
    "disc_dol.c",
    "disc_dol.h",
    "dol_sections.c",
    "dol_sections.h",
    "archive.c",
    "archive.h",
    "lzh1.c",
    "lzh1.h",
    "export_skeleton_startup_catalog.py",
)
ISO_SHA1 = "813c4511ba3135059a13de424555647e8252254b"
DOL_SHA1 = "46e3de854998a0f66c9be8c72a1164d22cd29eb4"
BOUNDARY = (
    "Host pinned-disc DOL initialized constants and animdata archive feed unchanged skeleton_startup_open. "
    "Catalog spans match independent NSC1 export; six complete resources, token/index metadata and six vectors "
    "match existing consumer with independently exported animdata and NSC1. Distinct identity cases and independent "
    "resource ownership after source close. Snapshots seed tests only. No original module routine, emulator, "
    "all-RAM execution, live startup join, rendering or new gameplay semantics claim."
)
MUTANTS = [
    {"name": name, "old": old, "new": new}
    for name, old, new in (
        ("base-origin", "span(dol,&h,0x80246490u,384)", "span(dol,&h,0x80246496u,384)"),
        ("hand-origin", "span(dol,&h,0x80246610u,12)", "span(dol,&h,0x80246616u,12)"),
        ("hair-origin", "span(dol,&h,0x8024661cu,114)", "span(dol,&h,0x80246622u,114)"),
        ("scale-value", "|scale[3];", "|(scale[3]^1u);"),
        (
            "identity-forwarding",
            "nflstreet_skeleton_startup_open(&s->archive,&c,identity,out)",
            "nflstreet_skeleton_startup_open(&s->archive,&c,identity^32u,out)",
        ),
        (
            "overwrite-owned-output",
            "if(r->bytes||r->size||r->token||r->archive_index||out->base_vectors[i])return 0;",
            "if((r->bytes||r->size||r->token||r->archive_index||out->base_vectors[i])&&identity==0)return 0;",
        ),
        ("archive-origin", 'disc,"animdata.dat",', 'disc,"plaface.dat",'),
    )
]


def run(command: list[str], log: Path) -> str:
    process = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, check=False)
    log.write_text(process.stdout)
    if process.returncode:
        print(process.stdout, flush=True)
        raise subprocess.CalledProcessError(process.returncode, command, output=process.stdout)
    return process.stdout


def independent_catalog(dol: bytes) -> bytes:
    """Raw original export layout, independent of production section parser/adapter."""
    import struct

    def span(address: int, length: int) -> bytes:
        for i in range(18):
            offset, start, size = (struct.unpack_from(">I", dol, base + i * 4)[0] for base in (0, 0x48, 0x90))
            if start <= address and address + length <= start + size and offset + size <= len(dol):
                return dol[offset + address - start : offset + address - start + length]
        raise RuntimeError("Reference catalog range missing from pinned DOL")

    return (
        b"NSC1"
        + struct.pack(">III", 1, 384, 114)
        + span(0x80246490, 384)
        + span(0x80246610, 12)
        + span(0x8024661C, 114)
        + span(0x80246690, 4)
    )


def main() -> None:
    here = Path(__file__).resolve().parent
    fast = os.environ.get("NFLSTREET_GATE_FAST") == "1"
    contexts = (15,) if fast else (14, 15, 16)
    repeats = 1 if fast else 2
    edited = 10 if fast else 200
    out = private(PRIVATE / "research-temp/nflstreet-skeleton-disc-20261017")
    out.mkdir(parents=True, exist_ok=True)
    report = out / "verification.json"
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY, "fast_mode": fast}
    try:
        before = baseline_hashes()
        entries = tuple("native_assets/" + name for name in SOURCES)
        sources = source_hashes(here.parent, entries)
        iso = private(PRIVATE / "research-temp/nflstreet-GNNE69.iso")
        dol = private(PRIVATE / "research-temp/nflstreet-native-windows-local/game/sys/main.dol")
        if digest(iso, "sha1") != ISO_SHA1 or digest(dol, "sha1") != DOL_SHA1:
            raise RuntimeError("ISO/DOL revision mismatch")
        data = private(PRIVATE / "private/nflstreet/GNNE69-r0")
        catalog, archive = data / "native/skeleton-startup.nsc1", data / "assets/animdata.dat"
        if catalog.read_bytes() != independent_catalog(dol.read_bytes()):
            raise RuntimeError("Independent NSC1 differs from pinned DOL source ranges")
        context_paths = {i: private(PRIVATE / f"research-temp/position-neutral-{i}.mem1") for i in contexts}
        nod = private(PRIVATE / "research-temp/nflstreet-live-20261013/build/_deps/nod_prebuilt-src")
        provider = [nod / "include/nod.h", nod / "lib/nod.lib", nod / "bin/nod.dll"]
        inputs = {str(p): digest(p) for p in [iso, dol, catalog, archive, *context_paths.values(), *provider]}

        def build(root: Path, destination: Path) -> None:
            run(
                [
                    "env",
                    f"DAYWATER_NOD={nod}",
                    "bash",
                    str(root / "build_windows_skeleton_disc_check.sh"),
                    str(destination),
                ],
                destination / "build.log",
            )

        def command(exe: Path, context: int, cases: int) -> list[str]:
            return [
                str(exe),
                windows(iso),
                windows(catalog),
                windows(archive),
                windows(context_paths[context]),
                str(context),
                str(cases),
            ]

        build(here, out)
        exe = out / "skeleton-disc-check.exe"
        logs, results = {}, {}
        for context in contexts:
            traces = []
            for repeat in range(repeats):
                log = out / f"check-{context}-{repeat}.log"
                text = run(command(exe, context, edited), log).strip()
                expected = (
                    f"Skeleton disc: {edited + 1} cases, {(edited + 1) * 6} complete resources, "
                    "4 catalog spans, 8 guards, independent output ownership; digest "
                )
                if not text.startswith(expected):
                    raise RuntimeError(f"Incomplete composition coverage: {text}")
                traces.append(digest(log))
                results[f"{context}-{repeat}"] = text
            if not fast and traces[0] != traces[1]:
                raise RuntimeError("Fresh composition traces differ")
            logs[str(context)] = traces

        def mutant_check(mutant: dict[str, str]) -> tuple[str, dict[str, object]]:
            scratch = out / "mutants" / mutant["name"]
            scratch.mkdir(parents=True, exist_ok=True)
            for name in SOURCES:
                shutil.copy2(here / name, scratch / name)
            target = scratch / "skeleton_disc.c"
            text = target.read_text()
            if text.count(mutant["old"]) != 1:
                raise RuntimeError(f"Mutation anchor must match exactly once: {mutant['name']}")
            target.write_text(text.replace(mutant["old"], mutant["new"]))
            build(scratch, scratch)
            mutant_exe = scratch / exe.name
            process = subprocess.run(
                command(mutant_exe, 14, 10), stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, check=False
            )
            log = scratch / "check.log"
            log.write_text(process.stdout)
            if process.returncode != 4 or "comparison failed at line" not in process.stdout:
                print(process.stdout, flush=True)
                raise RuntimeError(f"Mutant {mutant['name']} not caught: {process.returncode}")
            return mutant["name"], {
                "exit_code": process.returncode,
                "diagnostic": process.stdout.strip(),
                "source_sha256": digest(target),
                "executable_sha256": digest(mutant_exe),
                "log_sha256": digest(log),
            }

        mutant_results = {}
        if not fast:
            with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, (os.cpu_count() or 2) // 2)) as pool:
                for name, evidence in pool.map(mutant_check, MUTANTS):
                    mutant_results[name] = evidence
        if before != baseline_hashes() or inputs != {path: digest(Path(path)) for path in inputs}:
            raise RuntimeError("Protected originals or private inputs changed")
        if sources != source_hashes(here.parent, entries):
            raise RuntimeError("Sources changed during verification")
        record.update(
            status="passed",
            baseline=before,
            source_sha256=sources,
            input_sha256=inputs,
            check_log_sha256=logs,
            results=results,
            executable_sha256=digest(exe),
            cases_per_process=edited + 1,
            fresh_processes=len(contexts) * repeats,
            mutants_caught=list(mutant_results),
            mutants_total=0 if fast else len(MUTANTS),
            mutant_results=mutant_results,
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print(
        f"Skeleton disc: contexts {', '.join(map(str, contexts))} x {repeats} runs; "
        f"{len(mutant_results)}/{0 if fast else len(MUTANTS)} mutants caught."
    )
    print("Private report " + str(report))


if __name__ == "__main__":
    main()
