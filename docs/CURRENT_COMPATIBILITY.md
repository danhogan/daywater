# Current compatibility audit

Audit date: 2026-09-26

## Checkout

- Branch: `nflstreet-foundation`
- Commit: `ce633a0f9f5da963b030736ef7c6c8757d4ea452`
- Commit subject: `Bump version for new release (1.1.0)`
- Working tree on arrival: user-owned untracked onboarding, instruction, prompt,
  and documentation files were present. No tracked source files were modified by
  this audit.
- Project version: `1.1.0`

## Host environment

- Host: Fedora Linux 43 under WSL2, x86_64
- Kernel: `6.18.33.2-microsoft-standard-WSL2`
- CPU / memory: 12 CPUs / approximately 15 GiB RAM and 4 GiB swap
- Docker CLI: `29.8.0`; Docker Compose: `v5.5.1`
- Local Python selected by uv: CPython `3.14.0`
- uv: `0.11.32`

The Dockerfile installs Python 3.13 plus `python3.13-dev`, JDK 21,
Ghidra 12.0.4, and GameCubeLoader. It describes Dolphin 2603a, though the
actual image currently receives 2609 from the PPA.

## Docker and web UI status

`docker compose config --quiet` parsed successfully. It emitted only the
expected warning that `OPENAI_API_KEY` is unset.

The first unprivileged `docker compose up --build -d` attempt stopped with:

```text
permission denied while trying to connect to the docker API at
unix:///var/run/docker.sock
```

With approved daemon access, the unchanged image built and `daywater-web`
started. Inside the container, HTTP probes returned 200 for the web UI on
port 7860 and the Inspect viewer on port 7575. A sandboxed host-side `curl`
could not reach the published ports, so external/browser reachability was not
independently checked.

The build installed Ghidra 12.0.4, GameCubeLoader 1.3.0, compiled
Gekko/Broadway SLEIGH, and installed the locked Python runtime dependencies
under Python 3.13.15. The PPA currently supplied Dolphin package
`2609+dfsg1-0build1 (Ubuntu 24.04)`; the binary reports
`Dolphin [master] 2609`. This differs from the README and Dockerfile's
2603a claim, so savestate compatibility must be checked against the actual
build. The container remains running.

## Tests and checks

| Command | Result | Evidence |
| --- | --- | --- |
| `docker compose config --quiet` | pass | Compose configuration parsed; only missing-key warning. |
| `docker compose up --build -d` | pass with daemon access | Image built and service started. |
| Web UI and Inspect viewer | pass inside container | HTTP 200 from ports 7860 and 7575. |
| `uv run pytest` on host | blocked before collection | With a temporary uv cache, package download failed on DNS lookup (`mypy==2.1.0`). |
| `uv run pre-commit run --all-files` on host | blocked before hooks | Package download failed on DNS lookup (`inspect-ai==0.3.220`). |
| `python -m pytest -q` in container | **1 failed, 4 passed** | `test_shape_mismatch_raises` expects a `ValueError`; the scorer deliberately resizes different-sized candidate frames. |
| `ruff check src tests` in container | fail | 39 existing lint findings. |
| `ruff format --check src tests` in container | fail | 35 existing source files would be reformatted. |
| `python3 -m compileall -q src tests` | pass | Python syntax compilation completed without output. |
| `git diff --check` | pass | No whitespace errors in tracked changes. |

The documented host uv commands could not complete dependency setup because
the sandboxed host could not resolve Python package downloads. The host uses
Python 3.14 while the container uses 3.13; this audit does not establish
whether the host dependency set would work with network access. The full
pre-commit hook set remains unverified.

The present offline suite is `tests/test_scorer.py` (five scorer-math tests);
it does not cover configuration, backend routing, memory helpers, controller
input, or persistence. The failing test disagrees with an explicit resize
path in `src/agent/scorer.py`. Resolving that disagreement requires deciding
the desired behavior; it is not a compatibility/build repair, so the audit
leaves it intact. The pre-commit configuration runs formatting, AST,
lockfile, secret, Ruff, mypy, and `pip-audit` checks.

## Runtime layers found

