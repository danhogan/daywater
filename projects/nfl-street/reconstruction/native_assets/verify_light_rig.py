#!/usr/bin/env python3
"""Verify the native per-frame light update (801ED3A0) against the original in three saved contexts."""

import json
from pathlib import Path

from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

LINE = (
    "Original light update: 401 runs of 801ED3A0 (389 with a rig, 12 without) match all RAM; lights by type 0-5: "
    "1005/399/1015/122/442/129 (1490 with callbacks); 5342 set lights (515 specular), 1681 ambient colors; shadows "
    "by type 0-3: 123/223/55/60; 201 frame-order runs (801DF458, 801DFAB0, 801ED3A0) match the native field view, "
    "slot 3 and update; the saved camera "
)
# Context 15's saved field camera is the drawn one; in 14 and 16 it has already moved (ball_and_marks.md).
EXPECTED = {
    14: LINE + "does not reproduce the saved light sets; digest 69e47d6e.",
    15: LINE + "reproduces the saved light sets; digest 37a90390.",
    16: LINE + "does not reproduce the saved light sets; digest b199d1fa.",
}
SOURCES = (
    "light_rig.c",
    "light_rig.h",
    "light_rig_fixture.h",
    "hud_draw.c",
    "hud_draw.h",
    "uis_draw.c",
    "uis_draw.h",
    "ball_draw.c",
    "ball_draw.h",
    "gx_texobj.c",
    "scenery_draw.c",
    "scenery_draw.h",
    "marks_draw.c",
    "marks_draw.h",
    "matrix_stack.c",
    "matrix_stack.h",
    "draw_engine.c",
    "draw_engine.h",
    "check_fixture.h",
    "venue_fixture.h",
    "hud_fixture.h",
    "gx_oracle.h",
    "light_rig_original_check.c",
    "build_windows_light_rig_original_check.sh",
    "verify_light_rig.py",
)
BOUNDARY = (
    "Native 801ED3A0 (the field camera node's phase-1 callback): light callbacks 8015B858/8015B8A4/8015B8C4, "
    "world directions normalized in place, view directions from the stack top (801DEB54), light sets through "
    "801ED998 with the GX light initializers, shadow projections 801ECEA4/801ECF80 and colors 801ED024, and the "
    "r13-6220 clear. Compared as all RAM against the original module with the memory-only GX light initializers "
    "executing and no other GX entry allowed. Frame-order runs execute 801DF458 and 801DFAB0 (other GX entries "
    "returning) before 801ED3A0 and compare the light outputs, slot 3 and the stack top with the native field "
    "view (8017D16C), slot-3 copy and update. Other light callbacks are rejected natively."
)


def main() -> None:
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / "research-temp/nflstreet-light-rig-20261014")
    out.mkdir(parents=True, exist_ok=True)
    report = out / "verification.json"
    entries = tuple("native_assets/" + name for name in SOURCES)
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY}
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        contexts = {i: PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in (14, 15, 16)}
        inputs = {str(path): digest(path) for path in contexts.values()}
        run(["bash", str(here / "build_windows_light_rig_original_check.sh"), str(out)], out / "build.log")
        exe = out / "light-rig-original-check.exe"
        logs: dict[str, list[str]] = {}
        for i, context in contexts.items():
            traces = []
            for repeat in range(2):
                log = out / f"original-{i}-{repeat}.log"
                text = run([str(exe), windows(context), windows(BASELINES["original_dll"][0])], log)
                if text.replace("\r\n", "\n").strip() != EXPECTED[i]:
                    raise RuntimeError(f"Original light update comparison failed in context {i}")
                traces.append(str(digest(log)))
            if traces[0] != traces[1]:
                raise RuntimeError("Fresh original light update traces differ")
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
            update_cases_per_context=401,
            frame_order_cases_per_context=201,
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print("Original light update passes; private report " + str(report))


if __name__ == "__main__":
    main()
