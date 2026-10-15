#!/usr/bin/env python3
"""Check the native HUD producer slice and every mutant; full UIS execution remains pending."""

import argparse
import json
import re
import shutil
import subprocess
from pathlib import Path

from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

SOURCES = (
    "hud_values.c",
    "hud_values.h",
    "hud_values_original_check.c",
    "hud_values_mutants.json",
    "hud_values.md",
    "gekko_float.h",
    "build_windows_hud_values_original_check.sh",
    "verify_hud_values.py",
)
BOUNDARY = (
    "Producer slice: unhooked 8005D96C with neutral controlled input, 80119590 to the actual saved UIS turbo "
    "array, 8005D418, 80059920 and 801D15B0. Every MEM1 byte outside the reserved oracle stack compared "
    "after every update and after getters, including unchanged bytes. Full UIS 801CF610/script execution, "
    "score/down/distance/clock publication, turbo spending and GameBreaker events remain unmodeled."
)
SUMMARY = re.compile(
    r"Original HUD value producers: 401 cases x 8 updates; 6416 all-RAM comparisons; 12832 turbo and 6416 "
    r"GameBreaker fractions; turbo UIS payload and timer cleanup match; no hooks; digest [0-9a-f]{8}\."
)


def build(here: Path, out: Path) -> Path:
    out.mkdir(parents=True, exist_ok=True)
    run(["bash", str(here / "build_windows_hud_values_original_check.sh"), str(out)], out / "build.log")
    return out / "hud-values-original-check.exe"


def mutants(here: Path, out: Path, sources: dict[str, str], context: Path, build_only: bool) -> list[dict]:
    edits = json.loads((here / "hud_values_mutants.json").read_text())
    results = []
    names = set()
    for edit in edits:
        if set(edit) != {"name", "file", "old", "new"} or "\n" in edit["old"] or "\n" in edit["new"]:
            raise RuntimeError("Expected a one-line {name,file,old,new} mutant")
        name = edit["name"]
        if name in names or not re.fullmatch(r"[a-z0-9-]+", name):
            raise RuntimeError("Invalid or duplicate mutant name")
        names.add(name)
        destination = private(out / "mutants" / name)
        tree = destination / "source"
        if tree.exists():
            shutil.rmtree(tree)
        for source in sources:
            if source.startswith("ModernGekko/"):
                continue
            target = tree / source
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(here.parent / source, target)
        target = (tree / "native_assets" / edit["file"]).resolve()
        if not target.is_relative_to(tree.resolve()) or not target.is_file():
            raise RuntimeError("Mutant must edit a declared local source")
        original = target.read_text()
        if original.count(edit["old"]) != 1 or edit["old"] == edit["new"]:
            raise RuntimeError(f"Nonunique or ineffective mutant {name}")
        target.write_text(original.replace(edit["old"], edit["new"]))
        exe = build(tree / "native_assets", destination)
        if build_only:
            results.append({"name": name, "status": "compiled_only", "executable_sha256": digest(exe)})
            continue
        log = destination / "original-14.log"
        with log.open("w") as stream:
            result = subprocess.run(
                [str(exe), windows(context), windows(BASELINES["original_dll"][0])],
                stdout=stream,
                stderr=subprocess.STDOUT,
                check=False,
                timeout=300,
            )
        text = log.read_text()
        if result.returncode != 6 or not ("RAM differs case " in text or "Fraction differs case " in text):
            raise RuntimeError(f"Mutant {name} was not caught by a value/RAM comparison: {text}")
        results.append({"name": name, "caught": True, "log_sha256": digest(log)})
        print(f"mutant {name}: caught by original comparison", flush=True)
    if len(results) != 10:
        raise RuntimeError("Expected all 10 declared mutants")
    return results


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--build-only", action="store_true", help="Compile all checks and mutants; execute no Windows code"
    )
    parser.add_argument("--probe", action="store_true", help="Also trace the unhooked original score/down/UIS tick")
    args = parser.parse_args()
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / "research-temp/nflstreet-hudvals-20261015")
    out.mkdir(parents=True, exist_ok=True)
    report = out / "verification.json"
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY, "full_hud_values_complete": False}
    report.write_text(json.dumps(record, indent=2) + "\n")
    entries = tuple("native_assets/" + name for name in SOURCES)
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        contexts = {i: PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in (14, 15, 16)}
        inputs = {str(path): digest(path) for path in contexts.values()}
        exe = build(here, out)
        logs = {}
        if not args.build_only:
            for i, context in contexts.items():
                traces = []
                for repeat in range(2):
                    log = out / f"original-{i}-{repeat}.log"
                    text = run([str(exe), windows(context), windows(BASELINES["original_dll"][0])], log)
                    if not SUMMARY.fullmatch(text.replace("\r\n", "\n").strip()):
                        raise RuntimeError(f"Original HUD producer comparison failed in context {i}: {text}")
                    traces.append(digest(log))
                    print(f"context {i}, fresh run {repeat + 1}: {text.strip()}", flush=True)
                if traces[0] != traces[1]:
                    raise RuntimeError("Fresh original HUD producer traces differ")
                logs[str(i)] = traces
        record["mutants"] = mutants(here, out, sources, contexts[14], args.build_only)
        if args.probe and not args.build_only:
            for i, context in contexts.items():
                log = out / f"publication-probe-{i}.log"
                run([str(exe), windows(context), windows(BASELINES["original_dll"][0]), "probe"], log)
                record[f"publication_probe_{i}_sha256"] = digest(log)
        if before != baseline_hashes() or any(digest(Path(path)) != sha for path, sha in inputs.items()):
            raise RuntimeError("Protected originals or private inputs changed")
        if sources != source_hashes(root, entries):
            raise RuntimeError("Source changed during verification")
        record.update(
            status="compiled_only" if args.build_only else "passed_producer_slice",
            baseline=before,
            input_sha256=inputs,
            source_sha256=sources,
            original_log_sha256=logs,
            executable_sha256=digest(exe),
            cases_per_context=401,
            consecutive_updates=8,
            comparisons_per_context_per_run=6416,
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    if args.build_only:
        print("HUD producer slice and 10 mutants compiled; original execution and mutation results pending.")
    else:
        print("HUD producer slice passes; mutants: 10/10 caught. Full UIS/score/down publication remains pending.")
    print("Private report " + str(report))


if __name__ == "__main__":
    main()
