#!/usr/bin/env python3
"""Verify complete 800DE000 player records against the original and live captured records."""

import json
from pathlib import Path

from export_gear_texture_catalog import export
from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

NATIVE = (
    "Player records: 14 captured records rebuilt byte-identical from poisoned copies (GAME PLAY rows, disc and "
    "live STAT TEAM tables); 4304 controlled records (23 GAME rows x 44 STAT teams x4 side inputs, 256 1AHP "
    "cases; side0/1 1075/3229; 3956 disc-team cases agree); 2 unmodeled inputs rejected; digest 40f22d91."
)
ORIGINAL = (
    "Original player record checks: 4318 800DE6C8 calls on the live GAME/STAT databases match all 104 record bytes."
)
SOURCES = (
    "player_record.c",
    "player_record.h",
    "player_record_check.c",
    "player_record_original_check.c",
    "game_roster.c",
    "game_roster.h",
    "db_fixture.h",
    "gear_texture.c",
    "headwear_state.c",
    "archive.c",
    "lzh1.c",
    "actor_model_init.c",
    "actor_model_init_original_check.c",
    "build_windows_player_record_check.sh",
    "build_windows_player_record_original_check.sh",
    "export_gear_texture_catalog.py",
    "verify_player_record.py",
)
BOUNDARY = (
    "Complete original 800DE000 player records (+00..+64) from match GAME PLAY rows: STAT TEAM colors/LGLT/"
    "PYTT/LDTC, 8014F60C and getter fields, 1AHP, 800DD64C/800DD6CC default color state and the 800DE6C8 side "
    "byte. TEAM, LCGP and GEAR rows come from the on-disc template DB. The snapshot supplies the GAME PLAY table "
    "(match setup is not reconstructed), the save's created STAT TEAM row, actor player ids/slots and side inputs. "
    "The original runs unhooked on the live databases except 801AF944/9CC/57C synchronization. A matching LCGP "
    "row and a missing TEAM row are rejected, not modeled."
)


def checked(command: list[str], log: Path, expected: str, failure: str) -> str:
    """Run one check, require its exact summary output and return the log digest."""
    if run(command, log).replace("\r\n", "\n").strip() != expected:
        raise RuntimeError(failure)
    return str(digest(log))


def main() -> None:
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / "research-temp/nflstreet-player-record-20261012")
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
        for name in ("player_record", "player_record_original"):
            run(["bash", str(here / f"build_windows_{name}_check.sh"), str(out)], out / f"{name}-build.log")
        plain = out / "player-record-check.exe"
        original = out / "player-record-original-check.exe"
        prefix = [windows(catalog), *[windows(path) for path in templates]]
        native_logs = [
            checked(
                [str(plain), *prefix, windows(contexts[0])],
                out / f"native-{repeat}.log",
                NATIVE,
                "Incomplete native player record coverage",
            )
            for repeat in range(2)
        ]
        if native_logs[0] != native_logs[1]:
            raise RuntimeError("Fresh native player record traces differ")
        original_logs: dict[str, list[str]] = {}
        for i, context in zip((14, 15, 16), contexts, strict=True):
            traces = [
                checked(
                    [str(original), *prefix, windows(context), windows(BASELINES["original_dll"][0])],
                    out / f"original-{i}-{repeat}.log",
                    NATIVE + "\n" + ORIGINAL,
                    "Incomplete original player record comparison",
                )
                for repeat in range(2)
            ]
            if traces[0] != traces[1]:
                raise RuntimeError("Fresh original player record traces differ")
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
            live_records=14,
            controlled_records_per_process=4304,
            original_calls_per_process=4318,
            rejected_unmodeled_inputs=2,
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print("Original player records pass; private report " + str(report))


if __name__ == "__main__":
    main()
