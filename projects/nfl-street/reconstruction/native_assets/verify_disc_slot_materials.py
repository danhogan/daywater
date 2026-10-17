#!/usr/bin/env python3
"""Host owned SMC1 extraction/composition gate; no original boot execution."""

import concurrent.futures
import json
import os
import shutil
import subprocess
from pathlib import Path

from export_slot_material_catalog import export
from verify_controller_checks import PRIVATE, baseline_hashes, digest, private, windows
from verify_player_startup import source_hashes

SOURCES = (
    "disc_slot_materials.c",
    "disc_slot_materials.h",
    "disc_slot_materials_check.c",
    "build_windows_disc_slot_materials_check.sh",
    "verify_disc_slot_materials.py",
    "disc_slot_materials.md",
    "disc_dol.c",
    "disc_dol.h",
    "dol_sections.c",
    "dol_sections.h",
    "slot_materials.c",
    "slot_materials.h",
    "slot_textures.h",
    "gear_texture.h",
    "texture_apply.h",
    "archive.h",
    "texture.h",
    "face_texture.h",
    "face_texture.c",
    "texture.c",
    "archive.c",
    "lzh1.c",
    "lzh1.h",
    "export_slot_material_catalog.py",
    "export_gear_texture_catalog.py",
    "../menus/team_catalog.h",
)
BOUNDARY = (
    "Pinned DOL/disc extraction versus independent Python SMC1; existing parser/configure/hook composition. "
    "Saved contexts seed fixtures only; no original module, boot or emulator claim."
)


def run(command: list[str], log: Path) -> str:
    process = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, check=False)
    log.write_text(process.stdout)
    if process.returncode:
        print(process.stdout, flush=True)
        raise subprocess.CalledProcessError(process.returncode, command, output=process.stdout)
    return process.stdout


MUTANTS = [
    ("revision-pin", "!nflstreet_dol_verify_revision(dol,size)", "(!dol || size < 256)"),
    ("slot-pointer", "SM_BINDING+32u*i,4", "SM_BINDING+32u*((i+1u)%25u),4"),
    ("setting-word", "sm_append(&b,p+4,24)", "sm_append(&b,p,24)"),
    ("descriptor-texture", "sm_name(&b,sm_read(p+12))", "sm_name(&b,sm_read(p+4))"),
    ("shader-stage", "sm_append(&b,p,20u*stages)", "sm_append(&b,p+1,20u*stages)"),
    ("occupied-output", "if(!sm_empty(out)||!nflstreet_dol_verify_revision", "if(!out||!nflstreet_dol_verify_revision"),
    ("name-padding", "uint8_t name[16]={0};", "uint8_t name[16];memset(name,0xcc,sizeof name);"),
]


