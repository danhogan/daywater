#!/usr/bin/env python3
"""Integration gate: unhooked original assignment-follow leaves, all RAM, fresh processes, and compiled mutants."""

import json
import re
import shutil
import subprocess
from pathlib import Path

from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

SOURCES = (
    "actor_ai.c",
    "actor_ai.h",
    "actor_ai_original_check.c",
    "actor_ai_mutants.json",
    "build_windows_actor_ai_original_check.sh",
    "verify_actor_ai.py",
    "gekko_float.h",
    "gekko_quaternion/quaternion_kernels.c",
    "gekko_quaternion/quaternion_kernels.h",
)
INPUT_HASHES = {
    14: "ce01b500e8d247894b3a6f0c61d20c352af7000424244049c2dfcb942310d3f0",
    15: "d298e2dd2f5ea872ac14287e2e65e7ac75f8aa7c70fd8381e0f3805683aa40a2",
    16: "3156f4635869c55ba47c4d6b3dabb3051d8b1da8ca4eb353b741bd5063fa7046",
}
SUMMARY = re.compile(
    r"Original actor AI: 401 cases \(saved \+ 400 edited\), 14 actors; "
    r"decode 5614 enter 5614 target (\d+) project 5614 composed 5614; "
    r"persistent 24 sequences x 32 updates x 14 actors = 10752; "
    r"all RAM outside oracle stack matches; coverage near/mid/far (\d+)/(\d+)/(\d+) "
    r"preserve/raise (\d+)/(\d+) sign (\d+)/(\d+) clamp (\d+)/(\d+)/(\d+); digest [0-9a-f]{8}\."
)
BOUNDARY = (
    "GNNE69 r0: original 8008D5DC signed assignment decode, 8008D954 state entry, "
    "8008D764 width-adjusted offset target, 8008D864 target-to-requested-input, and their composition. "
    "No hooks, skips or native stand-ins in the original. All RAM outside 80F38000..80F46FFF compared. "
    "Finite inputs/intermediates; saved +154 union lanes that are nonfinite are excluded only from the "
    "isolated target suite; composed suite initializes every actor through original/native 8008D954. "
    "Persistent runs supply external XY samples and target actor arguments; no physics/play-call simulation. "
    "8008D988 resolver, eligibility, transitions, facing, interactions and input commit remain unmodeled."
)


def mutants(here: Path, out: Path, contexts: dict[int, Path]) -> list[dict[str, object]]:
    edits = json.loads((here / "actor_ai_mutants.json").read_text())
    if len(edits) < 5 or len({edit["name"] for edit in edits}) != len(edits):
        raise RuntimeError("Missing or duplicate mutants")
    results = []
    for edit in edits:
        name, filename, old, new = (edit[key] for key in ("name", "file", "old", "new"))
        if not re.fullmatch(r"[a-z0-9-]+", name) or filename != "actor_ai.c" or "\n" in old + new or old == new:
            raise RuntimeError("Mutants must be named one-line edits to actor_ai.c")
        scratch = private(out / "mutants" / name)
        tree = scratch / "source" / "native_assets"
        for entry in SOURCES:
            destination = tree / entry
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(here / entry, destination)
        target = tree / filename
        source = target.read_text()
        if source.count(old) != 1:
            raise RuntimeError(f"Mutant {name} does not match exactly once")
        target.write_text(source.replace(old, new))
        run(["bash", str(tree / "build_windows_actor_ai_original_check.sh"), str(scratch)], scratch / "build.log")
        exe = scratch / "actor-ai-original-check.exe"
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
            if result.returncode == 7 and "RAM mismatch" in log.read_text():
                results.append(
                    {
                        **edit,
                        "caught_context": context,
                        "log_sha256": digest(log),
                        "executable_sha256": digest(exe),
                        "mutated_source_sha256": digest(target),
                    }
                )
                caught = True
                break
            if result.returncode != 0:
                raise RuntimeError(f"Mutant {name} failed without an all-RAM mismatch: exit {result.returncode}")
        if not caught:
            raise RuntimeError(f"Mutant {name} escaped all contexts")
    return results


def main() -> None:
    here = Path(__file__).resolve().parent
    root = here.parent
    entries = tuple("native_assets/" + name for name in SOURCES)
    out = private(PRIVATE / "research-temp/nflstreet-ai-20261015")
    out.mkdir(parents=True, exist_ok=True)
    report = out / "verification.json"
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY}
    report.write_text(json.dumps(record, indent=2) + "\n")
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        contexts = {i: PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in (14, 15, 16)}
        inputs = {str(path): digest(path) for path in contexts.values()}
        if any(inputs[str(path)] != INPUT_HASHES[i] for i, path in contexts.items()):
            raise RuntimeError("Saved MEM1 input differs from the recorded context")
        run(["bash", str(here / "build_windows_actor_ai_original_check.sh"), str(out)], out / "build.log")
        exe = out / "actor-ai-original-check.exe"
        logs = {}
        for context, snapshot in contexts.items():
            traces = []
            for repeat in range(2):
                log = out / f"original-{context}-{repeat}.log"
                summary = run([str(exe), windows(snapshot), windows(BASELINES["original_dll"][0])], log).strip()
                match = SUMMARY.fullmatch(summary)
                if not match or not 5600 <= int(match[1]) <= 5614 or not all(int(n) for n in match.groups()[1:]):
                    raise RuntimeError(f"Incomplete original actor AI coverage in context {context}: {summary}")
                traces.append(digest(log))
                print(summary, flush=True)
            if traces[0] != traces[1]:
                raise RuntimeError(f"Fresh actor AI processes differ in context {context}")
            logs[str(context)] = traces
        killed = mutants(here, out, contexts)
        if before != baseline_hashes() or any(digest(Path(path)) != sha for path, sha in inputs.items()):
            raise RuntimeError("Protected originals or saved inputs changed")
        if sources != source_hashes(root, entries):
            raise RuntimeError("Sources changed during verification")
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
            actors=14,
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print(f"Original actor AI passes all three contexts twice; {len(killed)}/{len(killed)} mutants caught; {report}")


if __name__ == "__main__":
    main()
