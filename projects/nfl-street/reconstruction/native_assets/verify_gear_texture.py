#!/usr/bin/env python3
"""Verify selected gear texture plans against original 800DE3B8 and its record/key routines."""

import json
from pathlib import Path

from export_gear_texture_catalog import export
from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

NATIVE = (
    "Selected gear texture plans: 402 roster rows x2 captured team contexts, 14 captured records, "
    "112 controlled variants; 930 plans, 282 GEAR records (124 joined), 16429 slot indices across 23 slots "
    "(slots16/17/18: 664/535/98), 3889 palettes, 930 slot0 fallbacks, 26538 color requests, "
    "930 records +65 set; 336 live request slots match; digest 47d891f6."
)
ORIGINAL = (
    "Original gear texture checks: 282 GEAR records x2 fills, 402 player-field rows (12462 fields), "
    "1024 selector kinds, 65536 fallback codes, 930 complete 800DE3B8 plans (24180 slot writes, "
    "26538 color requests, 11160 queries)."
)
SOURCES = (
    "gear_texture.c",
    "gear_texture_check.c",
    "gear_texture_original_check.c",
    "headwear_state.c",
    "archive.c",
    "lzh1.c",
    "actor_model_init.c",
    "actor_model_init_original_check.c",
    "build_windows_gear_texture_check.sh",
    "build_windows_gear_texture_original_check.sh",
    "export_gear_texture_catalog.py",
    "verify_gear_texture.py",
)
BOUNDARY = (
    "Original 800DE3B8 selected gear texture plan: 12 GRTI/GEID joins, 800DD720 PLATEX/PLADYNCLUT keys, "
    "800DDB10/800DDD2C/800DDBE8 color and fixed-index paths, 800DD974 default keys, 800E3218 slot0 fallback "
    "and the +65 byte. Original 8014B918 GEAR records and 8014F60C/800DE000 player-record fields compared. "
    "801B1AC8 table search is supplied from fresh rows; each item is filled by original 801C4388/801B94C8 "
    "with the 801BFFDC type-0 terminator. 800E217C color, 800E2E48 request-record and 800E3020 state effects "
    "are recorded arguments, not executed. Plans for the 14 captured records also equal the live 800E2E48 "
    "records in the same snapshot (336 non-face slots). Team bytes +3C..+3F/+4C..+67 come from captured records; "
    "controlled variants are not roster claims."
)


def checked(command: list[str], log: Path, expected: str, failure: str) -> str:
    """Run one check, require its exact summary output and return the log digest."""
    if run(command, log).replace("\r\n", "\n").strip() != expected:
        raise RuntimeError(failure)
    return str(digest(log))


def main() -> None:
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / "research-temp/nflstreet-gear-texture-plan-20261012")
    out.mkdir(parents=True, exist_ok=True)
    data = private(PRIVATE / "private/nflstreet/GNNE69-r0")
    report = out / "verification.json"
    entries = tuple("native_assets/" + name for name in SOURCES) + ("menus/team_catalog.c",)
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY}
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        catalog = data / "native/gear-texture.gtc4"
        catalog_sha = export(catalog)
        archives = [data / "assets" / f"{name}.dat" for name in ("platex", "pladyncl")]
        contexts = [PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in (14, 15, 16)]
        gear = data / "unpacked/template/00002.bin"
        blocks = [data / "unpacked/db_teams" / f"{i:05}.bin" for i in range(1, 33)]
        inputs = {str(path): digest(path) for path in [*archives, *contexts, gear, catalog, *blocks]}
        for name in ("gear_texture", "gear_texture_original"):
            run(["bash", str(here / f"build_windows_{name}_check.sh"), str(out)], out / f"{name}-build.log")
        plain = out / "gear-texture-check.exe"
        original = out / "gear-texture-original-check.exe"
        prefix = [*[windows(path) for path in archives], windows(catalog), windows(gear), windows(data / "unpacked")]
        native_logs = [
            checked(
                [str(plain), *prefix, windows(contexts[0])],
                out / f"native-{repeat}.log",
                NATIVE,
                "Incomplete native gear texture coverage",
            )
            for repeat in range(2)
        ]
        if native_logs[0] != native_logs[1]:
            raise RuntimeError("Fresh native gear texture traces differ")
        original_logs: dict[str, list[str]] = {}
        for i, context in zip((14, 15, 16), contexts, strict=True):
            traces = [
                checked(
                    [str(original), *prefix, windows(context), windows(BASELINES["original_dll"][0])],
                    out / f"original-{i}-{repeat}.log",
                    NATIVE + "\n" + ORIGINAL,
                    "Incomplete original gear texture comparison",
                )
                for repeat in range(2)
            ]
            if traces[0] != traces[1]:
                raise RuntimeError("Fresh original gear texture traces differ")
            original_logs[str(i)] = traces
        if before != baseline_hashes() or any(digest(Path(path)) != sha for path, sha in inputs.items()):
            raise RuntimeError("Protected originals or private inputs changed")
        if sources != source_hashes(root, entries):
            raise RuntimeError("Source changed during verification")
        record.update(
            status="passed",
            baseline=before,
            catalog_sha256=catalog_sha,
            input_sha256=inputs,
            source_sha256=sources,
            native_log_sha256=native_logs,
            original_log_sha256=original_logs,
            executable_sha256={path.name: digest(path) for path in (plain, original)},
            live_request_slots=336,
            roster_rows=402,
            plans_per_process=930,
            gear_records=282,
            joined_gear_records=124,
            original_gear_record_cases_per_process=564,
            player_rows_per_process=402,
            player_fields_per_process=12462,
            selector_kind_cases_per_process=1024,
            fallback_codes_per_process=65536,
            original_plans_per_process=930,
            slot_writes_per_process=24180,
            color_requests_per_process=26538,
            gear_queries_per_process=11160,
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print("Original selected gear texture plans pass; private report " + str(report))


if __name__ == "__main__":
    main()