def main() -> None:
    here = Path(__file__).resolve().parent
    fast = os.environ.get("NFLSTREET_GATE_FAST") == "1"
    contexts, repeats, cases = ((15,), 1, 10) if fast else ((14, 15, 16), 2, 200)
    out = private(PRIVATE / "research-temp/nflstreet-disc-materials-20261017")
    out.mkdir(parents=True, exist_ok=True)
    report = out / "verification.json"
    record = {"status": "running", "boundary": BOUNDARY, "fast": fast}
    report.write_text(json.dumps(record, indent=2) + "\n")
    try:
        before = baseline_hashes()
        entries = tuple("native_assets/" + name for name in SOURCES)
        sources = source_hashes(here.parent, entries)
        iso = private(PRIVATE / "research-temp/nflstreet-GNNE69.iso")
        dol = private(PRIVATE / "research-temp/nflstreet-native-windows-local/game/sys/main.dol")
        if digest(iso, "sha1") != "813c4511ba3135059a13de424555647e8252254b":
            raise RuntimeError("Unexpected preserved ISO")
        if digest(dol, "sha1") != "46e3de854998a0f66c9be8c72a1164d22cd29eb4":
            raise RuntimeError("Unexpected DOL revision")
        reference = out / "reference.smc1"
        export(reference)
        rams = {i: private(PRIVATE / f"research-temp/position-neutral-{i}.mem1") for i in contexts}
        nod = private(PRIVATE / "research-temp/nflstreet-live-20261013/build/_deps/nod_prebuilt-src")
        inputs = {
            str(p): digest(p)
            for p in [
                iso,
                dol,
                reference,
                *rams.values(),
                nod / "include/nod.h",
                nod / "lib/nod.lib",
                nod / "bin/nod.dll",
            ]
        }
        run(["bash", str(here / "build_windows_disc_slot_materials_check.sh"), str(out)], out / "build.log")
        exe = out / "disc-slot-materials-check.exe"
        results, traces = {}, {}
        for context in contexts:
            hashes = []
            for repeat in range(repeats):
                log = out / f"materials-{context}-{repeat}.log"
                result = run(
                    [str(exe), windows(iso), windows(dol), windows(reference), windows(rams[context]), str(cases)], log
                )
                expected_summary = (
                    f"Disc materials: {cases + 1} cases; 14 malformed fixtures rejected; "
                    "full owned SMC1 and parser/configure/hook match; source lifetime checked"
                )
                if result.strip() != expected_summary:
                    raise RuntimeError("Missing material comparison summary")
                results[f"{context}-{repeat}"] = result.strip()
                hashes.append(digest(log))
            if len(set(hashes)) != 1:
                raise RuntimeError("Fresh process traces differ")
            traces[str(context)] = hashes

        def mutant_check(mutant: tuple[str, str, str]) -> str:
            name, old, new = mutant
            scratch = out / "mutants" / name / "native_assets"
            scratch.mkdir(parents=True, exist_ok=True)
            (scratch.parent / "menus").mkdir(exist_ok=True)
            # Copy include closure so mutations never resolve back to shared sources.
            for path in here.glob("*.h"):
                shutil.copy2(path, scratch / path.name)
            for filename in SOURCES:
                shutil.copy2(here / filename, scratch / filename)
            target = scratch / "disc_slot_materials.c"
            text = target.read_text()
            if text.count(old) != 1:
                raise RuntimeError(f"Mutation does not uniquely match: {name}")
            target.write_text(text.replace(old, new))
            run(
                ["bash", str(scratch / "build_windows_disc_slot_materials_check.sh"), str(scratch)],
                scratch / "build.log",
            )
            process = subprocess.run(
                [
                    str(scratch / "disc-slot-materials-check.exe"),
                    windows(iso),
                    windows(dol),
                    windows(reference),
                    windows(rams[14]),
                    "10",
                ],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                check=False,
            )
            (scratch / "check.log").write_text(process.stdout)
            if process.returncode != 4 or "comparison failed at line" not in process.stdout:
                print(process.stdout, flush=True)
                raise RuntimeError(f"Mutant {name} not caught by comparison: {process.returncode}")
            return name

        caught = []
        if not fast:
            with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, (os.cpu_count() or 2) // 2)) as pool:
                caught = list(pool.map(mutant_check, MUTANTS))
        if before != baseline_hashes() or sources != source_hashes(here.parent, entries):
            raise RuntimeError("Protected originals or sources changed")
        if any(digest(Path(path)) != sha for path, sha in inputs.items()):
            raise RuntimeError("Private inputs changed")
        record.update(
            status="passed",
            baseline=before,
            source_sha256=sources,
            input_sha256=inputs,
            results=results,
            check_log_sha256=traces,
            executable_sha256=digest(exe),
            mutants_caught=caught,
            mutants_total=0 if fast else len(MUTANTS),
            cases_per_process=cases + 1,
            fresh_processes=len(contexts) * repeats,
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print(
        f"Disc materials: contexts {', '.join(map(str, contexts))} x {repeats} runs; "
        f"{cases + 1} cases/process; {len(caught)}/{0 if fast else len(MUTANTS)} mutants caught."
    )
    print("Private report " + str(report))


if __name__ == "__main__":
    main()
