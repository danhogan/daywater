#!/usr/bin/env python3
"""Host retained-match aggregate composition using disc-backed catalogs."""

import concurrent.futures
import json
import os
import re
import shutil
import struct
import subprocess
import uuid
from pathlib import Path

from verify_controller_checks import PRIVATE, baseline_hashes, digest, private, windows
from verify_player_startup import source_hashes

OWN = (
    "match_disc_inputs.c",
    "match_disc_inputs.h",
    "match_disc_inputs_check.c",
    "build_windows_match_disc_inputs_check.sh",
    "verify_match_disc_inputs.py",
    "match_disc_inputs.md",
)
BOUNDARY = (
    "Host composition of existing retained-menu visual/skeleton constructors with owned pinned-disc catalogs. "
    "Reference is unchanged constructors fed independent exported catalogs. Three full14player aggregates "
    "per process, each84bound resources, bodyvariants0/1/2;201cheap varied GEAR record consumer cases; "
    "350perplayer catalog/name lifetime checks; occupiedoutput rejection and3latefailure rollbacks. "
    "Originalcallbacks deferred until bothstages succeed. Realcallbackfailure may retain earlier external "
    "sideeffects. Session/resolver/bodypool/headwear/otherarchives remain borrowed prerequisites. "
    "No originalboot, allRAMexecution, emulator, actorstartup, livegameplay or rendering claim."
)
MUTANTS = [
    {"name": name, "old": old, "new": new}
    for name, old, new in (
        ("gear-input", "local[p].visual.surface.gear=&s->gear.catalog;", "local[p].visual.surface.gear=NULL;"),
        (
            "material-input",
            "local[p].visual.surface.materials=&s->materials.catalog;",
            "local[p].visual.surface.materials=NULL;",
        ),
        ("derived-records", "local[p].visual.gear=&s->records;", "local[p].visual.gear=NULL;"),
        ("constructor-side", "local[p]=inputs[p];", "local[p]=inputs[p];local[p].record_side^=1u;"),
        ("skeleton-stage", "&catalog,binding,&s->skeletons)", "&catalog,NULL,&s->skeletons)"),
        (
            "early-real-flush",
            "q->items[q->count++]=(notification){c->call,c->opaque,token,size};return 1;",
            "q->items[q->count++]=(notification){c->call,c->opaque,token,size};return c->call(c->opaque,token,size);",
        ),
        ("wrong-player-opaque", "inputs[p].visual.surface.flush_opaque};", "inputs[0].visual.surface.flush_opaque};"),
        ("occupied-output", "||!out||*out)return 0;", "||!out)return 0;"),
    )
]


def run(command: list[str], log: Path) -> str:
    p = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, check=False)
    log.write_text(p.stdout)
    if p.returncode:
        print(p.stdout, flush=True)
        raise subprocess.CalledProcessError(p.returncode, command, output=p.stdout)
    return p.stdout


def inventory(here: Path) -> tuple[str, ...]:
    """Hash and copy every build source and recursively quoted local include."""
    pending = [here / name for name in OWN]
    build = (here / "build_windows_match_disc_inputs_check.sh").read_text()
    pending += [here / name for name in re.findall(r'"\$root/([^"\n]+\.c)"', build)]
    found: set[Path] = set()
    while pending:
        path = pending.pop().resolve()
        if path in found:
            continue
        path.relative_to(here.parent)
        found.add(path)
        if path.suffix in (".c", ".h"):
            for name in re.findall(r'^\s*#include\s+"([^"]+)"', path.read_text(), re.MULTILINE):
                pending.append(path.parent / name)
    return tuple(sorted(p.relative_to(here.parent).as_posix() for p in found))


