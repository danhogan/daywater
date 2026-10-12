"""Validate original handle clear/copy calls captured at entry and return."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

from verify_handle_writer import CALLS, check_dol

STATE_SHA256 = "e2faeb8f9481096ff87ae261c8c2943452fbc023cd27510f641808e307603d93"


def verify(path: Path) -> dict[str, object]:
    rows = [json.loads(line) for line in path.read_text().splitlines() if line]
    if not rows or rows[0].get("event") != "ready" or rows[-1].get("event") != "done":
        raise ValueError("capture lacks ready/done boundaries")
    if (rows[0].get("savestate_sha256") != STATE_SHA256 or
            rows[0].get("dolphin") != "2603a" or
            rows[0].get("dol_sha1") != "46e3de854998a0f66c9be8c72a1164d22cd29eb4"):
        raise ValueError("capture reference identity differs")
    pending = None
    completed = 0
    selected_writes = []
    for row in rows[1:-1]:
        if row["event"] in ("event_build_entry", "event_build_prefix",
                            "event_build_return", "event_process_entry",
                            "event_flag_command", "event_flag_written",
                            "event_gate_entry", "event_gate_return",
                            "actor_transfer_entry", "actor_transfer_return"):
            if pending is not None:
                raise ValueError("event build while handle helper is pending")
            continue  # Checked by the event-construction verifier.
        elif row["event"] == "actor_handoff":
            if pending is not None or row["pc"] != 0x8007C7B8:
                raise ValueError("handoff while helper is pending")
            # The separate handoff verifier checks record resolution.
            continue
        elif row["event"] == "handle_call":
            if pending is not None or row["pc"] not in CALLS:
                raise ValueError("unexpected or nested handle call")
            if not 0x80000000 <= row["destination"] <= 0x817FFFFC:
                raise ValueError("destination outside MEM1")
            if row["source"] and not 0x80000000 <= row["source"] <= 0x817FFFFC:
                raise ValueError("source outside MEM1")
            if row["pc"] == CALLS[0] and (row["source"] or row["source_value"]):
                raise ValueError("clear call source must be zero")
            pending = row
        elif row["event"] == "handle_return":
            if (pending is None or row["pc"] != pending["pc"] + 4 or
                    row["destination"] != pending["destination"] or
                    row["after"] != pending["source_value"]):
                raise ValueError("return does not match original helper result")
            selection = pending["selection"]
            if selection["record"] != 0x803B6F00 + 8 + selection["index"] * 0xF4:
                raise ValueError("selected record calculation differs")
            if row["destination"] == selection["record"] + 0xB4:
                if (pending["before"] != selection["handle"] or
                        row["selection"] != {**selection, "handle": row["after"]}):
                    raise ValueError("selected handle does not match destination write")
                selected_writes.append({"caller": f"0x{pending['pc']:08X}",
                                        "before": f"0x{pending['before']:08X}",
                                        "after": f"0x{row['after']:08X}"})
            completed += 1
            pending = None
        else:
            raise ValueError("unexpected trace event")
    done = rows[-1]
    if bool(pending) != done["incomplete_call"] or done["captured"] != completed:
        raise ValueError("miscounted complete or incomplete handle call")
    return {"completed_calls": completed, "selected_record_writes": selected_writes,
            "incomplete_call": pending is not None,
            "dumped_frames_advanced": done["frames"] - rows[0]["frames"]}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dol", required=True, type=Path)
    parser.add_argument("--trace", required=True, type=Path)
    args = parser.parse_args()
    check_dol(args.dol)
    print(json.dumps(verify(args.trace), sort_keys=True))


if __name__ == "__main__":
    main()
