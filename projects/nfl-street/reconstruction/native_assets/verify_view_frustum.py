#!/usr/bin/env python3
"""Original-DLL integration gate for the GNNE69 view-frustum owner."""

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
OUT = PRIVATE / "research-temp/nflstreet-view-frustum-20261018"
CACHE = Path("/home/dan/daywater/cache/binaries/46e3de854998a0f66c9be8c72a1164d22cd29eb4")
ABI = PRIVATE / "tools/ModernGekko-Template/lib/ModernGekko/include/moderngekko"
FILES = (
    "view_frustum.c",
    "view_frustum.h",
    "view_frustum_original_check.c",
    "build_windows_view_frustum_original_check.sh",
    "verify_view_frustum.py",
    "view_frustum.md",
    "view_nodes.c",
    "view_nodes.h",
    "list_nodes.c",
    "list_nodes.h",
    "object_pool.c",
    "object_pool.h",
    "scene_owner.c",
    "scene_owner.h",
    "camera_projection.c",
    "camera_projection.h",
    "camera_slots.h",
    "matrix_stack.h",
    "draw_engine.c",
    "draw_engine.h",
    "player_draw.h",
    "marker_ram.h",
)
BUILD = FILES[3]
EXE = "view-frustum-original-check.exe"
ADDRESSES = (
    "801805e0",
    "80180328",
    "80180340",
    "80180388",
    "80181184",
    "80181040",
    "801811f0",
    "80181128",
    "801810ec",
    "80180700",
    "801812d4",
    "801810a4",
    "80180784",
    "80180c6c",
    "80180b94",
    "80180b3c",
    "80180b5c",
    "80180b7c",
    "80180d64",
    "8017bc54",
    "801af57c",
    "801defd8",
    "801df14c",
    "801df39c",
    "801df8b4",
    "801879b4",
    "801af630",
    "801df60c",
    "801df6b4",
    "801df5f8",
    "80188c80",
    "80216c70",
    "80216fa0",
    "802171c4",
    "801df8c4",
    "801df938",
    "801fff64",
)
SUMMARY = re.compile(
    r"Original view frustum: (\d+) cases \(1 saved \+ (\d+) edited\); "
    r"fresh/set/scale/bind (\d+)/(\d+)/(\d+)/(\d+); "
    r"tangent/reduced/negative/project/default/nan (\d+)/(\d+)/(\d+)/(\d+)/(\d+)/(\d+); 8 host guards; "
    r"float returns, ordered services and all RAM outside phase stacks match; digest ([0-9a-f]{8})\."
)
MUTANTS = (
    ("angle_conversion", "angle^0x80000000u", "(angle+1u)^0x80000000u"),
    ("reduction_tail", "tail=sub(sub(z,v),lo)", "tail=0"),
    ("kernel_coefficient", "C(-0x4538),s,r", "C(-0x4534),s,r"),
    ("reciprocal_sign", "return F(reciprocal,correction,high);", "return F(reciprocal,correction,-high);"),
    ("half_angle", "half=angle/2", "half=angle"),
    ("flags", "&0xfffffffbu)|2u", "&0xffffffffu)|2u"),
    ("aspect", "mul(top,aspect)", "mul(top,1.0f)"),
    ("bottom", "bottom=-top", "bottom=top"),
    ("scale_ratio", "load(p+0xe0),load(p+0xe8)", "load(p+0xe8),load(p+0xe0)"),
    ("binding_flag", "p[0x45]=flag", "p[0x44]=flag"),
    (
        "invalid_product",
        "float result=a*b;return result!=result&&a==a&&b==b",
        "float result=a*b;return result!=result&&a!=a&&b==b",
    ),
)
BOUNDARY = (
    "Original frustum801DF60C, scale801DF6B4, binding801DF5F8, tangent80188C80/80216C70/kernel80216FA0 "
    "and first reduction802171C4 execute unhooked; larger argument reduction is unsupported host scope. "
    "Fresh list/view setup and existing projection builders also execute original unhooked. "
    "Only heap/error/geterror services early-return. Actual tangent binary32 returns, ordered services "
    "and all24MiB compare outside per-phase stack unions. Setup[80F46F90,80F47008), "
    "direct tangent[80F46F98,80F47008), frustum[80F46F70,80F47008), scale[80F46FA8,80F47008), "
    "perspective[80F46FF8,80F47008); binding/screen-mapping leaves have no exclusion. "
    "Only compared stack synchronized before narrower phases. Projection join compares both slots "
    "after original screen and perspective builders. Constants pinned against DOL and all captures. "
    "Native shared modules unchanged. No bootstrap, field/HUD type constructors, stack/scene publication "
    "or live/fresh-match claim."
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
        "phase_stack_exclusions": {
            "setup": ["80f46f90", "80f47008"],
            "tangent": ["80f46f98", "80f47008"],
            "frustum": ["80f46f70", "80f47008"],
            "scale": ["80f46fa8", "80f47008"],
            "perspective": ["80f46ff8", "80f47008"],
            "binding_and_screen": [],
        },
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
        raw: tuple[tuple[int, int, str], ...] = (
            (0x801DEF40, 0x1DBF40, "80030000806400007c6300504e800020"),
            (0x801DEE3C, 0x1DBE3C, "80030000806400007c6300504e800020"),
            (0x80263CC8, 0x260CC8, "0000000000000000433000008000000043300000000000003f0000003f800000"),
        )
        for guest, offset, expected in raw:
            with dol.open("rb") as stream:
                stream.seek(offset)
                if stream.read(len(expected) // 2).hex() != expected:
                    raise RuntimeError(f"Original DOL evidence changed: {guest:08x}")
        import struct

        dol_bytes = dol.read_bytes()
        constant_slices = []
        for guest, length in (
            (0x803A3B84, 4),
            (0x803A3BD8, 0x34),
            (0x803A5508, 52),
            (0x802617B8, 12),
            (0x80263CE8, 4),
            (0x802442C0, 20),
        ):
            for index in range(18):
                offset = struct.unpack_from(">I", dol_bytes, 4 * index)[0]
                address = struct.unpack_from(">I", dol_bytes, 0x48 + 4 * index)[0]
                size = struct.unpack_from(">I", dol_bytes, 0x90 + 4 * index)[0]
                if address <= guest and guest + length <= address + size:
                    file_offset = offset + guest - address
                    constant_slices.append((guest, file_offset, dol_bytes[file_offset : file_offset + length].hex()))
                    break
            else:
                raise RuntimeError(f"No DOL constant mapping: {guest:08x}")
        raw = (*raw, *constant_slices)
        table_hex = "80180c2080180c3080180d6480180c6c80180d8c80180e0c80180e6c80180f1880180fac000000200000001001000000"
        for context in contexts:
            with (PRIVATE / f"research-temp/position-neutral-{context}.mem1").open("rb") as stream:
                for guest, _, expected in (*raw, (0x80180C20, 0x17DC20, init_hex), (0x80389E88, 0, table_hex)):
                    stream.seek(guest - 0x80000000)
                    if stream.read(len(expected) // 2).hex() != expected:
                        raise RuntimeError(f"Capture evidence changed: {guest:08x}")
        inputs = fingerprints([dol, *(PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in contexts)])
        record["raw_source_evidence"] = {
            "leaves_and_constants": [{"guest": f"{g:08x}", "dol_offset": f"{o:x}", "bytes": h} for g, o, h in raw],
            "init": {"guest": "80180c20", "dol_offset": "17dc20", "bytes": init_hex},
            "type_table": {
                "guest": "80389e88",
                "bytes": table_hex,
                "provenance": "Matching captures; table producer not reconstructed",
            },
        }
        record.update(baselines=protected, source_sha256=before, input_sha256=inputs)
        build(HERE, out)
        executable_hash = digest(out / EXE)
        runs: list[dict[str, Any]] = []
        record["runs"] = runs
        jobs = [(context, repeat) for context in contexts for repeat in range(repeats)]

        def run_original(job: tuple[int, int]) -> dict[str, Any]:
            context, repeat = job
            log = out / f"context{context}-{repeat}.log"
            code, output = execute(command(out / EXE, context, edited), log)
            match = SUMMARY.fullmatch(output)
            if code or not match or tuple(map(int, match.groups()[:2])) != (edited + 1, edited):
                raise RuntimeError(f"Original comparison failed ({code}): {log}; {output}")
            if not all(int(value) for value in match.groups()[2:12]):
                raise RuntimeError(f"Missing branch coverage: {log}")
            return {"context": context, "repeat": repeat, "summary": output, "log_sha256": digest(log)}

        # Up to three independent processes, each with its own input RAM/log.
        # map yields stable report order; repeats still run in fresh processes.
        with ThreadPoolExecutor(max_workers=min(3, len(jobs))) as pool:
            for run in pool.map(run_original, jobs):
                runs.append(run)
                print(run["summary"], flush=True)
        for context in contexts:
            if len({run["summary"] for run in runs if run["context"] == context}) != 1:
                raise RuntimeError(f"Fresh processes differ in context {context}")
        mutants = []
        if not fast:
            workers = min(len(MUTANTS), max(1, (os.cpu_count() or 2) // 2))
            with ThreadPoolExecutor(max_workers=min(3, workers)) as pool:
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
        f"View frustum: passed; mode={'fast' if fast else 'full'}; contexts={len(contexts)}; "
        f"processes={len(contexts) * repeats}; cases/process={edited + 1}; "
        f"mutants={len(mutants)}/{len(mutants)}; private report {report}"
    )


if __name__ == "__main__":
    main()
