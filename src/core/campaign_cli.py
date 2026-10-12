"""Inspect a prior runtime PC against an analyzed executable from the shell."""

from __future__ import annotations

import argparse
import json
from dataclasses import asdict

from src.core.campaign import Campaign
from src.core.ghidra.analyze import cache_dir_for_sha1
from src.core.investigation import investigate_address
from src.core.paths import sessions_root


def main() -> None:
    parser = argparse.ArgumentParser(description="Persist a runtime-to-static investigation")
    parser.add_argument("project_id", help="Existing Daywater project ID")
    parser.add_argument("binary_sha1", help="SHA-1 of the analyzed executable")
    parser.add_argument("address", help="GameCube MEM1 address, e.g. 0x80001000")
    parser.add_argument("--writer-pc", action="append", default=[], help="Observed writer PC; repeatable")
    args = parser.parse_args()
    project_root = sessions_root() / args.project_id
    if not (project_root / "config.json").exists():
        parser.error(f"Daywater project not found: {args.project_id}")
    cache_dir = cache_dir_for_sha1(args.binary_sha1)
    if not (cache_dir / "functions.json").exists():
        parser.error(f"Ghidra analysis not found for {args.binary_sha1}")
    result = investigate_address(Campaign(project_root, args.binary_sha1), cache_dir,
                                 int(args.address, 0), writer_pcs=[int(pc, 0) for pc in args.writer_pc])
    print(json.dumps(asdict(result), indent=2))


if __name__ == "__main__":
    main()
