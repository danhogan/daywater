"""Offline backend selection and tool-bridge checks; no emulator or login."""

from __future__ import annotations

import asyncio
from pathlib import Path
from types import SimpleNamespace

import pytest

from src.agent.presets import get_preset
from src.core.config import WebSettings, validate_agent_config
from src.core.sessions import Project, ProjectConfig, Task, TaskConfig, TaskState


def project_task(root: Path) -> tuple[Project, Task]:
    project_root = root / "project"
    task_root = project_root / "tasks" / "task"
    task_root.mkdir(parents=True)
    project = Project(project_root, ProjectConfig(project_id="project", game_id="TEST"))
    project.save()
    project.iso_path.write_bytes(b"synthetic-test-disc")
    task = Task(
        task_root,
        TaskConfig(
            task_id="task",
            state=TaskState.READY,
            job_spec=get_preset("research").to_dict(),
        ),
    )
    task.save()
    return project, task


def test_backend_config_validation_and_legacy_default(tmp_path: Path) -> None:
    settings = WebSettings(tmp_path / "settings.json")
    settings.add_model_config(name="Existing", model="openai/gpt-5.5")
    assert settings.active_backend() == "inspect"
    settings.add_model_config(name="Codex", model="", backend="codex")
    assert settings.set_active_config(settings.model_configs()[-1]["id"])
    assert settings.active_backend() == "codex"
    assert validate_agent_config("codex", "", "secret", "") is not None
    assert validate_agent_config("codex", "openai/gpt-5.5", "", "") is not None
    assert validate_agent_config("unknown", "", "", "") is not None
    assert validate_agent_config("inspect", "", "", "") is not None


def test_codex_tool_bridge_calls_real_daywater_tool(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    from src.agent.codex_tools import build_server
    from src.agent.state import sample_store

    project, task = project_task(tmp_path)
    old_store = sample_store._store
    try:
        server, cleanup = build_server(task, project)
        names = {tool.name for tool in server._tool_manager.list_tools()}
        assert {"list_research", "list_findings", "entry_points"} <= names
        result = asyncio.run(server.call_tool("list_research", {}))
        assert result
        assert '"codex_tool_done"' in task.events_path.read_text()
        cleanup()
    finally:
        sample_store._store = old_store


def test_codex_rejects_missing_login(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    import openai_codex

    from src.agent import codex_backend

    project, task = project_task(tmp_path)
    monkeypatch.setattr(codex_backend, "codex_input", lambda *_: ("instructions", ["prompt"]))

    class FakeCodex:
        def __init__(self, _config: object) -> None: ...

        def __enter__(self) -> FakeCodex:
            return self

        def __exit__(self, *_args: object) -> None: ...

        def account(self) -> SimpleNamespace:
            return SimpleNamespace(account=None)

    monkeypatch.setattr(openai_codex, "Codex", FakeCodex)
    with pytest.raises(codex_backend.CodexAuthenticationError, match="not authenticated"):
        codex_backend.run_codex_task(task, project, project.iso_path, tmp_path, model="")


def test_runner_selects_codex_and_propagates_error(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    from src.agent import codex_backend, runner
    from src.core import config

    settings = WebSettings(tmp_path / "settings.json")
    settings.add_model_config(name="Codex", model="", backend="codex")
    settings.save()
    monkeypatch.setattr(config, "web_settings", settings)
    project, task = project_task(tmp_path)

    def fail(*_args: object, **_kwargs: object) -> None:
        raise RuntimeError("Codex test failure")

    monkeypatch.setattr(codex_backend, "run_codex_task", fail)
    result = asyncio.run(runner.run_agent(task, project))
    assert result["verdict"] == "FAILED"
    assert result["error"] == "Codex test failure"
    assert task.state == TaskState.FAILED
    assert task.config.agent_backend == "codex"
    assert task.config.result_summary == "Codex test failure"


def test_runner_keeps_inspect_path(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    import inspect_ai

    from src.agent import runner, sample_builder
    from src.core import config

    settings = WebSettings(tmp_path / "settings.json")
    settings.add_model_config(name="Inspect", model="openai/gpt-5.5")
    settings.save()
    monkeypatch.setattr(config, "web_settings", settings)
    project, task = project_task(tmp_path)

    monkeypatch.setattr(
        sample_builder,
        "build_task_from_project_task",
        lambda *_args: (object(), lambda: None),
    )
    score = SimpleNamespace(metrics={"accuracy": SimpleNamespace(value=1.0)})
    evaluation = SimpleNamespace(
        results=SimpleNamespace(scores=[score]),
        samples=[],
        location=tmp_path / "inspect.log",
    )
    monkeypatch.setattr(inspect_ai, "eval", lambda *_args, **_kwargs: [evaluation])
    result = asyncio.run(runner.run_agent(task, project))
    assert result["verdict"] == "PASS"
    assert task.state == TaskState.DONE
    assert task.config.agent_backend == "inspect"
