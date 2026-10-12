"""Compare the RAM and FNIG score readers across retained MEM1 captures.

Keep captures and optional JSONL output outside Git (W: is preferred).
"""

from __future__ import annotations

import argparse
import json
import sys
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from nflstreet_core.observer import Mem1Image, NFLStreetObserver  # noqa: E402


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("capture_dir", type=Path)
    parser.add_argument("--output", type=Path, help="optional JSONL manifest outside Git")
    args = parser.parse_args()
    paths = sorted(args.capture_dir.rglob("*.mem1"))
    if not paths:
        parser.error("no .mem1 captures found")

    counts: Counter[str] = Counter()
    output = args.output.open("w", encoding="utf-8") if args.output else None
    try:
        for path in paths:
            record: dict[str, object] = {"capture": str(path.relative_to(args.capture_dir))}
            try:
                observation = NFLStreetObserver(Mem1Image(path.read_bytes())).observe()
                record.update(
                    score=observation.score.__dict__ if observation.score else None,
                    score_status=observation.score_status,
                    named_score=observation.named_score.__dict__ if observation.named_score else None,
                    named_score_status=observation.named_score_status,
                )
                if observation.score is None or observation.named_score is None:
                    outcome = "unavailable"
                elif observation.score == observation.named_score:
                    outcome = "agree"
                else:
                    outcome = "disagree"
            except (OSError, ValueError) as exc:
                record["error"] = str(exc)
                outcome = "error"
            record["outcome"] = outcome
            counts[outcome] += 1
            if output:
                output.write(json.dumps(record, sort_keys=True) + "\n")
            if outcome != "agree":
                print(json.dumps(record, sort_keys=True))
    finally:
        if output:
            output.close()
    print(json.dumps({"captures": len(paths), "outcomes": dict(counts)}, sort_keys=True))


if __name__ == "__main__":
    main()
