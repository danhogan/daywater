#!/usr/bin/env python3
"""Windows integration gate: nod/TERF bytes versus every extracted game payload, plus planted mutants."""

import json
import shutil
import subprocess
from pathlib import Path

from verify_controller_checks import PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

SOURCES = (
    "disc_files.c",
    "disc_files.h",
    "disc_files_check.c",
    "disc_files_mutants.json",
    "archive.c",
    "archive.h",
    "lzh1.c",
    "lzh1.h",
    "build_windows_disc_files_check.sh",
    "verify_disc_files.py",
    "disc_files.md",
)
ISO_SHA1 = "813c4511ba3135059a13de424555647e8252254b"
BOUNDARY = (
    "Host nod FST reads and TERF DIR1/COMP entry decoding versus independent extracted bytes. "
    "Two fresh Windows processes per saved context 14/15/16. RAM is read-only context input, not booted; "
    "no original DVD timing, original-module RAM comparison, scene rendering or live-window claim. "
    "Extraction logs/report are generated host metadata, have no disc paths, and are inventoried and hashed separately."
)


def main() -> None:
    here = Path(__file__).resolve().parent
    out = private(PRIVATE / "research-temp/nflstreet-disc-20261015")
    out.mkdir(parents=True, exist_ok=True)
    report = out / "verification.json"
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY}
    report.write_text(json.dumps(record, indent=2) + "\n")
    try:
        before = baseline_hashes()
        entries = tuple("native_assets/" + name for name in SOURCES)
        sources = source_hashes(here.parent, entries)
        live = here.parent.parent / "native/live"
        live_sources = {
            str(live / name): digest(live / name) for name in ("CMakeLists.txt", "live_main.c", "README.md")
        }
        iso = private(PRIVATE / "research-temp/nflstreet-GNNE69.iso")
        if digest(iso, "sha1") != ISO_SHA1:
            raise RuntimeError("Disc image differs from preserved GNNE69 rev0 ISO")
        tree = private(PRIVATE / "private/nflstreet/GNNE69-r0/unpacked")
        files = sorted(path for path in tree.rglob("*") if path.is_file())
        payloads = []
        metadata = []
        for path in files:
            private(path)
            relative = path.relative_to(tree)
            if len(relative.parts) == 2 and path.suffix == ".bin" and len(path.stem) == 5 and path.stem.isdecimal():
                payloads.append((path, relative.as_posix()))
            elif path.name == "unpack.log" or relative.as_posix() == "unpack-report.json":
                metadata.append(str(relative))
            else:
                raise RuntimeError(f"Unclassified extracted-tree file: {relative}")
        if not payloads:
            raise RuntimeError("No extracted payloads")
        # Also compare whole DAT files, independently of our virtual-entry path.
        archives = sorted((tree.parent / "assets").glob("*.dat"))
        payloads.extend((private(path), path.name) for path in archives)
        contexts = {i: private(PRIVATE / f"research-temp/position-neutral-{i}.mem1") for i in (14, 15, 16)}
        nod = private(PRIVATE / "research-temp/nflstreet-live-20261013/build/_deps/nod_prebuilt-src")
        # The build supports DAYWATER_NOD overrides for manual compilation. This
        # gate pins the tested provider and records its header/import/DLL bytes.
        provider = [nod / "include/nod.h", nod / "lib/nod.lib", nod / "bin/nod.dll"]
        inputs = {str(path): digest(path) for path in [iso, *files, *archives, *contexts.values(), *provider]}
        manifest = out / "paths.tsv"
        manifest.write_text("".join(f"{windows(path)}\t{game}\n" for path, game in payloads))
        mapping = {
            path.relative_to(tree).as_posix(): {
                "virtual_path": game,
                "disc_archive": path.parent.name + ".dat",
                "entry": int(path.stem),
            }
            for path, game in payloads
            if path.is_relative_to(tree)
        }
        record.update(
            extracted_tree_files=len(files),
            payload_files=len(payloads) - len(archives),
            whole_archives=len(archives),
            generated_metadata=metadata,
            path_mapping="<archive>/NNNNN.bin -> <archive>.dat, decoded zero-based DIR1 entry NNNNN",
            paths=mapping,
        )
        build = ["env", f"DAYWATER_NOD={nod}", "bash"]
        run([*build, str(here / "build_windows_disc_files_check.sh"), str(out), "--live"], out / "build.log")
        exe = out / "disc-files-check.exe"
        logs = {}
        results = {}
        expected_bytes = sum(path.stat().st_size for path, _ in payloads)
        for context, ram in contexts.items():
            traces = []
            for repeat in range(2):
                log = out / f"disc-{context}-{repeat}.log"
                text = run([str(exe), windows(iso), windows(manifest), windows(ram), str(context)], log)
                result = json.loads(text.strip())
                if (
                    result["files"] != len(payloads)
                    or result["bytes"] != expected_bytes
                    or result["animation_entries"] != sum(game.startswith("animdata/") for _, game in payloads)
                    or result["path_variants"] != [134, 134, 133]
                    or result["invalid_paths"] != 10
                ):
                    raise RuntimeError(f"Incomplete byte comparison in context {context}")
                traces.append(digest(log))
                results[str(context)] = result
            if traces[0] != traces[1]:
                raise RuntimeError("Fresh disc traces differ")
            logs[str(context)] = traces
        mutants = json.loads((here / "disc_files_mutants.json").read_text())
        caught = []
        for mutant in mutants:
            scratch = out / "mutants" / mutant["name"]
            scratch.mkdir(parents=True, exist_ok=True)
            for name in SOURCES:
                shutil.copy2(here / name, scratch / name)
            target = scratch / mutant["file"]
            text = target.read_text()
            if text.count(mutant["old"]) != 1 or "\n" in mutant["old"] or "\n" in mutant["new"]:
                raise RuntimeError(f"Mutation must match exactly one source line: {mutant['name']}")
            target.write_text(text.replace(mutant["old"], mutant["new"]))
            run([*build, str(scratch / "build_windows_disc_files_check.sh"), str(scratch)], scratch / "build.log")
            # A build failure never counts as a detected mutant. Require the
            # checker's explicit comparison-failure status (not a crash).
            with (scratch / "check.log").open("w") as output:
                process = subprocess.run(
                    [str(scratch / exe.name), windows(iso), windows(manifest), windows(contexts[14]), "14"],
                    stdout=output,
                    stderr=subprocess.STDOUT,
                    check=False,
                )
            if process.returncode != 4:
                raise RuntimeError(f"Mutant {mutant['name']} was not caught by comparison: {process.returncode}")
            caught.append(mutant["name"])
        if before != baseline_hashes() or any(digest(Path(path)) != sha for path, sha in inputs.items()):
            raise RuntimeError("Protected originals or private inputs changed")
        if files != sorted(path for path in tree.rglob("*") if path.is_file()):
            raise RuntimeError("Extracted-tree inventory changed")
        if sources != source_hashes(here.parent, entries) or any(digest(Path(p)) != h for p, h in live_sources.items()):
            raise RuntimeError("Sources changed during verification")
        record.update(
            status="passed",
            baseline=before,
            input_sha256=inputs,
            source_sha256=sources,
            live_source_sha256=live_sources,
            original_log_sha256=logs,
            results=results,
            executable_sha256=digest(exe),
            manifest_sha256=digest(manifest),
            mutants_caught=caught,
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print(f"Disc files: {len(payloads)} files / {expected_bytes} bytes identical; contexts 14/15/16 x 2 fresh runs.")
    print(f"Disc mutants: {len(caught)}/{len(mutants)} caught; generated metadata: {len(metadata)} files inventoried.")
    print("Private report " + str(report))


if __name__ == "__main__":
    main()
