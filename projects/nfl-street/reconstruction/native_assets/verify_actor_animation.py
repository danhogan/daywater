#!/usr/bin/env python3
"""Verify native actor animation (8015F08C/801EAE1C, 8015E9C8, 8015EC30, 8015E938, 80053404) against the original and
run the live pose walk demo, in three saved contexts."""

import json
from pathlib import Path

from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

COMMON = (
    "Original actor animation: 5614 runs of 8015F08C (4018 after 801893C8(1)/801893EC, 1104 with flag 0x2000, 1136 "
    "moved destinations) match all RAM and the locked cache; 5614 runs of 8015E9C8 over 15618 entries "
    "(streamed/compressed/synthetic 5710/5234/4674, 7853 mirrored, 7712 flag 4; modes 1266/1556/1356/1436) match all "
    "RAM and its stack frame; 2814 runs of 8015EC30 ({active} active channels, 735 with flag 0x80), 804 of 8015E938 "
    "(387 mirrored) and 2814 of 80053404 match all RAM; the live channel-root inference reproduces {inferred} saved "
    "active bank 7 channels exactly; digest {digest}."
)
EXPECTED = {
    14: COMMON.format(active=7760, inferred="20 of 22", digest="97b981eb"),
    15: COMMON.format(active=7732, inferred="19 of 22", digest="76745929"),
    16: COMMON.format(active=7439, inferred="18 of 20", digest="5b068c8a"),
}
WALK = (
    "Live pose demo: controlled gameplay 803c19c0 visual 80b8e758, 150 updates walking then 60 stopped; 210 pose "
    "updates, 3 locomotion pair starts, 2 completed fades; right knee angle {knee}; ankle stride {stride} with 8 "
    "alternations while walking; ankle palettes changed in {changed} updates."
)
DEMO = {
    14: WALK.format(knee="6952..21864", stride="-0.845..0.904", changed=208),
    15: WALK.format(knee="7005..21839", stride="-0.845..0.901", changed=209),
    16: WALK.format(knee="6942..21797", stride="-0.845..0.904", changed=208),
}
SOURCES = (
    "actor_pose.c",
    "actor_pose.h",
    "actor_skeleton.c",
    "actor_skeleton.h",
    "compact_pose.c",
    "compact_pose.h",
    "actor_palettes.h",
    "matrix_stack.c",
    "matrix_stack.h",
    "draw_engine.c",
    "draw_engine.h",
    "check_fixture.h",
    "venue_fixture.h",
    "actor_animation_original_check.c",
    "build_windows_actor_animation_original_check.sh",
    "live_pose.c",
    "live_pose.h",
    "live_pose_demo.c",
    "build_windows_live_pose_demo.sh",
    "live_locomotion.c",
    "live_locomotion.h",
    "live_movement.c",
    "actor_palettes.c",
    "actor_world.c",
    "cluster_palette.c",
    "model_view.c",
    "pose.c",
    "verify_actor_animation.py",
)
BOUNDARY = (
    "Native 8015F08C/801EAE1C (compact skeleton through the locked cache and DMA), 8015E9C8 (pose list blend with "
    "8015DBFC/8015D584, scattered 8015D104, 801667CC/80166770/80166730 and the root and ball fields), 8015EC30, "
    "8015E938 and 80053404 on saved RAM, compared as all RAM (plus the locked cache and 8015E9C8's stack frame) "
    "against the original module; the cache enable/flush/store 801F0C84/801F0BA0/801F0BD0 return and the DMA "
    "801F0DD4 is served from the oracle's locked cache. The live pose source's channel-root inference is counted "
    "against the saved bank 7 channels. The live walk demo is a plausibility check of host clip selection "
    "(live_locomotion.c), not an original comparison."
)
BANK = PRIVATE / "private/nflstreet/GNNE69-r0/unpacked/animdata/00111.bin"
SETS = PRIVATE / "research-temp/nflstreet-set-playback-20261009/sets-14.bin"


def main() -> None:
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / "research-temp/nflstreet-actor-animation-20261014")
    out.mkdir(parents=True, exist_ok=True)
    report = out / "verification.json"
    entries = tuple("native_assets/" + name for name in SOURCES)
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY}
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        contexts = {i: PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in (14, 15, 16)}
        inputs = {str(path): digest(path) for path in (*contexts.values(), BANK, SETS)}
        run(["bash", str(here / "build_windows_actor_animation_original_check.sh"), str(out)], out / "build.log")
        run(["bash", str(here / "build_windows_live_pose_demo.sh"), str(out)], out / "build-demo.log")
        exe = out / "actor-animation-original-check.exe"
        demo = out / "live-pose-demo.exe"
        logs: dict[str, list[str]] = {}
        for i, context in contexts.items():
            traces = []
            for repeat in range(2):
                log = out / f"original-{i}-{repeat}.log"
                text = run([str(exe), windows(context), windows(BASELINES["original_dll"][0]), windows(BANK)], log)
                if text.replace("\r\n", "\n").strip() != EXPECTED[i]:
                    raise RuntimeError(f"Original actor animation comparison failed in context {i}")
                traces.append(str(digest(log)))
            if traces[0] != traces[1]:
                raise RuntimeError("Fresh original actor animation traces differ")
            csv = out / f"walk-{i}.csv"
            text = run([str(demo), windows(context), windows(BANK), windows(SETS), windows(csv)], out / f"demo-{i}.log")
            if text.replace("\r\n", "\n").strip() != DEMO[i]:
                raise RuntimeError(f"Live pose demo failed in context {i}")
            traces.append(str(digest(csv)))
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
            log_and_csv_sha256=logs,
            executable_sha256={"oracle": digest(exe), "demo": digest(demo)},
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print("Original actor animation and live pose demo pass; private report " + str(report))


if __name__ == "__main__":
    main()
