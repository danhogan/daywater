#!/usr/bin/env python3
"""Verify owned selected slot textures against the original 800E2958/800E2AF0/800E2CAC pipeline and live state."""

import json
from pathlib import Path

from export_gear_texture_catalog import export
from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

NATIVE = (
    "Selected slot textures: 819 players (14 captured, 1 controlled, 402 roster rows x2 contexts); "
    "350 live slots match layout/size, 281 byte-identical, 69 differ only in palette record0 pending 800D9C0C; "
    "81081 cache flushes, 4163 palette compositions reported; digest b5175da9."
)
ORIGINAL = (
    "Original slot texture checks: 819 800E2958 layouts, 819 800E2AF0 template sets, 819 800E2CAC players "
    "(18860 slot loads, 81081 flushes, 4163 800D9C0C requests); archive acquisition supplied."
)
SOURCES = (
    "slot_textures.c",
    "slot_textures_check.c",
    "slot_textures_original_check.c",
    "texture_apply.c",
    "face_texture.c",
    "texture.c",
    "gear_texture.c",
    "headwear_state.c",
    "archive.c",
    "lzh1.c",
    "actor_model_init.c",
    "actor_model_init_original_check.c",
    "build_windows_slot_textures_check.sh",
    "build_windows_slot_textures_original_check.sh",
    "export_gear_texture_catalog.py",
    "verify_slot_textures.py",
)
BOUNDARY = (
    "Owned selected slot textures: original 800E2958 layout/sizing, 800E2AF0 template destinations, 800E2E48 "
    "requests and the 800E2CAC/800E27F8 loader with 80164CE4 copies and 801F0BD0 flush order compared per slot. "
    "Archive acquisition (801A824C size, 801A75B0 decode) is supplied from fresh native decoding; 800D9C0C palette "
    "composition is recorded, not executed. The 14 captured players also match live destinations in the snapshot "
    "outside palette record0 data; slot1 uses the live face request for them and the template for roster rows."
)


def checked(command: list[str], log: Path, expected: str, failure: str) -> str:
    """Run one check, require its exact summary output and return the log digest."""
    if run(command, log).replace("\r\n", "\n").strip() != expected:
        raise RuntimeError(failure)
    return str(digest(log))


def main() -> None:
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / "research-temp/nflstreet-slot-textures-20261012")
    out.mkdir(parents=True, exist_ok=True)
    data = private(PRIVATE / "private/nflstreet/GNNE69-r0")
    report = out / "verification.json"
    entries = tuple("native_assets/" + name for name in SOURCES) + ("menus/team_catalog.c",)
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY}
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        catalog = data / "native/gear-texture.gtc3"
        catalog_sha = export(catalog)
        archives = [data / "assets" / f"{name}.dat" for name in ("platex", "pladyncl")]
        contexts = [PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in (14, 15, 16)]
        gear = data / "unpacked/template/00002.bin"
        blocks = [data / "unpacked/db_teams" / f"{i:05}.bin" for i in range(1, 33)]
        inputs = {str(path): digest(path) for path in [*archives, *contexts, gear, catalog, *blocks]}
        for name in ("slot_textures", "slot_textures_original"):
            run(["bash", str(here / f"build_windows_{name}_check.sh"), str(out)], out / f"{name}-build.log")
        plain = out / "slot-textures-check.exe"
        original = out / "slot-textures-original-check.exe"
        prefix = [*[windows(path) for path in archives], windows(catalog), windows(gear), windows(data / "unpacked")]
        native_logs = [
            checked(
                [str(plain), *prefix, windows(contexts[0])],
                out / f"native-{repeat}.log",
                NATIVE,
                "Incomplete native slot texture coverage",
            )
            for repeat in range(2)
        ]
        if native_logs[0] != native_logs[1]:
            raise RuntimeError("Fresh native slot texture traces differ")
        original_logs: dict[str, list[str]] = {}
        for i, context in zip((14, 15, 16), contexts, strict=True):
            traces = [
                checked(
                    [str(original), *prefix, windows(context), windows(BASELINES["original_dll"][0])],
                    out / f"original-{i}-{repeat}.log",
                    NATIVE + "\n" + ORIGINAL,
                    "Incomplete original slot texture comparison",
                )
                for repeat in range(2)
            ]
            if traces[0] != traces[1]:
                raise RuntimeError("Fresh original slot texture traces differ")
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
            players_per_process=819,
            live_slots=350,
            live_identical_slots=281,
            live_palette_pending_slots=69,
            original_players_per_process=819,
            original_slot_loads_per_process=18860,
            flushes_per_process=81081,
            palette_compositions_per_process=4163,
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print("Original selected slot texture pipeline passes; private report " + str(report))


if __name__ == "__main__":
    main()
