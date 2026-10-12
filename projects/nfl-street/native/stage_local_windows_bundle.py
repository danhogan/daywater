"""Stage a local-only Windows static-recompilation bundle from owned inputs.

The output contains proprietary game data. Keep it on private bulk storage;
never commit or distribute it.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
from pathlib import Path


EXPECTED_ISO_SHA1 = "813c4511ba3135059a13de424555647e8252254b"
ROOT = Path("/mnt/w/Daywater")


def sha1(path: Path) -> str:
    digest = hashlib.sha1()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(8 * 1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path,
                        default=ROOT / "research-temp/nflstreet-native-windows-local")
    parser.add_argument("--runner", type=Path,
                        default=ROOT / "tools/ModernGekko-Template/lib/ModernGekko/build-win64/moderngekko-run.exe")
    parser.add_argument("--game", type=Path,
                        default=ROOT / "research-temp/nflstreet-moderngekko-game")
    parser.add_argument("--iso", type=Path,
                        default=ROOT / "research-temp/nflstreet-GNNE69.iso")
    parser.add_argument("--module", type=Path,
                        default=Path("/mnt/w/nfsmod/GNNE69/a065ae2f4c192124266a56171aa5eed4e2dccb586d32670c5dab2e2ed3ad980c-84b18786e16f1f95/gGNNE69_recomp.dll"))
    parser.add_argument("--savestate", type=Path,
                        default=ROOT / "research-temp/nflstreet-moderngekko-win-vulkan-automation/artifacts/after-field-select.sav")
    parser.add_argument("--early-play-state", type=Path,
                        default=ROOT / "research-temp/nflstreet-moderngekko-win-vulkan-automation/artifacts/early-play.sav")
    parser.add_argument("--user-dir", type=Path,
                        default=ROOT / "research-temp/nflstreet-moderngekko-win-user-idle-801f471c")
    args = parser.parse_args()

    output = args.output.resolve()
    if output.exists():
        parser.error(f"output already exists: {output}")
    inputs = (args.runner, args.runner.parent / "Sys", args.game,
              args.iso, args.module, args.savestate, args.early_play_state,
              args.user_dir / "Config/GCPadNew.ini", args.user_dir / "Config/GFX.ini")
    for path in inputs:
        if not path.exists():
            parser.error(f"missing input: {path}")
    actual_sha1 = sha1(args.iso)
    if actual_sha1 != EXPECTED_ISO_SHA1:
        parser.error(f"ISO SHA-1 mismatch: {actual_sha1}")

    staging = output.with_name(output.name + ".staging")
    if staging.exists():
        parser.error(f"staging directory already exists: {staging}")
    staging.mkdir(parents=True)
    try:
        shutil.copy2(args.runner, staging / "moderngekko-run.exe")
        shutil.copytree(args.runner.parent / "Sys", staging / "Sys")
        shutil.copy2(args.module, staging / "gGNNE69_recomp.dll")
        shutil.copy2(args.iso, staging / "GNNE69.iso")
        shutil.copy2(args.savestate, staging / "match.sav")
        shutil.copy2(args.early_play_state, staging / "early-play.sav")
        shutil.copytree(args.game, staging / "game")
        config = staging / "user/Config"
        config.mkdir(parents=True)
        for filename in ("GCPadNew.ini", "GFX.ini"):
            shutil.copy2(args.user_dir / "Config" / filename, config / filename)
        gfx_path = config / "GFX.ini"
        gfx = gfx_path.read_text(encoding="utf-8")
        gfx, replacements = re.subn(
            r"(?m)^InternalResolution\s*=.*$", "InternalResolution = 1", gfx)
        if replacements != 1:
            raise RuntimeError("expected one InternalResolution setting")
        gfx_path.write_text(gfx, encoding="utf-8")
        (config / "Dolphin.ini").write_text(
            "[Core]\nStaticRecompIdlePC = 2149533468\nCPUCore = 6\n"
            "GFXBackend = Vulkan\n[Display]\nFullscreen = False\n"
            "[DSP]\nBackend = Cubeb\nDSPThread = True\n"
            "[Input]\nBackgroundInput = False\n", encoding="utf-8")
        (staging / "launch.cmd").write_text(
            "@echo off\r\nsetlocal\r\nset \"BUNDLE=%~dp0\"\r\n"
            "set \"MODERNGEKKO_BOOT_IMAGE=%BUNDLE%GNNE69.iso\"\r\n"
            "\"%BUNDLE%moderngekko-run.exe\" --game \"%BUNDLE%game\" "
            "--module \"%BUNDLE%gGNNE69_recomp.dll\" "
            "--user-dir \"%BUNDLE%user\" --graphics Vulkan "
            "--load-state \"%BUNDLE%match.sav\"\r\n"
            "exit /b %ERRORLEVEL%\r\n", encoding="utf-8")
        manifest = {
            "purpose": "Local GNNE69 static-recompilation research bundle; do not redistribute",
            "iso_sha1": actual_sha1,
            "runner_sha1": sha1(staging / "moderngekko-run.exe"),
            "module_sha1": sha1(staging / "gGNNE69_recomp.dll"),
            "savestate_sha1": sha1(staging / "match.sav"),
            "early_play_savestate_sha1": sha1(staging / "early-play.sav"),
            "sys_resource_files": sum(path.is_file() for path in (staging / "Sys").rglob("*")),
            "source": "projects/nfl-street/native/recomp.md",
        }
        (staging / "manifest.json").write_text(
            json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        staging.rename(output)
    except BaseException:
        shutil.rmtree(staging)
        raise
    print(json.dumps({"output": str(output), **manifest}, indent=2))


if __name__ == "__main__":
    main()
