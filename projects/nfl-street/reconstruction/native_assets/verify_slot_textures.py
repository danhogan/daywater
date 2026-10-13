#!/usr/bin/env python3
"""Verify owned, recolored and bound slot textures against the original pipeline and live state."""

import json
from pathlib import Path

from export_gear_texture_catalog import export
from export_slot_material_catalog import export as export_materials
from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

NATIVE = (
    "Selected slot textures: 819 players (14 captured, 1 controlled, 402 roster rows x2 contexts); "
    "140 color entries and 129 pladyncl op lists (722 ops) match live; 350 live slots match layout/size, "
    "350 byte-identical (70 recolored); 85244 cache flushes, 4163 palette compositions; digest 19439b8f.\n"
    "Slot materials: 513 controlled texture sets, 1312 controlled 80164DD8 settings in 64 lists; 20475 texture sets "
    "(20475 images, 20475 palettes), 40950 cache flushes, 552825 bytes left as allocator contents; 350 live sets match "
    "the captured binding table and objects after 80164DD8 (draw-time writes skipped: 1036 register-id, 56 TLUT-name, "
    "315 TLUT TMEM bytes); digest c3bd41c8."
)
ORIGINAL = (
    "Original slot material checks: 513 controlled and 20475 selected 801EBD38 texture sets x2 allocator fills match "
    "every byte, token and flush; 819 selected and 64 controlled 80164DD8 runs match every texture object.\n"
    "Original slot texture checks: 3072 800D9948 sweep cases; 819 800E2958 layouts, 819 800E2AF0 template sets, "
    "819 800E2CAC players (18860 slot loads, 85244 flushes, 4163 800D9C0C compositions); archive acquisition supplied."
)
SOURCES = (
    "slot_textures.c",
    "palette_recolor.c",
    "slot_materials.c",
    "slot_materials.h",
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
    "export_slot_material_catalog.py",
    "verify_slot_textures.py",
)
BOUNDARY = (
    "Owned, recolored selected slot textures: original 800E2958 layout, 800E2AF0 templates, 800E2E48 requests, "
    "the 800E2CAC/800E27F8 loader with 80164CE4 copies and 800D9C0C recoloring compared per slot, including "
    "801F0BD0 flush order; 800D9948 swept over all 8 operations and 3 palette formats. Archive acquisition "
    "(801A824C, 801A75B0) is supplied from fresh native decoding; native-built 8030A898 color table and r13-5B48 "
    "op lists are installed for the original; 8020380C GX FIFO flushes are counted only. Live snapshot: color "
    "table, all pladyncl op lists and all 350 captured-player slot destinations match byte for byte. Color state "
    "is record +4C..+63 from captured team records; slot1 uses the live face request for captured players. Material "
    "binding: original 800E2BE8/801EBD38 texture sets (GXTexObj/GXTlutObj objects, entries, name tables) and the "
    "actor-startup 80164DD8 default settings, compared byte for byte with two allocator fills, plus synthetic MMAP and "
    "settings sweeps. Live sets match except draw-time 801C7AD4 writes (register ids, TLUT name, TMEM offset); the "
    "8017B570 loader pool and model-material hookup 801EBC78 are not modeled."
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
        catalog = data / "native/gear-texture.gtc4"
        catalog_sha = export(catalog)
        materials = data / "native/slot-materials.smc1"
        materials_sha = export_materials(materials)
        archives = [data / "assets" / f"{name}.dat" for name in ("platex", "pladyncl")]
        contexts = [PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in (14, 15, 16)]
        gear = data / "unpacked/template/00002.bin"
        blocks = [data / "unpacked/db_teams" / f"{i:05}.bin" for i in range(1, 33)]
        inputs = {str(path): digest(path) for path in [*archives, *contexts, gear, catalog, materials, *blocks]}
        for name in ("slot_textures", "slot_textures_original"):
            run(["bash", str(here / f"build_windows_{name}_check.sh"), str(out)], out / f"{name}-build.log")
        plain = out / "slot-textures-check.exe"
        original = out / "slot-textures-original-check.exe"
        prefix = [*[windows(path) for path in archives], windows(catalog), windows(gear), windows(data / "unpacked")]
        native_logs = [
            checked(
                [str(plain), *prefix, windows(contexts[0]), windows(materials)],
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
                    [
                        str(original),
                        *prefix,
                        windows(context),
                        windows(materials),
                        windows(BASELINES["original_dll"][0]),
                    ],
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
            material_catalog_sha256=materials_sha,
            input_sha256=inputs,
            source_sha256=sources,
            native_log_sha256=native_logs,
            original_log_sha256=original_logs,
            executable_sha256={path.name: digest(path) for path in (plain, original)},
            players_per_process=819,
            live_color_entries=140,
            live_dynclut_entries=129,
            live_dynclut_ops=722,
            live_slots=350,
            live_identical_slots=350,
            live_recolored_slots=70,
            original_sweep_cases_per_process=3072,
            original_players_per_process=819,
            original_slot_loads_per_process=18860,
            flushes_per_process=85244,
            palette_compositions_per_process=4163,
            material_sets_per_process=20475,
            controlled_material_sets_per_process=513,
            controlled_material_settings_per_process=1312,
            live_material_sets=350,
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print("Original selected slot texture, recolor and material pipeline passes; private report " + str(report))


if __name__ == "__main__":
    main()
