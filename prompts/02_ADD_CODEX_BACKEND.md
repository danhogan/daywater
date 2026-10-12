# Task 2: Add a Codex-Authenticated Agent Backend

## Context

Daywater currently has an Inspect-based agent implementation whose setup is oriented around an OpenAI API-backed model.

We want an additional backend that can use Codex through the user's existing Codex/ChatGPT authentication instead of requiring an OpenAI API key.

The existing Daywater backend must continue to work.

## Preferred implementation

Use the official Python package:

```bash
pip install openai-codex
```

Prefer the SDK over spawning `codex exec` once per prompt.

The SDK supports reusing existing Codex authentication, starting/resuming threads, turns, streaming, and sandbox configuration.

## Goal

Introduce the smallest clean abstraction necessary so Daywater can choose between:

```text
existing Inspect/API backend
Codex backend
```

Do not rewrite Daywater's reverse-engineering tools.

## Investigation first

Before coding, determine:

- where Daywater constructs/runs its current Inspect agent;
- how Daywater tools are exposed to that agent;
- how progress/events reach the UI;
- how sessions/traces are persisted;
- what execution boundary exists between host and container;
- where Codex authentication state would need to live.

## Authentication constraint

Never commit or copy secret tokens into source control.

If the container cannot safely reuse host Codex auth, evaluate these options in order:

1. controlled read-only/read-write mount of the minimum required Codex auth/config;
2. a host-side Codex process/backend talking to Daywater locally;
3. expose Daywater tools through a local MCP server and connect Codex to that;
4. raw `codex exec` subprocess as a temporary fallback.

Document the selected design and why.

## Functional target

A user should be able to select a Codex backend and run a simple Daywater research task without entering an OpenAI API key for that backend.

The Codex agent must be able to use the Daywater capabilities required by the task, not merely return text.

## Compatibility

Preserve:

- current Docker workflow;
- existing Inspect/OpenAI backend;
- existing task/session storage where practical;
- UI progress reporting where practical;
- upstream mergeability.

## Tests

Add tests for:

- backend selection;
- configuration validation;
- safe behavior when Codex is not authenticated;
- error propagation;
- cancellation if the current architecture supports it.

Use fakes/mocks for offline tests.

## Documentation

Add a concise setup section explaining:

1. how to authenticate Codex;
2. how Daywater discovers/reuses that auth;
3. how to select the Codex backend;
4. container/host limitations;
5. how to verify it with a trivial task.

## Stop condition

Once a trivial Daywater task can successfully invoke a real Daywater tool through the Codex-backed path and return its result, stop.

Do not begin NFL Street reverse engineering in this task.
