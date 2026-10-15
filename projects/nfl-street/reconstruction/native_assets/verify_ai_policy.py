#!/usr/bin/env python3
"""Integration gate: native policy closure, all RAM, fresh processes, and compiled mutants."""

import json
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

# The shared outer gate invokes this file directly without PYTHONPATH.
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "menus"))
from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows  # noqa: E402
from verify_player_startup import source_hashes  # noqa: E402

SOURCES = tuple(
    "native_assets/" + name
    for name in (
        "ai_policy.c",
        "ai_policy.h",
        "ai_policy_original_check.c",
        "ai_policy_mutants.json",
        "ai_policy_sources.json",
        "ai_policy.md",
        "build_ai_policy_native.py",
        "build_channel_callbacks_native.py",
        "build_windows_ai_policy_original_check.sh",
        "verify_ai_policy.py",
        "actor_ai_dispatch.c",
        "actor_ai_dispatch.h",
        "actor_ai.c",
        "actor_ai.h",
        "locomotion_predicates.c",
        "locomotion_predicates.h",
        "gekko_float.h",
        "gekko_quaternion/quaternion_kernels.c",
        "gekko_quaternion/quaternion_kernels.h",
    )
)

LIVE = ("CMakeLists.txt", "live_ai.c", "live_ai.h", "live_main.c")


def live_hashes(root: Path) -> dict[str, str]:
    return {name: digest(root.parent / "native/live" / name) for name in LIVE}


INPUT_HASHES = {
    14: "ce01b500e8d247894b3a6f0c61d20c352af7000424244049c2dfcb942310d3f0",
    15: "d298e2dd2f5ea872ac14287e2e65e7ac75f8aa7c70fd8381e0f3805683aa40a2",
    16: "3156f4635869c55ba47c4d6b3dabb3051d8b1da8ca4eb353b741bd5063fa7046",
}
SUMMARY = re.compile(
    r"Original AI policy: 401 cases \(saved \+ 400 edited\), "
    r"services 800AAC08/800AFC88/800A0D10/8006E2E4/8007C180 actors "
    r"(\d+)/(\d+)/(\d+)/(\d+)/(\d+) runs (\d+)/(\d+)/(\d+)/(\d+)/(\d+); "
    r"dispatch 5213; persistent 24 x 32 x 13 = 9984; "
    r"all RAM outside oracle stack and returns match; no original hooks; digest [0-9a-f]{8}\."
)
BOUNDARY = (
    "GNNE69 r0 native 800AAC08, 800AFC88, 800A0D10, 8006E2E4, 8007C180 and source-derived helper closure. "
    "Every saved actor assigned to each of the five entries gets saved-as-is and 400 edited direct cases. "
    "Dispatch and finalization compare after each actor operation; persistent sequences also commit inputs. "
    "All MEM1 outside 80F38000..80F46FFF and defined policy return values compared against UNHOOKED originals. "
    "Native code has no original-backed service. Static C functions preserve instruction order; no runtime decoder. "
    "Unreconstructed system instructions and unknown indirect targets reject. No movement, animation advancement, "
    "collision or autonomous-play completion claim; live movement is measured separately."
)


def build_checker(here: Path, out: Path) -> Path:
    """Compile only, preserving any older executable Windows still has open."""
    builds = private(out / "builds")
    builds.mkdir(parents=True, exist_ok=True)
    build = private(Path(tempfile.mkdtemp(prefix="checker-", dir=builds)))
    run(
        ["bash", str(here / "build_windows_ai_policy_original_check.sh"), str(build)],
        out / "build.log",
    )
    return build / "ai-policy-original-check.exe"


def check_sources(root: Path, expected: dict[str, str], stage: str, record: dict[str, object]) -> None:
    """Reject concurrent edits before mixing baseline and mutant provenance."""
    current = source_hashes(root, SOURCES)
    if "live_sha256" in record and live_hashes(root) != record["live_sha256"]:
        raise RuntimeError(f"Live integration changed during verification ({stage})")
    changes = {
        name: {"before": expected.get(name), "after": current.get(name)}
        for name in sorted(expected.keys() | current.keys())
        if expected.get(name) != current.get(name)
    }
    if changes:
        record.update(source_changes=changes, source_change_stage=stage)
        raise RuntimeError(f"Sources changed during verification ({stage}): {', '.join(changes)}")


