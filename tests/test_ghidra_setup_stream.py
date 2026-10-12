"""Offline regression test for setup progress events."""

from __future__ import annotations

import asyncio
import json
from pathlib import Path

from src.agent import runner


def test_ghidra_stream_delivers_events_appended_after_connect(
    tmp_path: Path, monkeypatch,
) -> None:
    events_path = tmp_path / "ghidra.jsonl"
    monkeypatch.setattr(runner, "_ghidra_init_events", events_path)

    async def exercise() -> list[str]:
        runner.reset_ghidra_init_events()
        stream = runner.stream_ghidra_init_events()
        await stream.__anext__()  # Initial heartbeat before the first event.
        runner._emit_ghidra_event({"t": "log", "msg": "starting"})
        first = await asyncio.wait_for(stream.__anext__(), timeout=2)
        await stream.__anext__()  # Heartbeat after the first event.
        runner._emit_ghidra_event({"t": "done", "ok": True})
        last = await asyncio.wait_for(stream.__anext__(), timeout=2)
        return [first, last]

    messages = asyncio.run(exercise())
    assert [json.loads(message.removeprefix("data: ").strip())["t"] for message in messages] == [
        "log", "done",
    ]
