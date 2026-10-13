#!/usr/bin/env python3
"""Render the captured players from the verified draw submission and check its GPU-state and texture evidence."""

import json
from pathlib import Path

from verify_controller_checks import PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_draw import SOURCES as DRAW_SOURCES
from verify_player_startup import source_hashes

TEXTURES = (
    "Texture decode: 337 captured slot textures (4221952 pixels) decode identically from their GX texture/TLUT "
    "objects and their MMAP files."
)
STATE = (
    "Captured GPU state: 2/2 recorded one-stage player draw states are produced by the native calls; 2/2 native "
    "one-stage states were recorded."
)
RENDER = {
    14: "4955 GX calls, 340 display lists, 5480 primitives, 19631 triangles (9955 culled, 0 clipped), 254 decoded "
    "textures, 30685 shaded pixels, 22295 covered; digest fd5f2385.",
    15: "4955 GX calls, 340 display lists, 5480 primitives, 19631 triangles (9924 culled, 0 clipped), 254 decoded "
    "textures, 30180 shaded pixels, 21973 covered; digest db7ee221.",
    16: "4947 GX calls, 340 display lists, 5456 primitives, 19572 triangles (9909 culled, 0 clipped), 252 decoded "
    "textures, 30139 shaded pixels, 21608 covered; digest ac372641.",
}
SOURCES = (
    "gx_render.c",
    "gx_render.h",
    "player_render_check.c",
    "player_draw.c",
    "player_draw.h",
    "texture.c",
    "texture.h",
    "build_windows_player_render_check.sh",
    "verify_player_render.py",
)
BOUNDARY = (
    "Software GX execution of the verified native 80161E10 submission for the 14 captured players of three saved "
    "contexts: display lists with indexed matrix loads, transform, one-channel lighting, clipping, culling, mip "
    "sampling, integer TEV, alpha compare, depth and blending. Frame state comes from the saved GX shadow "
    "(projection, viewport, color mode, konst selections) and the 801C769C frame start (depth mode, texgens). "
    "Checked: every recorded one-stage player draw state of the 2026-10-08 original frame capture equals the SDK "
    "encoding of the native calls and vice versa; all non-empty captured slot textures decode identically through "
    "their GX objects and through the independent MMAP reader; renders repeat exactly. Inspection images only: "
    "exact original pixels, field/scene drawing and fresh selected actors are not claimed."
)


def main() -> None:
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / "research-temp/nflstreet-player-render-20261013")
    out.mkdir(parents=True, exist_ok=True)
    report = out / "verification.json"
    draw_report = private(PRIVATE / "research-temp/nflstreet-player-draw-20261013/verification.json")
    entries = tuple("native_assets/" + name for name in SOURCES)
    draw_entries = tuple("native_assets/" + name for name in DRAW_SOURCES)
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY}
    try:
        reference = json.loads(draw_report.read_text())
        if reference["status"] != "passed" or source_hashes(root, draw_entries) != reference["source_sha256"]:
            raise RuntimeError("The player draw submission report is not passed for the current sources")
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        contexts = {i: PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in (14, 15, 16)}
        profile = PRIVATE / "research-temp/nflstreet-render-capture-20261008/reference-player-profile.bin"
        inputs = {str(path): digest(path) for path in (*contexts.values(), profile)}
        run(["bash", str(here / "build_windows_player_render_check.sh"), str(out)], out / "player_render-build.log")
        exe = out / "player-render-check.exe"
        logs: dict[str, list[str]] = {}
        images: dict[str, str] = {}
        for i, context in contexts.items():
            image = out / f"players-{i}.bmp"
            expected = f"Player render: 14 actors, {RENDER[i]}\n{TEXTURES}\n{STATE}"
            traces = []
            for repeat in range(2):
                log = out / f"render-{i}-{repeat}.log"
                text = run([str(exe), windows(context), windows(image), windows(profile)], log)
                if text.replace("\r\n", "\n").strip() != expected:
                    raise RuntimeError(f"Player render check failed in context {i}")
                traces.append(str(digest(log)))
            if traces[0] != traces[1]:
                raise RuntimeError("Fresh player render traces differ")
            logs[str(i)] = traces
            images[image.name] = str(digest(image))
        if before != baseline_hashes() or any(digest(Path(path)) != sha for path, sha in inputs.items()):
            raise RuntimeError("Protected originals or private inputs changed")
        if sources != source_hashes(root, entries):
            raise RuntimeError("Source changed during verification")
        record.update(
            status="passed",
            baseline=before,
            input_sha256=inputs,
            source_sha256=sources,
            draw_report_sha256=digest(draw_report),
            log_sha256=logs,
            image_sha256=images,
            executable_sha256=digest(exe),
            players_per_context=14,
            cross_decoded_textures=337,
            captured_states_matched=2,
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print("Native captured-player render passes; private report " + str(report))


if __name__ == "__main__":
    main()