def mutants(
    here: Path,
    out: Path,
    contexts: dict[int, Path],
    sources: dict[str, str],
    record: dict[str, object],
) -> list[dict[str, object]]:
    edits = json.loads((here / "ai_policy_mutants.json").read_text())
    if len(edits) < 5 or len({edit["name"] for edit in edits}) != len(edits):
        raise RuntimeError("Missing or duplicate mutants")
    results: list[dict[str, object]] = []
    record["mutants"] = results
    for edit in edits:
        check_sources(here.parent, sources, f"before mutant {edit['name']}", record)
        name, filename, old, new = (edit[key] for key in ("name", "file", "old", "new"))
        if not re.fullmatch(r"[a-z0-9-]+", name) or filename != "ai_policy.c" or "\n" in old + new or old == new:
            raise RuntimeError("Mutants must be named one-line edits to ai_policy.c")
        scratch = private(out / "mutants" / name)
        tree = scratch / "source" / "native_assets"
        for entry in sources:
            if entry.startswith("ModernGekko/"):
                continue
            destination = scratch / "source" / entry
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(here.parent / entry, destination)
        copied = source_hashes(scratch / "source", SOURCES)
        if copied != sources:
            raise RuntimeError(f"Mutant {name} source copy differs from the pinned baseline sources")
        target = tree / filename
        source = target.read_text()
        if source.count(old) != 1:
            raise RuntimeError(f"Mutant {name} does not match exactly once")
        target.write_text(source.replace(old, new))
        exe = build_checker(tree, scratch)
        check_sources(here.parent, sources, f"after building mutant {name}", record)
        caught = False
        for context, snapshot in contexts.items():
            log = scratch / f"original-{context}.log"
            with log.open("w") as output:
                result = subprocess.run(
                    [str(exe), windows(snapshot), windows(BASELINES["original_dll"][0])],
                    stdout=output,
                    stderr=subprocess.STDOUT,
                    check=False,
                )
            # Crashes, native rejections and build failures are NOT mutant kills.
            if result.returncode == 7 and any(
                marker in log.read_text() for marker in ("RAM mismatch", "return mismatch")
            ):
                results.append(
                    {
                        **edit,
                        "caught_context": context,
                        "log_sha256": digest(log),
                        "executable": str(exe),
                        "executable_sha256": digest(exe),
                        "mutated_source_sha256": digest(target),
                    }
                )
                caught = True
                break
            if result.returncode != 0:
                raise RuntimeError(f"Mutant {name} failed without a RAM/return mismatch: exit {result.returncode}")
        check_sources(here.parent, sources, f"after mutant {name}", record)
        if not caught:
            raise RuntimeError(f"Mutant {name} escaped all contexts")
    return results


def main() -> None:
    here = Path(__file__).resolve().parent
    root = here.parent
    entries = SOURCES
    out = private(PRIVATE / "research-temp/nflstreet-ai-policy-20261015")
    out.mkdir(parents=True, exist_ok=True)
    report = out / "verification.json"
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY}
    report.write_text(json.dumps(record, indent=2) + "\n")
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        record["live_sha256"] = live_hashes(root)
        contexts = {i: PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in (14, 15, 16)}
        inputs = {str(path): digest(path) for path in contexts.values()}
        record.update(baseline=before, input_sha256=inputs, source_sha256=sources)
        if any(inputs[str(path)] != INPUT_HASHES[i] for i, path in contexts.items()):
            raise RuntimeError("Saved MEM1 input differs from the recorded context")
        check_sources(root, sources, "before baseline build", record)
        exe = build_checker(here, out)
        record.update(executable=str(exe), executable_sha256=digest(exe))
        check_sources(root, sources, "after baseline build", record)
        logs: dict[str, list[str]] = {}
        record["original_log_sha256"] = logs
        for context, snapshot in contexts.items():
            traces = []
            for repeat in range(2):
                check_sources(root, sources, f"before context {context} repeat {repeat}", record)
                log = out / f"original-{context}-{repeat}.log"
                summary = run([str(exe), windows(snapshot), windows(BASELINES["original_dll"][0])], log).strip()
                match = SUMMARY.search(summary)
                if not match or not all(int(n) for n in match.groups()):
                    raise RuntimeError(f"Incomplete original actor AI coverage in context {context}: {summary}")
                populations = [int(n) for n in match.groups()[:5]]
                runs = [int(n) for n in match.groups()[5:]]
                if runs != [401 * n for n in populations]:
                    raise RuntimeError(f"Missing per-actor service coverage in context {context}")
                traces.append(digest(log))
                print(match[0], flush=True)
                # Retain completed comparisons even if a later provenance check fails.
                logs[str(context)] = traces.copy()
                check_sources(root, sources, f"after context {context} repeat {repeat}", record)
            if traces[0] != traces[1]:
                raise RuntimeError(f"Fresh AI policy processes differ in context {context}")
            logs[str(context)] = traces
        check_sources(root, sources, "before mutants", record)
        killed = mutants(here, out, contexts, sources, record)
        if before != baseline_hashes() or any(digest(Path(path)) != sha for path, sha in inputs.items()):
            raise RuntimeError("Protected originals or saved inputs changed")
        check_sources(root, sources, "final acceptance", record)
        record.update(
            status="passed",
            baseline=before,
            input_sha256=inputs,
            source_sha256=sources,
            original_log_sha256=logs,
            executable_sha256=digest(exe),
            mutants=killed,
            cases_per_context=401,
            persistent_sequences_per_context=24,
            updates_per_sequence=32,
            actors=13,
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print(f"Original AI policy passes all three contexts twice; {len(killed)}/{len(killed)} mutants caught; {report}")


if __name__ == "__main__":
    main()
