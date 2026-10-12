# Task 1: Audit Daywater Before Modifying It

You are working in a fork of:

https://github.com/MaukWM/daywater

Do not redesign the project yet.

## Goal

Determine whether the current Daywater checkout is healthy enough to use as the base for an NFL Street reverse-engineering project.

## Requirements

1. Read the repository structure and README.
2. Identify the current agent/model integration points.
3. Identify the Dolphin, Ghidra, memory, controller/input, savestate, screenshot, and knowledge-persistence layers.
4. Build/start the existing Docker development environment without changing architecture.
5. Run the project's documented tests/checks.
6. Record failures before fixing them.
7. Fix only clear compatibility/build defects required to establish a baseline.
8. Do not mass-upgrade dependencies.
9. Do not remove Inspect AI.
10. Do not add NFL Street-specific code.

## Deliverable

Create:

```text
docs/CURRENT_COMPATIBILITY.md
```

It must contain:

- current commit/hash;
- host environment;
- Docker status;
- test/check results;
- whether the web UI starts;
- whether Ghidra initializes;
- whether Dolphin is available;
- current model-provider architecture;
- likely extension point for a Codex backend;
- concrete blockers;
- recommended next change.

If code changes were required, keep them minimal and explain each one.

Stop after the baseline/audit. Do not implement the Codex backend in this task.
