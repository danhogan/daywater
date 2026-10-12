"""Stage or revert an opt-in live differential probe for 0x801FFF10."""

from __future__ import annotations

import argparse
from pathlib import Path
import shutil

from stage_live_diff_probe import NATIVE, RECONSTRUCTION, changes, replace_once


SPEC = dict(name="projection", chunk="chunk_0128_text1_801FF480.c",
            func="func_801FF480", entry="801FFF10", ret="801FFF60",
            end="801FFF64", stack=0, stack_bytes=0,
            candidate="nflstreet_three_pair_cpu_candidate",
            source=RECONSTRUCTION / "three_pair_projection/full_state_candidate.c",
            header=RECONSTRUCTION / "three_pair_projection/full_state_candidate.h",
            staged="projection_fastpath.inc",
            staged_header="full_state_candidate.h",
            outputs="ctx->gpr[5], 12u, 0u, 0u")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("apply", "revert"))
    parser.add_argument("generated", type=Path)
    args = parser.parse_args()
    generated = args.generated.resolve()
    chunks = generated / "chunks"
    helper = chunks / "live_diff_probe.inc"
    chunk = chunks / SPEC["chunk"]
    staged = chunks / SPEC["staged"]
    header = chunks / SPEC["staged_header"]
    if not chunk.is_file() or not (generated / "generated.h").is_file():
        parser.error("expected known generated GNNE69 C module layout")
    if args.action == "apply":
        if helper.exists() or staged.exists() or header.exists():
            parser.error("probe already staged")
    elif (helper.read_bytes() != (NATIVE / helper.name).read_bytes() or
          staged.read_bytes() != SPEC["source"].read_bytes() or
          header.read_bytes() != SPEC["header"].read_bytes()):
        parser.error("staged probe files missing or modified")
    data = chunk.read_bytes()
    for original, replacement in changes(SPEC):
        data = replace_once(data, original, replacement) if args.action == "apply" else \
               replace_once(data, replacement, original)
    chunk.write_bytes(data)
    if args.action == "apply":
        shutil.copyfile(SPEC["source"], staged)
        shutil.copyfile(SPEC["header"], header)
        shutil.copyfile(NATIVE / helper.name, helper)
    else:
        staged.unlink()
        header.unlink()
        helper.unlink()
    print(f"{args.action}: live 0x801FFF10 probe")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
