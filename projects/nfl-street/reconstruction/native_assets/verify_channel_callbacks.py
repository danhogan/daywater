#!/usr/bin/env python3
"""Task K original-module comparison. Owner-run only; uses its own private report paths."""

import json
import re
import sys
from pathlib import Path

from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

SOURCES = (
    "channel_callbacks.c",
    "channel_callbacks.h",
    "channel_callbacks_native.inc",
    "channel_callbacks_original_check.c",
    "build_channel_callbacks_native.py",
    "build_windows_channel_callbacks_original_check.sh",
    "verify_channel_callbacks.py",
    "actor_channels.c",
    "actor_channels.h",
    "actor_pose.c",
    "actor_pose.h",
    "anim_stream.c",
    "anim_stream.h",
    "check_fixture.h",
    "draw_engine.c",
    "draw_engine.h",
    "gekko_float.h",
    "marks_draw.h",
    "matrix_stack.c",
    "matrix_stack.h",
    "player_draw.h",
    "scenery_draw.h",
    "venue_fixture.h",
)
BOUNDARY = (
    "Whole native 8003A24C on all 14 saved gameplay actors, with native group and event callbacks, against "
    "the original 8003A24C with its group callbacks and 8001532C executing unmodified. The original's "
    "80039990 and 80039A18 return at entry, and 8017BB78 is served from bank files. All RAM outside the "
    "oracle stack plus the 8015E9C8 per-actor locals are compared after every update. A saved-as-is case, "
    "400 deterministic edited cases, and three 600-update soaks per context. No rejected case is accepted "
    "as a pass. Source-derived native basic blocks, with exact single arithmetic through gekko_float.h; "
    "unsupported hardware instructions, absent indirect targets and invalid addresses fail explicitly."
)
ANIMDATA = PRIVATE / "private/nflstreet/GNNE69-r0/unpacked/animdata"
OUT = PRIVATE / "research-temp/nflstreet-clips2-20261015"
SUMMARY = re.compile(
    r"Channel callbacks: 401 cases \(saved \+ 400 edited\), 3 x 600 soak updates; "
    r"2201 full 8003A24C updates of 14 actors match all RAM and pose locals; "
    r"([1-9]\d*) events, ([1-9]\d*) groups, ([1-9]\d*) operand entries; digest ([0-9a-f]{8})\."
)
SOAK = re.compile(
    r"Channel callbacks soak ([0-2]): 600 updates, all RAM compared after each; "
    r"(\d+)/14 active, (\d+)/14 poses changed in last 60, (\d+)/14 keep animating\."
)


def check_output(text: str) -> dict[str, object]:
    lines = text.replace("\r\n", "\n").strip().splitlines()
    if len(lines) != 4:
        raise RuntimeError(f"Incomplete callback comparison: {text}")
    soaks = []
    for i, line in enumerate(lines[:3]):
        match = SOAK.fullmatch(line)
        if not match or int(match[1]) != i or any(int(match[j]) > 14 for j in (2, 3, 4)):
            raise RuntimeError(f"Invalid callback soak: {line}")
        if int(match[4]) > min(int(match[2]), int(match[3])):
            raise RuntimeError(f"Inconsistent callback soak: {line}")
        soaks.append({"case": i, "active": int(match[2]), "changing_poses": int(match[3]), "animating": int(match[4])})
    result = SUMMARY.fullmatch(lines[-1])
    if not result:
        raise RuntimeError(f"Callback comparison failed: {lines[-1]}")
    return {
        "soaks": soaks,
        "events": int(result[1]),
        "groups": int(result[2]),
        "operand_entries": int(result[3]),
        "ram_digest": result[4],
    }


def main() -> None:
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(OUT)
    out.mkdir(parents=True, exist_ok=True)
    report = out / "verification.json"
    # source_hashes resolves paths relative to reconstruction/, not native_assets/.
    entries = tuple("native_assets/" + name for name in SOURCES)
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY}
    try:
        only = [int(arg) for arg in sys.argv[1:]] or [14, 15, 16]
        if len(set(only)) != len(only) or any(i not in (14, 15, 16) for i in only):
            raise ValueError("Contexts must be distinct members of 14, 15, 16")
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        contexts = {i: PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in (14, 15, 16)}
        banks = sorted(ANIMDATA.glob("*.bin"))
        if not banks:
            raise RuntimeError("No private animation banks")
        inputs = {str(path): digest(path) for path in (*contexts.values(), *banks)}
        run(["bash", str(here / "build_windows_channel_callbacks_original_check.sh"), str(out)], out / "build.log")
        exe = out / "channel-callbacks-original-check.exe"
        logs: dict[str, list[str]] = {}
        results: dict[str, object] = {}
        for i in only:
            traces = []
            for repeat in range(2):
                log = out / f"callbacks-{i}-{repeat}.log"
                args = [str(exe), windows(contexts[i]), windows(BASELINES["original_dll"][0]), windows(ANIMDATA)]
                text = run(args, log)
                results[str(i)] = check_output(text)
                traces.append(str(digest(log)))
            if traces[0] != traces[1]:
                raise RuntimeError(f"Fresh callback traces differ in context {i}")
            logs[str(i)] = traces
        if before != baseline_hashes() or any(digest(Path(path)) != sha for path, sha in inputs.items()):
            raise RuntimeError("Protected originals or private inputs changed")
        if sources != source_hashes(root, entries):
            raise RuntimeError("Source changed during verification")
        record.update(
            status="passed" if set(only) == {14, 15, 16} else "partial",
            baseline=before,
            input_sha256=inputs,
            source_sha256=sources,
            log_sha256=logs,
            executable_sha256=digest(exe),
            comparisons=results,
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print("Original channel callbacks pass; private report " + str(report))


if __name__ == "__main__":
    main()
