#!/usr/bin/env python3
"""Verify native actor palettes (80162760) and placement (800D4A6C) against the original in three saved contexts."""

import json
from pathlib import Path

from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

EXPECTED = {
    14: "Original actor palettes: 5614 runs of 80162760 over 14 actors x 401 cases (LOD 0/1/2/3: "
    "452/2609/2083/470, 1294 with a scale vector) with 482 hand and 759 held-item palettes match all RAM "
    "and the locked cache; 809254 palette matrices allocated; 5614 runs of 800D4A6C over 401 cases (36 "
    "mirrored) match all RAM; digest b6d24192.",
    15: "Original actor palettes: 5614 runs of 80162760 over 14 actors x 401 cases (LOD 0/1/2/3: "
    "452/2609/2083/470, 1294 with a scale vector) with 482 hand and 759 held-item palettes match all RAM "
    "and the locked cache; 809254 palette matrices allocated; 5614 runs of 800D4A6C over 401 cases (36 "
    "mirrored) match all RAM; digest ff5c0443.",
    16: "Original actor palettes: 5614 runs of 80162760 over 14 actors x 401 cases (LOD 0/1/2/3: "
    "452/2609/2083/470, 1294 with a scale vector) with 482 hand and 521 held-item palettes match all RAM "
    "and the locked cache; 800686 palette matrices allocated; 5614 runs of 800D4A6C over 401 cases (36 "
    "mirrored) match all RAM; digest 96c8ae54.",
}
SOURCES = (
    "actor_palettes.c",
    "actor_palettes.h",
    "actor_world.c",
    "actor_world.h",
    "cluster_palette.c",
    "cluster_palette.h",
    "model_view.c",
    "model_view.h",
    "matrix_stack.c",
    "matrix_stack.h",
    "draw_engine.c",
    "draw_engine.h",
    "check_fixture.h",
    "venue_fixture.h",
    "actor_palettes_original_check.c",
    "build_windows_actor_palettes_original_check.sh",
    "verify_actor_palettes.py",
)
BOUNDARY = (
    "Native 80162760 as 80164000 calls it (no lineup record): body, attachments (+5A8, +788, +968) with the "
    "+1358 scale on tagged rows, head (+698) on skeleton bone 13, hands (LOD 0) and held items, through the "
    "801CA4B0 pool, 801CA4E8/801CA720 cluster palettes, the locked cache and its 32-byte-block DMA 801F0DD4. "
    "Compared as all RAM and the locked cache against the original module, with the enabled dynamics 8004C268/"
    "8004BF1C returning at once (their disabled form is modeled), the cache enable/flush 801F0C84/801F0BD0 "
    "returning and DMA served from the oracle's locked cache. Flag 0x4000 (801CA900) is rejected natively. "
    "Native 800D4A6C/800D4B0C place every visual from its gameplay actor, compared as all RAM."
)


def main() -> None:
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / "research-temp/nflstreet-actor-palettes-20261013")
    out.mkdir(parents=True, exist_ok=True)
    report = out / "verification.json"
    entries = tuple("native_assets/" + name for name in SOURCES)
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY}
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        contexts = {i: PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in (14, 15, 16)}
        inputs = {str(path): digest(path) for path in contexts.values()}
        run(["bash", str(here / "build_windows_actor_palettes_original_check.sh"), str(out)], out / "build.log")
        exe = out / "actor-palettes-original-check.exe"
        logs: dict[str, list[str]] = {}
        for i, context in contexts.items():
            traces = []
            for repeat in range(2):
                log = out / f"original-{i}-{repeat}.log"
                text = run([str(exe), windows(context), windows(BASELINES["original_dll"][0])], log)
                if text.replace("\r\n", "\n").strip() != EXPECTED[i]:
                    raise RuntimeError(f"Original actor palette comparison failed in context {i}")
                traces.append(str(digest(log)))
            if traces[0] != traces[1]:
                raise RuntimeError("Fresh original actor palette traces differ")
            logs[str(i)] = traces
        if before != baseline_hashes() or any(digest(Path(path)) != sha for path, sha in inputs.items()):
            raise RuntimeError("Protected originals or private inputs changed")
        if sources != source_hashes(root, entries):
            raise RuntimeError("Source changed during verification")
        record.update(
            status="passed",
            baseline=before,
            input_sha256=inputs,
            source_sha256=sources,
            original_log_sha256=logs,
            executable_sha256=digest(exe),
            palette_cases_per_context=401,
            placement_cases_per_context=401,
            actors=14,
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print("Original actor palettes and placement pass; private report " + str(report))


if __name__ == "__main__":
    main()
