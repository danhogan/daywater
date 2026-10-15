#!/usr/bin/env python3
"""Integration gate: state dispatch and input commit boundaries, all RAM, fresh processes, and compiled mutants."""

import json
import re
import shutil
import subprocess
from pathlib import Path

from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

SOURCES = tuple(
    "native_assets/" + name
    for name in (
        "actor_ai_dispatch.c",
        "actor_ai_dispatch.h",
        "actor_ai_dispatch_original_check.c",
        "actor_ai_dispatch_mutants.json",
        "actor_ai_dispatch.md",
        "build_windows_actor_ai_dispatch_original_check.sh",
        "verify_actor_ai_dispatch.py",
        "actor_ai.c",
        "actor_ai.h",
        "locomotion_predicates.c",
        "locomotion_predicates.h",
        "gekko_float.h",
        "gekko_quaternion/quaternion_kernels.c",
        "gekko_quaternion/quaternion_kernels.h",
    )
)
INPUT_HASHES = {
    14: "ce01b500e8d247894b3a6f0c61d20c352af7000424244049c2dfcb942310d3f0",
    15: "d298e2dd2f5ea872ac14287e2e65e7ac75f8aa7c70fd8381e0f3805683aa40a2",
    16: "3156f4635869c55ba47c4d6b3dabb3051d8b1da8ca4eb353b741bd5063fa7046",
}
SUMMARY = re.compile(
    r"Original actor AI dispatch: 401 cases \(saved \+ 400 edited\) x 14 actors; "
    r"step 5614 transition 5614 commit 5614 follow 5614 eligibility 5614 facing 5614 interaction 5614 finish 5614; "
    r"persistent 24 x 32 x 14 = 10752; all RAM outside oracle stack and service effects match; "
    r"entry/pop/remove (\d+)/(\d+)/(\d+); remove phases (\d+)/(\d+); "
    r"original-backed policy calls (\d+); digest [0-9a-f]{8}\."
)
BOUNDARY = (
    "GNNE69 r0 native 8007E0B4 state-table dispatcher, entry/pop/replace/enqueue/remove, "
    "8008D988 follow callback, 8008D654 eligibility, 8005335C facing and 80021EF0 interaction lifecycle. "
    "Input commit reuses native 800150E8 locomotion selector, followed by native 800F8214 normalization. "
    "Unknown state callbacks and reached 8009AD80/8009E590/8005DCF8/800F8088 policy branches are ORIGINAL-backed "
    "services on the native side: this verifies their scheduling, not their reconstruction. "
    "Original routines 80177EF4/80076C48/80076E18/80021B58/80021AFC return at entry on both sides, "
    "with argument traces compared, never native stand-ins in the oracle. All other originals unhooked. "
    "All RAM outside 80F38000..80F46FFF compared after every actor call/update. "
    "Edited queues/tables use actual original leaf callbacks; no generated emulated instructions. "
    "Standalone follow and edited dispatcher/transition tables enter follow once before their batch; "
    "persistent sequences do so once before updates, because saved +154/+158 belong to other state unions. "
    "Every entry is compared too. Saved dispatcher/transition cases preserve their original state. "
    "Removal coverage counts original callback return predicates at 8007E110/190, since the "
    "8007E5A4 calls jump inside the compiled chunk; both controlled/update removal phases are required. "
    "No complete autonomous policies, football physics or original movement simulation claimed."
)


def mutants(here: Path, out: Path, contexts: dict[int, Path]) -> list[dict[str, object]]:
    edits = json.loads((here / "actor_ai_dispatch_mutants.json").read_text())
    if len(edits) < 5 or len({edit["name"] for edit in edits}) != len(edits):
        raise RuntimeError("Missing or duplicate mutants")
    results = []
    for edit in edits:
        name, filename, old, new = (edit[key] for key in ("name", "file", "old", "new"))
        if (
            not re.fullmatch(r"[a-z0-9-]+", name)
            or filename != "actor_ai_dispatch.c"
            or "\n" in old + new
            or old == new
        ):
            raise RuntimeError("Mutants must be named one-line edits to actor_ai_dispatch.c")
        scratch = private(out / "mutants" / name)
        tree = scratch / "source" / "native_assets"
        hashes = source_hashes(here.parent, SOURCES)
        for entry in hashes:
            if entry.startswith("ModernGekko/"):
                continue
            destination = scratch / "source" / entry
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(here.parent / entry, destination)
        target = tree / filename
        source = target.read_text()
        if source.count(old) != 1:
            raise RuntimeError(f"Mutant {name} does not match exactly once")
        target.write_text(source.replace(old, new))
        run(
            ["bash", str(tree / "build_windows_actor_ai_dispatch_original_check.sh"), str(scratch)],
            scratch / "build.log",
        )
        exe = scratch / "actor-ai-dispatch-original-check.exe"
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
                marker in log.read_text() for marker in ("RAM mismatch", "return mismatch", "effect trace mismatch")
            ):
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
                raise RuntimeError(
                    f"Mutant {name} failed without a RAM/return/effect mismatch: exit {result.returncode}"
                )
        if not caught:
            raise RuntimeError(f"Mutant {name} escaped all contexts")
    return results


def main() -> None:
    here = Path(__file__).resolve().parent
    root = here.parent
    entries = SOURCES
    out = private(PRIVATE / "research-temp/nflstreet-ai2-20261015")
    out.mkdir(parents=True, exist_ok=True)
    report = out / "verification.json"
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY}
    report.write_text(json.dumps(record, indent=2) + "\n")
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        contexts = {i: PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in (14, 15, 16)}
        inputs = {str(path): digest(path) for path in contexts.values()}
        record.update(baseline=before, input_sha256=inputs, source_sha256=sources)
        if any(inputs[str(path)] != INPUT_HASHES[i] for i, path in contexts.items()):
            raise RuntimeError("Saved MEM1 input differs from the recorded context")
        run(["bash", str(here / "build_windows_actor_ai_dispatch_original_check.sh"), str(out)], out / "build.log")
        exe = out / "actor-ai-dispatch-original-check.exe"
        logs = {}
        for context, snapshot in contexts.items():
            traces = []
            for repeat in range(2):
                log = out / f"original-{context}-{repeat}.log"
                summary = run([str(exe), windows(snapshot), windows(BASELINES["original_dll"][0])], log).strip()
                match = SUMMARY.search(summary)
                if not match or not all(int(n) for n in match.groups()):
                    raise RuntimeError(f"Incomplete original actor AI coverage in context {context}: {summary}")
                traces.append(digest(log))
                print(match[0], flush=True)
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
    print(
        f"Original actor AI dispatch passes all three contexts twice; "
        f"{len(killed)}/{len(killed)} mutants caught; {report}"
    )


if __name__ == "__main__":
    main()
