"""Stage or revert the guarded 0x801FFEBC candidate in private generated C."""

from __future__ import annotations

import argparse
import shutil
from pathlib import Path

from stage_product_fastpath import replace_once


SOURCE = Path(__file__).resolve().parents[1] / "reconstruction/weighted_pair_reduction"
CHUNK_NAME = "chunk_0128_text1_801FF480.c"
ORIGINAL_INCLUDE = b'#include "../generated.h"\r\n'
INCLUDE = ORIGINAL_INCLUDE + b'#include "weighted_fastpath.inc"\r\n'
ORIGINAL_ENTRY = b"label_801FFEBC:\r\n"
ENTRY = (
    ORIGINAL_ENTRY +
    b"    if (nflstreet_weighted_pair_cpu_candidate(ctx))\r\n"
    b"        goto return_dispatch_801FF480;\r\n"
)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("apply", "revert"))
    parser.add_argument("generated", type=Path)
    args = parser.parse_args()
    generated = args.generated.resolve()
    chunk = generated / "chunks" / CHUNK_NAME
    staged_c = generated / "chunks/weighted_fastpath.inc"
    staged_h = generated / "chunks/full_state_candidate.h"
    if not chunk.is_file() or not (generated / "generated.h").is_file():
        parser.error("expected known generated GNNE69 C module layout")
    data = chunk.read_bytes()
    if args.action == "apply":
        if staged_c.exists() or staged_h.exists():
            parser.error("candidate files already staged")
        data = replace_once(data, ORIGINAL_INCLUDE, INCLUDE)
        data = replace_once(data, ORIGINAL_ENTRY, ENTRY)
        shutil.copyfile(SOURCE / "full_state_candidate.c", staged_c)
        shutil.copyfile(SOURCE / "full_state_candidate.h", staged_h)
    else:
        if (not staged_c.is_file() or not staged_h.is_file() or
                staged_c.read_bytes() != (SOURCE / "full_state_candidate.c").read_bytes() or
                staged_h.read_bytes() != (SOURCE / "full_state_candidate.h").read_bytes()):
            parser.error("staged candidate files missing or changed")
        data = replace_once(data, INCLUDE, ORIGINAL_INCLUDE)
        data = replace_once(data, ENTRY, ORIGINAL_ENTRY)
    chunk.write_bytes(data)
    if args.action == "revert":
        staged_c.unlink()
        staged_h.unlink()
    print(f"{args.action}: {chunk}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
