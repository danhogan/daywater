"""Agent access to the project campaign's runtime-to-static bridge."""

from __future__ import annotations

import json
from dataclasses import asdict
from pathlib import Path

from inspect_ai.tool import Tool, tool

from src.agent.state import NO_BINARY_SELECTED_MSG, current_cache_dir
from src.core.campaign import Campaign
from src.core.investigation import investigate_address as bridge


@tool
def investigate_address(project_root: Path, session: object | None = None) -> Tool:
    async def execute(address: str, writer_pcs: list[str] | None = None,
                      watch_seconds: float = 3.0) -> str:
        """Read a MEM1 address, collect writer PCs, and link them to Ghidra functions.

        Args:
            address: GameCube address such as 0x80ABCDEF.
            writer_pcs: Optional PCs from a previous watchpoint observation.
            watch_seconds: Duration for live writer capture.
        """
        cache_dir = current_cache_dir()
        if cache_dir is None:
            return NO_BINARY_SELECTED_MSG
        try:
            campaign = Campaign(project_root, cache_dir.name)
            result = bridge(campaign, cache_dir, int(address, 0), session=session,
                            writer_pcs=[int(pc, 0) for pc in writer_pcs or []],
                            watch_seconds=watch_seconds)
            return json.dumps(asdict(result), indent=2)
        except (ValueError, KeyError, FileNotFoundError, RuntimeError) as exc:
            return f"Investigation failed: {exc}"

    return execute
