#!/usr/bin/env python3
"""Verify joined player surface ownership against current original-verified stages."""
import json
from pathlib import Path
from verify_controller_checks import PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes
from verify_slot_textures import SOURCES as STAGE_SOURCES

EXPECTED = ("Player surface owner: 28 builds (14 captured requests x2 allocator fills) match separate "
            "texture/recolor/bind/configure/hook stages including every flush; 307 failure/reuse rejections "
            "preserve output; 15 semantic record/name builds agree; material allocations balance; close is repeatable.")
SOURCES = ("player_surface.c", "player_surface_check.c", "build_windows_player_surface_check.sh", "verify_player_surface.py",
           "slot_textures.c", "palette_recolor.c", "slot_materials.c", "texture_apply.c", "face_texture.c", "texture.c",
           "gear_texture.c", "headwear_state.c", "archive.c", "lzh1.c")

def main():
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / "research-temp/nflstreet-player-surface-20261013")
    out.mkdir(parents=True, exist_ok=True)
    stage_report = private(PRIVATE / "research-temp/nflstreet-slot-textures-20261012/verification.json")
    record = {"status": "failed", "boundary": "Host ownership joins verified stages. Supplied player records, "
              "color state and captured face requests; semantic named fallback also checked. Original evidence is "
              "in the source-matched slot-stage report. No new original gameplay or GPU drawing claim. "
              "Flush callback side effects cannot be rolled back. External token reservations remain caller-owned."}
    try:
        ref = json.loads(stage_report.read_text())
        stage_entries = tuple("native_assets/" + name for name in STAGE_SOURCES) + ("menus/team_catalog.c",)
        if ref["status"] != "passed" or source_hashes(root, stage_entries) != ref["source_sha256"]:
            raise RuntimeError("Run verify_slot_textures.py: original stage evidence is stale")
        entries = tuple("native_assets/" + name for name in SOURCES) + ("menus/team_catalog.c", "menus/team_identity.c")
        sources = source_hashes(root, entries)
        before = baseline_hashes()
        if before != ref["baseline"]:
            raise RuntimeError("Protected baseline differs from original stage evidence")
        data = PRIVATE / "private/nflstreet/GNNE69-r0"
        catalog = next(Path(path) for path in ref["input_sha256"] if "/native/gear-texture." in path)
        material = next(Path(path) for path in ref["input_sha256"] if path.endswith("slot-materials.smc1"))
        inputs = [data / "assets/platex.dat", data / "assets/pladyncl.dat", catalog,
                  data / "unpacked/template/00002.bin", material, data / "unpacked/db_teams/00001.bin",
                  *(PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in (14, 15, 16))]
        hashes = {str(path): digest(path) for path in inputs}
        if any(ref["input_sha256"].get(path) != value for path, value in hashes.items()):
            raise RuntimeError("Private stage inputs have changed")
        run(["bash", str(here / "build_windows_player_surface_check.sh"), str(out)], out / "build.log")
        exe = out / "player-surface-check.exe"
        logs = {}
        for context in (14, 15, 16):
            traces = []
            for repeat in range(2):
                log = out / f"joined-{context}-{repeat}.log"
                args = [*(windows(path) for path in inputs[:4]), windows(data / "unpacked"),
                        windows(PRIVATE / f"research-temp/position-neutral-{context}.mem1"), windows(material)]
                if run([str(exe), *args], log).strip() != EXPECTED:
                    raise RuntimeError("Incomplete joined ownership comparison")
                traces.append(digest(log))
            if traces[0] != traces[1]:
                raise RuntimeError("Fresh process traces differ")
            logs[str(context)] = traces
        if baseline_hashes() != before or source_hashes(root, entries) != sources or any(digest(Path(p)) != h for p, h in hashes.items()):
            raise RuntimeError("Baseline, source or private input changed during verification")
        record.update(status="passed", baseline=before, source_sha256=sources, input_sha256=hashes,
                      original_stage_report_sha256=digest(stage_report), executable_sha256=digest(exe),
                      logs_sha256=logs, builds_per_process=28, rejections_per_process=307, semantic_builds_per_process=15)
    except Exception as exc:
        record["error"] = str(exc)
        raise
    finally:
        (out / "verification.json").write_text(json.dumps(record, indent=2) + "\n")
    print(EXPECTED)
    print("Three saved contexts repeated twice; original-stage evidence current; protected baseline unchanged.")

if __name__ == "__main__":
    main()