| Concern | Implementation |
| --- | --- |
| Dolphin lifecycle | `src/core/dolphin/runner.py` and `session.py`; headless `dolphin-emu` launching, cleanup, and per-session user directories. |
| Memory | `src/core/dolphin/memory.py`; Linux `process_vm_readv` with `/proc/<pid>/mem` fallback, GameCube MEM1 discovery, and big-endian typed reads. |
| Controller/input | `src/core/dolphin/input.py`; Dolphin pipe input and timestamped input sequences. Agent-facing tools are in `src/agent/tools/controller.py`. |
| Savestates | `src/core/dolphin/runner.py` validates Dolphin savestate versions; project upload/management is in `src/api/routes/savestates.py`. |
| Screenshots/frame comparison | `src/core/dolphin/runner.py`, `diff.py`, and agent capture/visual-Gecko tools. |
| Ghidra | `src/core/ghidra/analyze.py`, `iso.py`, and `cache.py`; PyGhidra starts an in-process JVM and uses GameCubeLoader. The setup endpoint invokes `src.agent.runner.run_ghidra_init`. |
| Knowledge persistence | `src/core/knowledge/`; atomic project/savestate findings, research documents/indexes, Gecko-code records, and function annotations. |

## Ghidra and Dolphin availability

Neither `ghidraRun` nor `dolphin-emu-nogui` is available on the host PATH.
`DAYWATER_GHIDRA_HOME` is unset on the host. Both Ghidra and Dolphin are
available in the container. Dolphin's binary/version was verified, but no
game was booted and no live memory, controller, frame, or savestate operation
was exercised.

The Ghidra initialization function was exercised in the container. It found
GameCubeLoader and compiled SLEIGH, started the JVM in 2.9 seconds, imported
`DecompInterface` and `ConsoleTaskMonitor`, and emitted
`{"t": "done", "ok": true}`. No sample DOL was present, so actual binary
analysis was skipped. The temporary initialization test process remained
alive after reporting completion and was stopped; the web service was
unaffected.

## Current model-provider architecture

The active agent runtime is Inspect AI; it has not been removed or bypassed.

1. The web UI stores model configurations in
   `sessions/.daywater_settings.json` through `WebSettings` in
   `src/core/config.py`.
2. Activating a configuration exports `INSPECT_EVAL_MODEL` and the provider's
   API-key environment variable. The settings API exposes OpenAI and several
   LiteLLM-supported providers.
3. `src/agent/runner.py:run_agent()` reads `INSPECT_EVAL_MODEL` (default
   `openai/gpt-5.5`) and calls `inspect_ai.eval()` with the generated Inspect
   task.
4. `src/agent/task.py`, `sample_builder.py`, prompts, tools, and scorers build
   that Inspect task. `src/api/app.py` optionally launches `inspect view` on
   port 7575.

There is currently no explicit agent-backend abstraction: the UI configuration
and the execution call both assume Inspect/LiteLLM model naming and API keys.

## Likely Codex extension point

Introduce a small backend interface at the execution boundary in
`src/agent/runner.py`, immediately around the current `inspect_ai.eval()` call.
It should accept the existing prepared task/project context and return the
current result shape (`verdict`, `gecko`, `accuracy`, `log_file`). Keep the
existing Inspect implementation as the default backend.

The web settings model should select a backend separately from model/API-key
configuration. A Codex implementation can then use the official
`openai-codex` SDK and reuse a mounted/host-provided existing auth session
where safe, without forcing Codex credentials into Daywater's persisted model
configuration. Do not select an auth-mount, host-side service, MCP bridge, or
CLI fallback until Docker/auth behavior is tested and the tradeoffs are
documented.

## Concrete blockers

1. The offline scorer suite has one failing test: its shape-mismatch
   expectation conflicts with current resize behavior. The checkout also has
   39 Ruff lint findings and 35 files that fail the formatting check. The
   documented pre-commit run did not complete.
2. Dolphin is currently 2609 in the image, despite documentation claiming
   2603a. Uploaded savestates must match the actual running build.
3. No ISO or matching savestate was supplied. Live memory, input, frame,
   savestate, and Ghidra binary-analysis integration remain unverified.
4. No model API key was configured, so no agent task was run. Host-side uv
   checks were blocked by sandbox DNS resolution.

## Recommended next change

First resolve the scorer test versus resize behavior and bring the existing
checks to a passing baseline in a controlled change. Reconcile the Dolphin
version claim with the installed build before relying on savestates. Then add
focused offline tests for the current configuration and execution-result
boundary, introduce a minimal backend abstraction, and retain Inspect as its
existing implementation. Do not add NFL Street-specific logic or implement
the Codex backend in this audit.

## Changes made

No application, test, dependency, or architecture changes were made. This
report is the only audit artifact updated.
