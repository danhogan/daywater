#!/usr/bin/env python3
"""Host disc/DOL-to-GTC4 byte preparation and existing native consumer composition."""

import concurrent.futures
import json
import os
import shutil
import subprocess
import uuid
from pathlib import Path

from export_gear_texture_catalog import export
from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, windows
from verify_player_startup import source_hashes

OWN = (
    "disc_gear_catalog.c",
    "disc_gear_catalog.h",
    "disc_gear_catalog_check.c",
    "build_windows_disc_gear_catalog_check.sh",
    "verify_disc_gear_catalog.py",
    "disc_gear_catalog.md",
)
SHARED = (
    "disc_dol.c",
    "disc_dol.h",
    "dol_sections.c",
    "dol_sections.h",
    "gear_texture.c",
    "gear_texture.h",
    "headwear_state.c",
    "headwear_state.h",
    "archive.c",
    "archive.h",
    "lzh1.c",
    "lzh1.h",
    "export_gear_texture_catalog.py",
    "verify_player_startup.py",
)
BOUNDARY = (
    "Host pinned disc/DOL composition, exact GTC4 bytes against independent Python DOL export, "
    "existing catalog views/template lookup and GEAR record consumer comparison, owned lifetimes. "
    "Contexts only seed native consumer case variation; no original execution/all-RAM equivalence, "
    "emulator boot, fresh actors, natural match startup or live integration claim."
)
MUTANTS = (
    ("revision-bypass", "!nflstreet_dol_verify_revision(dol,size)", "(!dol||!size)"),
    ("category-offset", "COPY(0x802859f8u,48u);", "COPY(0x802859fcu,48u);"),
    ("query-tags-offset", "COPY(0x80285968u,104u);", "COPY(0x8028596cu,104u);"),
    ("template-stride", "0x802799ecu+24u*slot", "0x802799ecu+20u*slot"),
    ("color-names-offset", "COPY(0x8027132cu,360u);", "COPY(0x8027133bu,360u);"),
    ("color-tags-offset", "0x8028575cu+16u*item+8u", "0x8028575cu+16u*item+4u"),
    ("occupied-owner-accept", "if(!empty(out)||", "if(!out||"),
)


def run(command: list[str], log: Path, expect_failure: bool = False) -> str:
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, check=False)
    log.write_text(result.stdout)
    if expect_failure:
        if result.returncode != 4 or "comparison failed at line" not in result.stdout:
            print(result.stdout, flush=True)
            raise RuntimeError(f"Mutant did not produce a comparison failure: {result.returncode}")
    elif result.returncode:
        print(result.stdout, flush=True)
        raise subprocess.CalledProcessError(result.returncode, command, output=result.stdout)
    return result.stdout


