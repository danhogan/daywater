#!/usr/bin/env python3
"""Build selection's independent instruction-ordered C closure without editing shared sources."""

import json
from pathlib import Path
from typing import Any

import build_ai_policy_native as policy_builder

HERE = Path(__file__).resolve().parent
ROOTS = "800f4e54 800f4ee0 800f42a8 800f4488 800f2718 800ef234 800f3264 800f3278".split()


def main() -> None:
    # Reuse the pinned source-construction machinery, never a runtime decoder.
    # The original files stay untouched; a separate namespace owns these roots,
    # output filenames and exported call entry points.
    source = Path(policy_builder.__file__).read_text()
    source = source.replace('"ai_policy.c"', '"play_selection_native.inc"')
    source = source.replace('"ai_policy_sources.json"', '"play_selection_sources.json"')
    # Ghidra's direct call graph omits function-valued arguments, notably
    # 8012F930's comparator/swap callbacks. Follow their actual source references
    # as well as direct calls and saved pointer tables. &LAB references are
    # callback addresses; plain LAB labels inside a body are not functions.
    direct = '        todo += [v["addr"] for v in graph.get(addr, {}).get("callees", [])]'
    source = source.replace(
        direct,
        direct
        + '\n        references = CACHE / "decompiled" / f"{addr}.txt"'
        + "\n        if references.exists():"
        + '\n            todo += [v for v in re.findall(r"(?:FUN_|&LAB_)([0-9a-f]{8})", references.read_text())'
        + "\n                     if 0x80004000 <= int(v, 16) < 0x80244000 and not int(v, 16) & 3]",
    )
    for name in ("init", "host_call", "call", "service"):
        source = source.replace(f"nflstreet_ai_policy_{name}", f"nflstreet_play_native_{name}")
    scope: dict[str, Any] = {"__file__": str(HERE / "build_play_selection_native.py"), "__name__": "selection_builder"}
    exec(compile(source, str(HERE / "build_play_selection_native.py"), "exec"), scope)
    # 801AC670 reads a function at 8039E2B4 + mode*8, then BLRL at
    # 801AC6AC. 801AC8C0/801ACBC8 register modes 0/1 through 801AC6C0;
    # these are RAM callback records, not PTR_FUN tables or direct callees.
    # The failed saved-as-is case reached mode 0 -> 801AC6E0. Include both
    # actual bindings from every capture and their original helper closure.
    # No case edits, oracle hooks or native stand-ins are used to avoid sorting.
    sort_callbacks = {}
    targets = []
    for context in (14, 15, 16):
        capture = Path(f"/mnt/w/Daywater/research-temp/position-neutral-{context}.mem1").read_bytes()
        bindings = {}
        for mode in (0, 1):
            address = 0x8039E2B4 + 8 * mode
            offset = address - 0x80000000
            target = int.from_bytes(capture[offset : offset + 4], "big")
            if not 0x80004000 <= target < 0x80244000 or target & 3:
                raise RuntimeError(f"Invalid sort registration {address:08x} -> {target:08x} in {context}")
            bindings[f"{address:08x}"] = f"{target:08x}"
            targets.append(f"{target:08x}")
        sort_callbacks[str(context)] = bindings
    scope["ROOTS"] = list(dict.fromkeys(scope["ROOTS"] + ROOTS + sorted(set(targets))))
    scope["main"]()
    manifest = HERE / "play_selection_sources.json"
    provenance = json.loads(manifest.read_text())
    provenance["sort_callbacks"] = sort_callbacks
    manifest.write_text(json.dumps(provenance, indent=2, sort_keys=True) + "\n")


if __name__ == "__main__":
    main()