def main() -> None:
    here = Path(__file__).resolve().parent
    fast = os.environ.get("NFLSTREET_GATE_FAST") == "1"
    contexts, repeats = ((15,), 1) if fast else ((14, 15, 16), 2)
    variants, cheap = (1, 11) if fast else (3, 201)
    base = private(PRIVATE / "research-temp/nflstreet-match-disc-inputs-20261017")
    out = base / ("run-" + uuid.uuid4().hex[:12])
    out.mkdir(parents=True)
    report = base / "verification.json"
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY, "fast_mode": fast, "run_dir": str(out)}
    report.write_text(json.dumps(record, indent=2) + "\n")
    try:
        before = baseline_hashes()
        entries = inventory(here)
        sources = source_hashes(here.parent, entries)
        data = private(PRIVATE / "private/nflstreet/GNNE69-r0")
        assets = [
            data / "assets" / f"{n}.dat"
            for n in ("plaface", "plarosterhair", "platex", "plamodel", "plahair", "animdata", "pladyncl")
        ]
        stat, schema = data / "unpacked/template/00002.bin", data / "unpacked/template/00001.bin"
        catalogs = [
            data / "native" / name for name in ("skeleton-startup.nsc1", "gear-texture.gtc4", "slot-materials.smc1")
        ]
        selection = private(
            PRIVATE / "research-temp/nflstreet-menu-match-owner-20261013/field-picker-window-selection.json"
        )
        selected = json.loads(selection.read_text())
        if selected["schema_version"] != 1:
            raise RuntimeError("Unsupported retained menu selection")
        words = [
            *selected["team_ids"],
            *(v for a in selected["selected_players"] for v in a),
            *(v for a in selected["formation_players"] for v in a),
            selected["field_slot"],
            selected["field_key"],
        ]
        if len(words) != 46 or any(type(v) is not int or not 0 <= v <= 0xFFFFFFFF for v in words):
            raise RuntimeError("Invalid retained menu words")
        capture = out / "retained-menu.qms1"
        capture.write_bytes(b"QMS1" + struct.pack(">47I", 1, *words))
        iso = private(PRIVATE / "research-temp/nflstreet-GNNE69.iso")
        if digest(iso, "sha1") != "813c4511ba3135059a13de424555647e8252254b":
            raise RuntimeError("Wrong ISO revision")
        contexts_paths = {c: private(PRIVATE / f"research-temp/position-neutral-{c}.mem1") for c in contexts}
        nod = private(PRIVATE / "research-temp/nflstreet-live-20261013/build/_deps/nod_prebuilt-src")
        provider = [nod / "include/nod.h", nod / "lib/nod.lib", nod / "bin/nod.dll"]
        files = [*assets, stat, schema, *catalogs, selection, capture, iso, *contexts_paths.values(), *provider]
        inputs = {str(p): digest(p) for p in files}
        args = [*assets[:3], data / "unpacked", *assets[3:6], stat, *catalogs, assets[6], schema, capture, iso]
        stages = {}
        for directory in (
            "nflstreet-match-skeletons-20261013",
            "nflstreet-skeleton-disc-20261017",
            "nflstreet-disc-materials-20261017",
            "nflstreet-disc-gear-20261017/run-56f2107c2888",
        ):
            stage_path = private(PRIVATE / "research-temp" / directory / "verification.json")
            stage = json.loads(stage_path.read_text())
            if stage["status"] != "passed" or stage["baseline"] != before:
                raise RuntimeError("Required stage missing baseline-matched pass: " + directory)
            if "match-skeletons" not in directory:
                if (
                    stage.get("fast", False)
                    or stage.get("fast_mode", False)
                    or stage.get("fresh_processes") != 6
                    or stage.get("mutants_total") != 7
                    or len(stage.get("mutants_caught", [])) != 7
                ):
                    raise RuntimeError("Catalog provenance requires a full six-process mutant pass: " + directory)
            for name, sha in stage["source_sha256"].items():
                if digest(here.parent / name) != sha:
                    raise RuntimeError("Stale stage source: " + name)
            stages[str(stage_path)] = digest(stage_path)

        def build(root: Path, destination: Path) -> None:
            run(
                [
                    "env",
                    f"DAYWATER_NOD={nod}",
                    "bash",
                    str(root / "build_windows_match_disc_inputs_check.sh"),
                    str(destination),
                ],
                destination / "build.log",
            )

        def command(exe: Path, context: int, aggregates: int, records: int) -> list[str]:
            return [
                str(exe),
                *(windows(p) for p in args),
                windows(contexts_paths[context]),
                str(context),
                str(aggregates),
                str(records),
            ]

        build(here, out)
        exe = out / "match-disc-inputs-check.exe"
        logs, results = {}, {}
        for context in contexts:
            traces = []
            for repeat in range(repeats):
                log = out / f"match-{context}-{repeat}.log"
                text = run(command(exe, context, variants, cheap), log).strip()
                expected = (
                    f"Match disc inputs: {variants} full aggregates, {variants * 14} players, "
                    f"{variants * 84} bound resources; "
                    f"350 catalog queries, {cheap} varied gear records, 1 occupied-output guard, "
                    "3 late rollbacks, 1 namespace guard; digest "
                )
                if not text.startswith(expected):
                    raise RuntimeError("Incomplete aggregate check: " + text)
                traces.append(digest(log))
                results[f"{context}-{repeat}"] = text
            if not fast and traces[0] != traces[1]:
                raise RuntimeError("Fresh aggregate traces differ")
            logs[str(context)] = traces

        def mutant_check(m: dict[str, str]) -> tuple[str, dict[str, object]]:
            scratch = out / "mutants" / m["name"]
            root = scratch / "reconstruction"
            for entry in entries:
                target = root / entry
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(here.parent / entry, target)
            source = root / "native_assets/match_disc_inputs.c"
            text = source.read_text()
            if text.count(m["old"]) != 1:
                raise RuntimeError("Mutation anchor mismatch: " + m["name"])
            source.write_text(text.replace(m["old"], m["new"]))
            build(root / "native_assets", scratch)
            mutant_exe = scratch / exe.name
            p = subprocess.run(
                command(mutant_exe, 14, 1, 11), stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, check=False
            )
            log = scratch / "check.log"
            log.write_text(p.stdout)
            if p.returncode != 4 or "comparison failed at line" not in p.stdout:
                print(p.stdout, flush=True)
                raise RuntimeError(f"Mutant {m['name']} not caught: {p.returncode}")
            return m["name"], {
                "exit_code": p.returncode,
                "diagnostic": p.stdout.strip(),
                "source_sha256": digest(source),
                "executable_sha256": digest(mutant_exe),
                "log_sha256": digest(log),
            }

        mutant_results = {}
        if not fast:
            with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, min(2, (os.cpu_count() or 2) // 2))) as pool:
                mutant_results = dict(pool.map(mutant_check, MUTANTS))
        if sources != source_hashes(here.parent, entries) or before != baseline_hashes():
            raise RuntimeError("Source/baseline changed during gate")
        if any(digest(Path(p)) != sha for p, sha in {**inputs, **stages}.items()):
            raise RuntimeError("Input/stage changed during gate")
        record.update(
            status="passed",
            baseline=before,
            source_sha256=sources,
            input_sha256=inputs,
            stage_report_sha256=stages,
            results=results,
            check_log_sha256=logs,
            executable_sha256=digest(exe),
            fresh_processes=len(contexts) * repeats,
            full_aggregates_per_process=variants,
            cheap_varied_records_per_process=cheap,
            bound_resources_per_process=variants * 84,
            mutants_caught=list(mutant_results),
            mutants_total=0 if fast else len(MUTANTS),
            mutant_results=mutant_results,
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        text = json.dumps(record, indent=2) + "\n"
        report.write_text(text)
        (out / "verification.json").write_text(text)
    print(
        f"Match disc inputs: contexts {', '.join(map(str, contexts))} x {repeats} runs; "
        f"{len(mutant_results)}/{0 if fast else len(MUTANTS)} mutants caught."
    )
    print("Private report " + str(report))


if __name__ == "__main__":
    main()
