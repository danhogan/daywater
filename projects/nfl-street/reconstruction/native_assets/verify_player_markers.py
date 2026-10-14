#!/usr/bin/env python3
"""Verify the native player-select rings (800E4A24), camera slots (801DFAB0 tail) and name tag projection
(800E20E4/800DB0E4) against the original in three saved contexts."""

import json
from pathlib import Path

from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

EXPECTED = {
    14: "Saved frame: 1 shown tags re-projected within 3 pixels of the stored positions.\n"
    "Original player markers: 401 runs of 800E4A24 (1269 tracked rings, 99 pooled rings started, 359 released, "
    "170 moves of the controlled ring, 38 mirrored, 189 controlled marks); 401 runs of 801DFAB0's slot tail; "
    "401 x 14 runs of 800E20E4 (1643 projected, 719 hidden, 3252 without a shown tag) match all RAM; digest 5ddcbaed.",
    15: "Saved frame: 1 shown tags re-projected within 0 pixels of the stored positions.\n"
    "Original player markers: 401 runs of 800E4A24 (1269 tracked rings, 99 pooled rings started, 359 released, "
    "220 moves of the controlled ring, 38 mirrored, 189 controlled marks); 401 runs of 801DFAB0's slot tail; "
    "401 x 14 runs of 800E20E4 (1645 projected, 717 hidden, 3252 without a shown tag) match all RAM; digest 0c60d199.",
    16: "Saved frame: 1 shown tags re-projected within 3 pixels of the stored positions.\n"
    "Original player markers: 401 runs of 800E4A24 (1269 tracked rings, 99 pooled rings started, 359 released, "
    "170 moves of the controlled ring, 38 mirrored, 189 controlled marks); 401 runs of 801DFAB0's slot tail; "
    "401 x 14 runs of 800E20E4 (1638 projected, 724 hidden, 3252 without a shown tag) match all RAM; digest 8dc5b5a6.",
}
SOURCES = (
    "marker_ram.h",
    "select_rings.c",
    "select_rings.h",
    "camera_slots.c",
    "camera_slots.h",
    "name_tag_project.c",
    "name_tag_project.h",
    "matrix_stack.c",
    "matrix_stack.h",
    "draw_engine.c",
    "draw_engine.h",
    "check_fixture.h",
    "venue_fixture.h",
    "hud_fixture.h",
    "player_markers_original_check.c",
    "build_windows_player_markers_original_check.sh",
    "verify_player_markers.py",
)
BOUNDARY = (
    "Native 800E4A24 (player-select ring update with the pooled ring animation 800E44E4, pool start 800E4F34, "
    "position copy 800E41A8 and target lookup 800E3C48), the tail of 801DFAB0 (camera slots 3 to 6 from the "
    "viewport stack's view) and 800E20E4/800DB0E4 (name tag projection through slot 4 and slot 1), each compared "
    "as all RAM outside the oracle stack against the original module over a saved case and 400 edited cases per "
    "context. The only skipped original routine is the player draw 80164000 at the end of 800E20E4, which returns "
    "at once. 801DFAB0 runs from its first stack selection to its call of 801DF710. Slots 1 and 2 "
    "(801DF8C4/801DF938) are read as saved."
)


def main() -> None:
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / "research-temp/nflstreet-player-markers-20261014")
    out.mkdir(parents=True, exist_ok=True)
    report = out / "verification.json"
    entries = tuple("native_assets/" + name for name in SOURCES)
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY}
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        contexts = {i: PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in (14, 15, 16)}
        inputs = {str(path): digest(path) for path in contexts.values()}
        run(["bash", str(here / "build_windows_player_markers_original_check.sh"), str(out)], out / "build.log")
        exe = out / "player-markers-original-check.exe"
        logs: dict[str, list[str]] = {}
        for i, context in contexts.items():
            traces = []
            for repeat in range(2):
                log = out / f"original-{i}-{repeat}.log"
                text = run([str(exe), windows(context), windows(BASELINES["original_dll"][0])], log)
                if text.replace("\r\n", "\n").strip() != EXPECTED[i]:
                    raise RuntimeError(f"Original player marker comparison failed in context {i}")
                traces.append(str(digest(log)))
            if traces[0] != traces[1]:
                raise RuntimeError("Fresh original player marker traces differ")
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
    print("Original player markers pass; private report " + str(report))


if __name__ == "__main__":
    main()
