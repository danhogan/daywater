#!/usr/bin/env python3
"""Verify the native gameplay-actor animation update (8003A24C without 80039990/80039A18) and the bank-file stream
service against the original, in three saved contexts."""

import json
import sys
from pathlib import Path

from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

EXPECTED = {
    14: (
        "Actor channels soak: 600 native updates of all 14 actors from the saved state without providers;"
        " 9 actors keep active channels, 9 poses changed over the last 60.\nActor channels: 14 streamed ba"
        "nks mapped to their files; 276 saved stream cache slots equal the service's decoded frames (0 sl"
        "ots the saved frame tables no longer name); 255 runs of 8003A24C (dt 1/2-or-0.5/random 289/211/1"
        "00) match all RAM, the 8015E9C8 frames and 820 event and 4958 group callback calls (219 skipped "
        "operands), 46 cases rejected at kernel_rem_pio2f as the original enters it; original blocks: sle"
        "rp 6068, acosf 1634, rem_pio2f 4, curves 5069/1814/2384, retire 5754, evict 24420, hit 11476, op"
        "erand 381625, 8017428C 53470, resources 23360, mirrored blend 28610, rotate 4902, 80188EF8 1014,"
        " kernel_rem_pio2f 15435; direct service: 263 runs (38 stopped where the cache path stops, 0 wher"
        "e the cache serves a stale saved slot), 13039 frames equal the cache's; digest 74e437e9."
    ),
    15: (
        "Actor channels soak: 600 native updates of all 14 actors from the saved state without providers;"
        " 7 actors keep active channels, 7 poses changed over the last 60.\nActor channels: 14 streamed ba"
        "nks mapped to their files; 266 saved stream cache slots equal the service's decoded frames (10 s"
        "lots the saved frame tables no longer name); 251 runs of 8003A24C (dt 1/2-or-0.5/random 285/212/"
        "103) match all RAM, the 8015E9C8 frames and 862 event and 5115 group callback calls (210 skipped"
        " operands), 50 cases rejected at kernel_rem_pio2f as the original enters it; original blocks: sl"
        "erp 6872, acosf 1841, rem_pio2f 1, curves 4980/1835/2506, retire 5313, evict 26205, hit 10670, o"
        "perand 635585, 8017428C 52250, resources 24576, mirrored blend 28346, rotate 4554, 80188EF8 1148"
        ", kernel_rem_pio2f 17107; direct service: 259 runs (37 stopped where the cache path stops, 5 whe"
        "re the cache serves a stale saved slot), 12940 frames equal the cache's; digest a586c53c."
    ),
    16: (
        "Actor channels soak: 600 native updates of all 14 actors from the saved state without providers;"
        " 7 actors keep active channels, 7 poses changed over the last 60.\nActor channels: 14 streamed ba"
        "nks mapped to their files; 276 saved stream cache slots equal the service's decoded frames (0 sl"
        "ots the saved frame tables no longer name); 259 runs of 8003A24C (dt 1/2-or-0.5/random 287/202/1"
        "11) match all RAM, the 8015E9C8 frames and 935 event and 5154 group callback calls (243 skipped "
        "operands), 42 cases rejected at kernel_rem_pio2f as the original enters it; original blocks: sle"
        "rp 6736, acosf 1803, rem_pio2f 4, curves 4798/1863/2450, retire 5637, evict 26082, hit 8896, ope"
        "rand 318116, 8017428C 49000, resources 23960, mirrored blend 27141, rotate 4650, 80188EF8 1126, "
        "kernel_rem_pio2f 14758; direct service: 254 runs (47 stopped where the cache path stops, 0 where"
        " the cache serves a stale saved slot), 11915 frames equal the cache's; digest 63cfdafc."
    ),
}
SOURCES = (
    "actor_channels.c",
    "actor_channels.h",
    "anim_stream.c",
    "anim_stream.h",
    "gekko_float.h",
    "actor_pose.c",
    "actor_pose.h",
    "player_draw.h",
    "matrix_stack.c",
    "matrix_stack.h",
    "draw_engine.c",
    "draw_engine.h",
    "check_fixture.h",
    "actor_channels_original_check.c",
    "build_windows_actor_channels_original_check.sh",
    "verify_actor_channels.py",
)
BOUNDARY = (
    "Native 8003A24C phases (80178290 manager tick with the curves 80178E78/80178E7C/80178F04/80178F28, 80177A64, "
    "801778C0/80175AF8/801758E0, 80177950; 80173D60 with 80177374/80176E28/80176DD0; 80176650 with 801749E8 and the "
    "stream cache 80179B60/80179830/80179900/801519A8; 800D4CBC/8015E9C8; 80176904 with 8017650C and the quaternion "
    "slerp incl. acosf/sinf/rem_pio2f medium path; 80176B3C; 800D4CE4/8015EC30) on saved RAM, compared as all RAM plus "
    "the 8015E9C8 stack frames and the provider calls against the original module. In the original run 80039990 and "
    "80039A18 return at entry, the event callback (r13-0x6734) and the 801778C0 group callbacks return at entry (event "
    "r3 as the native hook returns), and 8017BB78 is served from the bank files. Cases where the original enters "
    "rem_pio2f's large path (80217548, not reconstructed) are counted with the native stopping there. The direct "
    "stream service is compared with the cache path."
)
ANIMDATA = PRIVATE / "private/nflstreet/GNNE69-r0/unpacked/animdata"


def main() -> None:
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / "research-temp/nflstreet-actor-channels-20261014")
    out.mkdir(parents=True, exist_ok=True)
    report = out / "verification.json"
    entries = tuple("native_assets/" + name for name in SOURCES)
    record: dict[str, object] = {"status": "running", "boundary": BOUNDARY}
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        contexts = {i: PRIVATE / f"research-temp/position-neutral-{i}.mem1" for i in (14, 15, 16)}
        banks = sorted(ANIMDATA.glob("*.bin"))
        inputs = {str(path): digest(path) for path in (*contexts.values(), *banks)}
        run(["bash", str(here / "build_windows_actor_channels_original_check.sh"), str(out)], out / "build.log")
        exe = out / "actor-channels-original-check.exe"
        logs: dict[str, list[str]] = {}
        only = [int(a) for a in sys.argv[1:]] or list(contexts)
        for i in only:
            traces = []
            for repeat in range(2):
                log = out / f"original-{i}-{repeat}.log"
                args = [str(exe), windows(contexts[i]), windows(BASELINES["original_dll"][0]), windows(ANIMDATA)]
                text = run(args, log).replace("\r\n", "\n").strip()
                if text != EXPECTED[i]:
                    raise RuntimeError(f"Actor channels comparison failed in context {i}: {text}")
                traces.append(str(digest(log)))
            if traces[0] != traces[1]:
                raise RuntimeError("Fresh actor channels traces differ")
            logs[str(i)] = traces
        if before != baseline_hashes() or any(digest(Path(path)) != sha for path, sha in inputs.items()):
            raise RuntimeError("Protected originals or private inputs changed")
        if sources != source_hashes(root, entries):
            raise RuntimeError("Source changed during verification")
        record.update(
            status="passed" if len(only) == 3 else "partial",
            baseline=before,
            input_sha256=inputs,
            source_sha256=sources,
            log_sha256=logs,
            executable_sha256=digest(exe),
        )
    except Exception as exc:
        record.update(status="failed", error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + "\n")
    print("Original actor channels pass; private report " + str(report))


if __name__ == "__main__":
    main()
