#!/usr/bin/env python3
"""Build selection's independent instruction-ordered C closure without editing shared sources."""

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
    scope["ROOTS"] = list(dict.fromkeys(scope["ROOTS"] + ROOTS))
    scope["main"]()


if __name__ == "__main__":
    main()