def main() -> None:
    here = Path(__file__).resolve().parent
    fast = os.environ.get("NFLSTREET_GATE_FAST") == "1"
    contexts = (15,) if fast else (14, 15, 16)
    repeats, cases = (1, 12) if fast else (2, 400)
    # Never overwrite a previous full or fast report.
    out = private(PRIVATE / "research-temp/nflstreet-disc-gear-20261017" / ("run-" + uuid.uuid4().hex[:12]))
    out.mkdir(parents=True, exist_ok=False)
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY, "fast": fast}
    report = out / "verification.json"
    report.write_text(json.dumps(record, indent=2) + "\n")
    try:
        before = baseline_hashes()
        entries = tuple("native_assets/" + name for name in (*OWN, *SHARED)) + (
            "menus/team_catalog.c",
            "menus/team_catalog.h",
            "menus/verify_controller_checks.py",
        )
        sources = source_hashes(here.parent, entries)
        iso = private(PRIVATE / "research-temp/nflstreet-GNNE69.iso")
        dol = private(BASELINES["dol"][0])
        if digest(iso, "sha1") != "813c4511ba3135059a13de424555647e8252254b":
            raise RuntimeError("Unexpected disc revision")
        if digest(dol, "sha1") != "46e3de854998a0f66c9be8c72a1164d22cd29eb4":
            raise RuntimeError("Unexpected independent DOL revision")
        ref = private(out / "independent-gear.gtc4")
        export_sha = export(ref)
        db = private(PRIVATE / "private/nflstreet/GNNE69-r0/unpacked/template/00002.bin")
        snapshots = {
            context: private(PRIVATE / f"research-temp/position-neutral-{context}.mem1") for context in contexts
        }
        nod = private(PRIVATE / "research-temp/nflstreet-live-20261013/build/_deps/nod_prebuilt-src")
        inputs = {
            str(path): digest(path)
            for path in (
                iso,
                dol,
                ref,
                db,
                *snapshots.values(),
                nod / "include/nod.h",
                nod / "lib/nod.lib",
                nod / "bin/nod.dll",
            )
        }

        def build(root: Path, target: Path) -> None:
            run(
                [
                    "env",
                    f"DAYWATER_GEAR_SHARED={here}",
                    f"DAYWATER_NOD={nod}",
                    "ZIG_GLOBAL_CACHE_DIR=/home/dan/.cache/daywater-zig-cache",
                    "bash",
                    str(root / "build_windows_disc_gear_catalog_check.sh"),
                    str(target),
                ],
                target / "build.log",
            )

        def command(target: Path, context: int, count: int) -> list[str]:
            return [
                str(target / "disc-gear-catalog-check.exe"),
                windows(iso),
                windows(dol),
                windows(ref),
                windows(db),
                windows(snapshots[context]),
                str(count),
            ]

        build(here, out)
        results, logs = {}, {}
        for context in contexts:
            traces = []
            for repeat in range(repeats):
                log = out / f"check-{context}-{repeat}.log"
                text = run(command(out, context, cases), log)
                if f"Disc gear catalog: {cases} consumer/lifecycle cases, 2924 exact bytes" not in text:
                    raise RuntimeError("Missing comparison summary")
                results[f"{context}-{repeat}"] = text.strip()
                traces.append(digest(log))
            if len(set(traces)) != 1:
                raise RuntimeError("Fresh-process traces differ")
            logs[str(context)] = traces

        def mutant_check(mutant: tuple[str, str, str]) -> str:
            name, old, new = mutant
            scratch = out / "mutants" / name
            scratch.mkdir(parents=True)
            for source in OWN:
                shutil.copy2(here / source, scratch / source)
            target = scratch / "disc_gear_catalog.c"
            text = target.read_text()
            if text.count(old) != 1:
                raise RuntimeError(f"Mutation does not match once: {name}")
            target.write_text(text.replace(old, new))
            build(scratch, scratch)
            run(command(scratch, contexts[0], 12), scratch / "check.log", expect_failure=True)
            return name

        caught = []
        mutants = () if fast else MUTANTS
        with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, min(8, (os.cpu_count() or 2) // 2))) as executor:
            caught = list(executor.map(mutant_check, mutants))
        if sources != source_hashes(here.parent, entries) or before != baseline_hashes():
            raise RuntimeError("Source or protected baseline changed")
        if any(digest(Path(path)) != sha for path, sha in inputs.items()):
            raise RuntimeError("Private inputs changed")
        record.update(
            status="passed",
            source_sha256=sources,
            input_sha256=inputs,
            baseline=before,
            independent_catalog_sha256=export_sha,
            results=results,
            check_log_sha256=logs,
            cases_per_process=cases,
            fresh_processes=len(contexts) * repeats,
            executable_sha256=digest(out / "disc-gear-catalog-check.exe"),
            mutants_caught=caught,
            mutants_total=len(mutants),
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print(
        f"Disc gear catalog: contexts {contexts} x {repeats} runs; {cases} cases/process; "
        f"{len(caught)}/{len(mutants)} mutants caught."
    )
    print("Private report " + str(report))


if __name__ == "__main__":
    main()
