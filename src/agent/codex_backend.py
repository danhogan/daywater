"""Codex execution using the existing Daywater task context and tool set."""

from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any

from src.agent.job_spec import EvaluationMethod, GoalType
from src.core.sessions import Project, Task


class CodexAuthenticationError(RuntimeError):
    """No usable Codex login is visible to the Daywater process."""


def codex_mcp_config(project: Project, task: Task) -> dict[str, Any]:
    """Start one task-scoped MCP server with the same container filesystem."""
    return {
        "mcp_servers": {
            "daywater": {
                "command": sys.executable,
                "args": ["-m", "src.agent.codex_tools", project.project_id, task.task_id],
                "cwd": str(Path(__file__).resolve().parents[2]),
                "startup_timeout_sec": 60,
                "tool_timeout_sec": 300,
            }
        }
    }


def codex_input(task: Task, project: Project) -> tuple[str, list[Any]]:
    """Reuse Daywater's prompt and sample content without its Inspect solver."""
    from inspect_ai.model import ContentImage, ContentText
    from openai_codex import LocalImageInput, TextInput

    from src.agent.prompts.builder import build_system_prompt
    from src.agent.sample_builder import build_sample

    spec = task.config.get_job_spec()
    instructions = build_system_prompt(spec).replace("submit()", "finish with a final response")
    instructions += (
        "\n\nFor this Codex run, use the daywater MCP tools for game research and "
        "knowledge changes. Call at least one Daywater tool and report its "
        "result. End with a concise final response; there is no submit tool."
    )
    sample = build_sample(task, project, spec)
    parts: list[Any] = []
    for message in sample.input:
        content = message.content if isinstance(message.content, list) else [message.content]
        for block in content:
            if isinstance(block, ContentText):
                parts.append(TextInput(block.text))
            elif isinstance(block, ContentImage):
                parts.append(LocalImageInput(str(block.image)))
    return instructions, parts


def run_codex_task(
    task: Task,
    project: Project,
    iso_path: Path,
    extract_root: Path,
    *,
    model: str = "",
) -> dict[str, Any]:
    """Run one Codex turn and return Daywater's existing result shape."""
    from openai_codex import Codex, CodexConfig, Sandbox
    from openai_codex.types import TurnStatus

    spec = task.config.get_job_spec()
    errors = spec.validate()
    if errors:
        raise ValueError(f"Invalid job spec: {'; '.join(errors)}")
    if spec.goal_type != GoalType.STATIC_RESEARCH or spec.evaluation != EvaluationMethod.MANUAL_REVIEW:
        raise ValueError("Codex currently supports static research with manual review")
    if spec.needs_savestate and not task.config.savestate_id:
        raise ValueError("Job spec requires a savestate")
    if spec.evaluation == EvaluationMethod.PIXEL_DIFF_MASK:
        if not task.reference_path.exists() or not task.mask_path.exists():
            raise ValueError("Pixel-diff task requires a reference frame and mask")
    if not iso_path.exists():
        raise FileNotFoundError(f"Project ISO not found: {iso_path}")

    instructions, inputs = codex_input(task, project)
    event_size = task.events_path.stat().st_size if task.events_path.exists() else 0

    with Codex(CodexConfig(cwd=str(Path(__file__).resolve().parents[2]))) as codex:
        if codex.account().account is None:
            raise CodexAuthenticationError(
                "Codex is not authenticated in the Daywater container. "
                "Mount an existing Codex auth session and check the Codex setup guide."
            )
        thread = codex.thread_start(
            model=model or None,
            sandbox=Sandbox.read_only,
            developer_instructions=instructions,
            config=codex_mcp_config(project, task),
        )
        task.config.codex_thread_id = thread.id
        task.save()
        task.append_event({"t": "codex_thread", "thread_id": thread.id})

        turn = thread.turn(inputs)
        task.append_event({"t": "codex_progress", "event": "turn/started"})
        result = turn.run()
        task.append_event({"t": "codex_progress", "event": f"turn/{result.status}"})

    if result.status != TurnStatus.completed:
        raise RuntimeError(f"Codex turn {result.status}: {result.error or 'no details'}")

    # Tool calls are emitted by the MCP process into the task's existing event log.
    with task.events_path.open("rb") as events:
        events.seek(event_size)
        tool_used = any(json.loads(line).get("t") == "codex_tool_done" for line in events)
    if not tool_used:
        raise RuntimeError("Codex finished without calling a Daywater tool")

    summary = (result.final_response or "").strip()
    if not summary:
        raise RuntimeError("Codex completed without a final response")
    return {
        "verdict": "PASS",
        "gecko": "",
        "accuracy": 1.0,
        "summary": summary,
        "thread_id": thread.id,
    }
