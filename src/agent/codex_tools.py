"""Task-scoped stdio MCP adapter for Daywater's existing Inspect tools."""

from __future__ import annotations

import functools
import signal
import sys
from collections.abc import Callable
from typing import Any

from mcp.server.fastmcp import FastMCP

from src.agent.sample_builder import _setup_session
from src.agent.state import sample_store
from src.agent.tools.builder import build_tools
from src.core.paths import cache_root, sessions_root
from src.core.sessions import Project, ProjectStore, Task


class _MemoryStore:
    """The small get/set surface used by SampleStore outside Inspect."""

    def __init__(self) -> None:
        self.data: dict[str, Any] = {}

    def get(self, key: str, default: Any = None) -> Any:
        return self.data.get(key, default)

    def set(self, key: str, value: Any) -> None:
        self.data[key] = value


def build_server(task: Task, project: Project) -> tuple[FastMCP, Callable[[], None]]:
    """Register only the tools allowed by this task's JobSpec."""
    spec = task.config.get_job_spec()
    errors = spec.validate()
    if errors:
        raise ValueError(f"Invalid job spec: {'; '.join(errors)}")

    local_store = _MemoryStore()
    sample_store._store = lambda: local_store  # type: ignore[method-assign]

    iso_path = project.iso_path.resolve()
    extract_root = cache_root() / "extracted" / iso_path.stem
    session, session_ref, cleanup = _setup_session(spec, task, project, iso_path)
    savestate = project.get_savestate(task.config.savestate_id) if task.config.savestate_id else None
    try:
        tools = build_tools(
            spec,
            project_root=project.root,
            iso_path=iso_path,
            extract_root=extract_root,
            session=session_ref or session,
            savestate_root=savestate.root if savestate else None,
            task_root=task.root,
            task_id=task.task_id,
            task=task,
            project=project,
            savestate_path=savestate.savestate_path if savestate else None,
            winddown=False,
        )
    except BaseException:
        cleanup()
        raise

    server = FastMCP("daywater")
    calls = 0
    for tool in tools:
        info = getattr(tool, "__registry_info__", None)
        name = getattr(info, "name", None) or tool.__name__

        @functools.wraps(tool)
        async def invoke(*args: Any, _tool: Any = tool, _name: str = name, **kwargs: Any) -> Any:
            nonlocal calls
            if calls >= spec.max_tool_calls:
                raise RuntimeError(f"Daywater tool budget exhausted ({spec.max_tool_calls} calls)")
            calls += 1
            task.append_event({"t": "codex_tool_start", "name": _name})
            try:
                value = await _tool(*args, **kwargs)
            except Exception as exc:
                task.append_event({"t": "codex_tool_error", "name": _name, "error": str(exc)})
                raise
            task.append_event({"t": "codex_tool_done", "name": _name})
            return value

        server.add_tool(invoke, name=name, description=tool.__doc__)
    return server, cleanup


def main() -> None:
    if len(sys.argv) != 3:
        raise SystemExit("Usage: python -m src.agent.codex_tools PROJECT_ID TASK_ID")
    project_id, task_id = sys.argv[1:]
    project = ProjectStore(sessions_root()).get(project_id)
    if project is None:
        raise SystemExit(f"Project not found: {project_id}")
    task = project.get_task(task_id)
    if task is None:
        raise SystemExit(f"Task not found: {task_id}")

    server, cleanup = build_server(task, project)

    def stop(_signal: int, _frame: Any) -> None:
        raise SystemExit(0)

    signal.signal(signal.SIGTERM, stop)
    try:
        server.run(transport="stdio")
    finally:
        cleanup()


if __name__ == "__main__":
    main()
