#!/usr/bin/env python3
"""Verify native match GAME PLAY tables against the original match-start loader and live state."""

import json
from pathlib import Path

from export_gear_texture_catalog import export
from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

NATIVE = (
    "GAME rosters: live GAME PLAY table rebuilt byte-identical (23 rows, 14 lineup values) and 14 captured records "
    "from it; 129 controlled team pairs (3048 rows, 3048 player records); 2 unmodeled inputs rejected; digest e750223f."
)
ORIGINAL = "Original GAME roster checks: 130 801E3828/801E3720/801E05A4 loads match every GAME PLAY row byte for byte."
SOURCES = (
    "game_roster.c",
    "game_roster.h",
    "game_roster_check.c",
    "game_roster_original_check.c",
    "db_fixture.h",
    "player_record.c",
    "player_record.h",
    "gear_texture.c",
    "headwear_state.c",
    "archive.c",
    "lzh1.c",
    "actor_model_init.c",
    "actor_model_init_original_check.c",
    "build_windows_game_roster_check.sh",
    "build_windows_game_roster_original_check.sh",
    "export_gear_texture_catalog.py",
    "verify_game_roster.py",
)
BOUNDARY = (
    "Native match GAME PLAY table from the disc STAT PLAY/THCD/TEAM tables, the two selected teams and the menu's "
    "lineup GILP values: original 801E05A4 -> 801E1628/801E78C4 copy with 801E7154 DIGP renumbering scoped to destination team (THCD before "
    "PLAY per team), schema conversion and DIGT = team slot; then complete 800DE000 records from it. The snapshot "
    "supplies selected teams, lineup values, the live GAME table and actor ids. The original runs on emptied GAME "
    "PLAY/THCD tables (the clear before a fresh match is not traced) with the db_teams.dat open marked done; only "
    "synchronization returns. Created (PYTT 5) teams, teams missing from STAT and the lineup writer are not modeled."
)


def checked(command: list[str], log: Path, expected: str, failure: str) -> str:
    """Run one check, require its exact summary output and return the log digest."""
    if run(command, log).replace("\r\n", "\n").strip() != expected:
        raise RuntimeError(failure)
    return str(digest(log))


def main() -> None:
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / "research-temp/nflstreet-game-roster-20261012")
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
        templates = [data / "unpacked/template" / f"{i:05}.bin" for i in (1, 2)]
        contexts = [PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in (14, 15, 16)]
        inputs = {str(path): digest(path) for path in [catalog, *templates, *contexts]}
        for name in ("game_roster", "game_roster_original"):
            run(["bash", str(here / f"build_windows_{name}_check.sh"), str(out)], out / f"{name}-build.log")
        plain = out / "game-roster-check.exe"
        original = out / "game-roster-original-check.exe"
        prefix = [windows(catalog), *[windows(path) for path in templates]]
        native_logs = [
            checked(
                [str(plain), *prefix, windows(contexts[0])],
                out / f"native-{repeat}.log",
                NATIVE,
                "Incomplete native GAME roster coverage",
            )
            for repeat in range(2)
        ]
        if native_logs[0] != native_logs[1]:
            raise RuntimeError("Fresh native GAME roster traces differ")
        original_logs: dict[str, list[str]] = {}
        for i, context in zip((14, 15, 16), contexts, strict=True):
            traces = [
                checked(
                    [str(original), *prefix, windows(context), windows(BASELINES["original_dll"][0])],
                    out / f"original-{i}-{repeat}.log",
                    NATIVE + "\n" + ORIGINAL,
                    "Incomplete original GAME roster comparison",
                )
                for repeat in range(2)
            ]
            if traces[0] != traces[1]:
                raise RuntimeError("Fresh original GAME roster traces differ")
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
            live_game_rows=23,
            live_lineup_values=14,
            live_records=14,
            controlled_team_pairs_per_process=129,
            controlled_rows_per_process=3048,
            original_loads_per_process=130,
            rejected_unmodeled_inputs=2,
            mirrored_team_pairs_per_process=43,
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print("Native GAME rosters pass; private report " + str(report))


if __name__ == "__main__":
    main()
