#!/usr/bin/env python3
"""Verify joined selected-player geometry and surfaces without runtime snapshots."""
import json
import re
from pathlib import Path
from verify_controller_checks import PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

EXPECTED = ("Selected player visuals: 508 roster rows x3 bodies = 1524 geometry/surface assemblies; "
            "0 named-face fallbacks; 13 pre-upload collision/failure guards; occupied outputs reject and close repeats. "
            "Native STAT-to-GAME records and selected STAT geometry, no runtime DOL/module/snapshot.\n"
            "Fresh roster handoff: 480 controlled team-pair builds; source_rows joins each selected STAT identity "
            "to its complete native GAME player record.")

def main():
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / "research-temp/nflstreet-selected-visual-20261013")
    out.mkdir(parents=True, exist_ok=True)
    record = {"status": "failed", "boundary": "Host ownership joins selected geometry and owned surfaces. "
              "508 STAT roster rows, three explicit body variants; source_rows maps each GAME row to its STAT "
              "identity; complete player record and colors follow from native GAME and STAT TEAM/LCGP. Side "
              "input is explicitly 0 in this corpus. This does not verify full actor creation, skeleton "
              "attachment, GPU drawing, menu-driven loading or live match startup."}
    build = here / "build_windows_selected_player_visual_check.sh"
    compiled = re.findall(r'"\$root/([^"\n]+\.c)"', build.read_text())
    entries = tuple("native_assets/" + name for name in compiled) + (
        "native_assets/build_windows_selected_player_visual_check.sh", "native_assets/verify_selected_player_visual.py")
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        refs = {}
        for name in ("nflstreet-face-texture-20261012", "nflstreet-player-surface-20261013", "nflstreet-game-roster-20261012"):
            path = private(PRIVATE / "research-temp" / name / "verification.json")
            r = json.loads(path.read_text())
            if r["status"] != "passed" or r["baseline"] != before:
                raise RuntimeError("Missing passed baseline-matched stage evidence: " + name)
            headers = PRIVATE / "tools/ModernGekko-Template/lib/ModernGekko/include"
            for key, value in r["source_sha256"].items():
                source = headers / key.removeprefix("ModernGekko/") if key.startswith("ModernGekko/") else root / key
                if digest(source) != value:
                    raise RuntimeError("Stale stage evidence: " + str(source))
            refs[name] = digest(path)
        data = PRIVATE / "private/nflstreet/GNNE69-r0"
        assets = [data / "assets" / f"{n}.dat" for n in ("plaface", "plarosterhair", "platex", "plamodel", "plahair", "animdata", "pladyncl")]
        files = [*assets, data / "unpacked/template/00002.bin", data / "native/skeleton-startup.nsc1",
                 data / "native/gear-texture.gtc4", data / "native/slot-materials.smc1", data / "unpacked/template/00001.bin",
                 *(data / "unpacked/db_teams" / f"{i:05}.bin" for i in range(1, 33))]
        inputs = {str(p): digest(p) for p in files}
        run(["bash", str(build), str(out)], out / "build.log")
        exe = out / "selected-player-visual-check.exe"
        args = [*assets[:3], data / "unpacked", *assets[3:6], *files[7:11], assets[6], files[11]]
        logs = []
        for repeat in range(2):
            log = out / f"joined-{repeat}.log"
            if run([str(exe), *(windows(p) for p in args)], log).strip() != EXPECTED:
                raise RuntimeError("Incomplete selected visual corpus")
            logs.append(digest(log))
        if logs[0] != logs[1] or sources != source_hashes(root, entries) or baseline_hashes() != before or any(digest(Path(p)) != h for p, h in inputs.items()):
            raise RuntimeError("Trace, source, input or baseline changed")
        record.update(status="passed", baseline=before, source_sha256=sources, input_sha256=inputs,
                      stage_report_sha256=refs, executable_sha256=digest(exe), logs_sha256=logs,
                      rows=508, assemblies_per_process=1524, face_fallbacks_per_process=0, rejection_guards_per_process=13, roster_builds_per_process=480)
    except Exception as exc:
        record["error"] = str(exc)
        raise
    finally:
        (out / "verification.json").write_text(json.dumps(record, indent=2) + "\n")
    print(EXPECTED)
    print("Fresh Windows processes agree; stage evidence current; protected baseline unchanged.")

if __name__ == "__main__":
    main()
