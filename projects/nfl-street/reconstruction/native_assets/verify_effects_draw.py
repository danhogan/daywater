#!/usr/bin/env python3
"""Verify the native actor fans, strips and particle emitters (scene node entries 11, 15, 16) against the original."""

import json
from pathlib import Path

from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

EXPECTED = {
    14: "Saved frame: 3869 GX calls (3612 pipe writes)\nRefused unmodeled inputs: 6 of 6.\n"
    "Original effects: 1203 runs of 80040C60, 80040C84 and 800CF284 over 401 cases match 1291638 GX calls "
    "and 1186493 pipe writes (1951 fans, 5802 strips, 22499 particle quads), all RAM; digest 00d709cc.",
    15: "Saved frame: 2487 GX calls (2296 pipe writes)\nRefused unmodeled inputs: 6 of 6.\n"
    "Original effects: 1203 runs of 80040C60, 80040C84 and 800CF284 over 401 cases match 1069457 GX calls "
    "and 974180 pipe writes (1878 fans, 4932 strips, 17924 particle quads), all RAM; digest 1c7f6a82.",
    16: "Saved frame: 3264 GX calls (3028 pipe writes)\nRefused unmodeled inputs: 6 of 6.\n"
    "Original effects: 1203 runs of 80040C60, 80040C84 and 800CF284 over 401 cases match 1032027 GX calls "
    "and 940077 pipe writes (2049 fans, 6124 strips, 14829 particle quads), all RAM; digest cf63dfe7.",
}
SOURCES = (
    "effects_draw.c",
    "effects_draw.h",
    "effects_draw_original_check.c",
    "scenery_draw.c",
    "marks_draw.c",
    "matrix_stack.c",
    "draw_engine.c",
    "check_fixture.h",
    "venue_fixture.h",
    "gx_oracle.h",
    "build_windows_effects_draw_original_check.sh",
    "verify_effects_draw.py",
)
BOUNDARY = (
    "Native scene node entries 11 (80040C60 -> 801646CC: the actors' textured fans), 15 (80040C84 -> 801649C4 -> "
    "801590F4: four triangle strips per actor with flag 0x10000) and 16 (800CF284 -> 80158D24 and 80158798 per "
    "particle emitter, textured or flat quads). Compared call by call (GX SDK entries and every immediate write to "
    "the GX pipe) and as all RAM outside the oracle stack against the original module on the same prepared RAM, "
    "with the original routines executing in full (only the GX SDK entries and the OS cache routines return). "
    "Refused natively and checked as refusals: the scaled-actor gate 800DFAF0 (r13-7238), the particle gradient "
    "80198D74, 800CF66C's 80158E00 (r13-72FC == 1), 80167874 (r13-58BC), a zero sprite-sheet divisor and a missing "
    "emitter shader. Entry 14 (800E5CA0 -> 800D1118) and the clouds are not modeled."
)


def main() -> None:
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / "research-temp/nflstreet-effects-draw-20261014")
    out.mkdir(parents=True, exist_ok=True)
    report = out / "verification.json"
    entries = tuple("native_assets/" + name for name in SOURCES)
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY}
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        contexts = {i: PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in (14, 15, 16)}
        inputs = {str(path): digest(path) for path in contexts.values()}
        run(["bash", str(here / "build_windows_effects_draw_original_check.sh"), str(out)], out / "build.log")
        exe = out / "effects-draw-original-check.exe"
        logs: dict[str, list[str]] = {}
        for i, context in contexts.items():
            traces = []
            for repeat in range(2):
                log = out / f"original-{i}-{repeat}.log"
                text = run([str(exe), windows(context), windows(BASELINES["original_dll"][0])], log)
                if text.replace("\r\n", "\n").strip() != EXPECTED[i]:
                    raise RuntimeError(f"Original effects comparison failed in context {i}")
                traces.append(str(digest(log)))
            if traces[0] != traces[1]:
                raise RuntimeError("Fresh original effects traces differ")
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
            cases_per_context=401,
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print("Original effects draw passes; private report " + str(report))


if __name__ == "__main__":
    main()
