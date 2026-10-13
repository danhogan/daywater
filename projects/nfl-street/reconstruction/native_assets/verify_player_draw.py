#!/usr/bin/env python3
"""Verify the native player draw submission against the original 80161E10 in three saved contexts."""

import json
from pathlib import Path

from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

LIVE = (
    "Player draw: 14 live actors ({calls} GX calls, 340 display lists, 42 lights, 30 TwoPass, 415 TLUT loads, "
    "0 default textures, {cached} cached formats, 0 headwear); "
)
NATIVE = {
    14: LIVE.format(calls=4955, cached=260)
    + "840 controlled draws (363915 GX calls, 22989 display lists, 2112 lights (686 scaled), 2316 TwoPass, "
    "17729 TLUT loads, 1687 default textures, 1195 shader parameters, 14005 cached formats, 508 LOD-0 heads "
    "(134 compiled), 1562 refreshed instances, 242 headwear; light color: 290 restarts, 297 interpolating, "
    "159 fading, 47 fade ends, 70 tints; 186 clamped eye angles); 4 rejections; digest 51d7ff72.",
    15: LIVE.format(calls=4955, cached=260)
    + "840 controlled draws (363915 GX calls, 22989 display lists, 2112 lights (686 scaled), 2316 TwoPass, "
    "17729 TLUT loads, 1687 default textures, 1195 shader parameters, 14005 cached formats, 508 LOD-0 heads "
    "(134 compiled), 1562 refreshed instances, 242 headwear; light color: 290 restarts, 297 interpolating, "
    "159 fading, 47 fade ends, 70 tints; 186 clamped eye angles); 4 rejections; digest 1aa857df.",
    16: LIVE.format(calls=4947, cached=261)
    + "840 controlled draws (369649 GX calls, 23339 display lists, 2064 lights (683 scaled), 2329 TwoPass, "
    "17919 TLUT loads, 1671 default textures, 1084 shader parameters, 13987 cached formats, 520 LOD-0 heads "
    "(142 compiled), 1695 refreshed instances, 243 headwear; light color: 290 restarts, 301 interpolating, "
    "163 fading, 53 fade ends, 74 tints; 220 clamped eye angles); 4 rejections; digest 0d794dfa.",
}
ORIGINAL = {
    context: f"Original player draw: 854 80161E10 runs match every GX call ({calls} calls), the render state and "
    "all RAM outside the oracle stack."
    for context, calls in ((14, 368870), (15, 368870), (16, 374596))
}
SOURCES = (
    "player_draw.c",
    "player_draw.h",
    "draw_engine.c",
    "draw_engine.h",
    "player_draw_check.c",
    "player_draw_original_check.c",
    "build_windows_player_draw_check.sh",
    "build_windows_player_draw_original_check.sh",
    "verify_player_draw.py",
)
BOUNDARY = (
    "Native 80161E10 player draw submission: render-state reset 801C7648, light-color interpolation and fade, "
    "light set 1 scaling 801EDC60/801ED6A8, channel/alpha/cull/blend presets, the twelve 801EC140 model instances "
    "with 801EC0D4 chunk-list rebuilds, the 80162220 head (LOD 0 eye texture matrices 8016373C/801636E8, groups "
    "3/4/2), and per chunk 801C7D54/801C7B58 material application (801C7A3C shader switch, NULL/OnePass/TwoPass "
    "setups and 801CACE4 TEV presets, 801C7AD4 texture/TLUT loads, shader parameters) plus 801CCBD8/801C7F18 vertex "
    "descriptors, arrays and display lists. Compared as GX SDK calls (arguments and pointed-to values) recorded at "
    "the SDK entry points while the original module runs every game routine, then the render-state globals and "
    "all RAM outside the oracle stack. The head display-list compile behind 801626E0 is a supplied service; SDK "
    "functions, GPU execution and pixels are not modeled."
)


def checked(command: list[str], log: Path, expected: str, failure: str) -> str:
    """Run one check, require its exact summary output and return the log digest."""
    if run(command, log).replace("\r\n", "\n").strip() != expected:
        raise RuntimeError(failure)
    return str(digest(log))


def main() -> None:
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / "research-temp/nflstreet-player-draw-20261013")
    out.mkdir(parents=True, exist_ok=True)
    report = out / "verification.json"
    entries = tuple("native_assets/" + name for name in SOURCES)
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY}
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        contexts = {i: PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in (14, 15, 16)}
        inputs = {str(path): digest(path) for path in contexts.values()}
        for name in ("player_draw", "player_draw_original"):
            run(["bash", str(here / f"build_windows_{name}_check.sh"), str(out)], out / f"{name}-build.log")
        plain = out / "player-draw-check.exe"
        original = out / "player-draw-original-check.exe"
        native_logs = [
            checked(
                [str(plain), windows(contexts[14])],
                out / f"native-{repeat}.log",
                NATIVE[14],
                "Incomplete native player draw coverage",
            )
            for repeat in range(2)
        ]
        if native_logs[0] != native_logs[1]:
            raise RuntimeError("Fresh native player draw traces differ")
        original_logs: dict[str, list[str]] = {}
        for i, context in contexts.items():
            traces = [
                checked(
                    [str(original), windows(context), windows(BASELINES["original_dll"][0])],
                    out / f"original-{i}-{repeat}.log",
                    NATIVE[i] + "\n" + ORIGINAL[i],
                    f"Original player draw comparison failed in context {i}",
                )
                for repeat in range(2)
            ]
            if traces[0] != traces[1]:
                raise RuntimeError("Fresh original player draw traces differ")
            original_logs[str(i)] = traces
        if before != baseline_hashes() or any(digest(Path(path)) != sha for path, sha in inputs.items()):
            raise RuntimeError("Protected originals or private inputs changed")
        if sources != source_hashes(root, entries):
            raise RuntimeError("Source changed during verification")
        record.update(
            status="passed",
            baseline=before,
            input_sha256=inputs,
            source_sha256=sources,
            native_log_sha256=native_logs,
            original_log_sha256=original_logs,
            executable_sha256={path.name: digest(path) for path in (plain, original)},
            live_actors_per_context=14,
            controlled_draws_per_context=840,
            original_runs_per_process=854,
            original_gx_calls={"14": 368870, "15": 368870, "16": 374596},
            rejection_cases=4,
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print("Original player draw submission passes; private report " + str(report))


if __name__ == "__main__":
    main()
