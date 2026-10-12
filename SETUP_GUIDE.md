# Setup Guide: Daywater + NFL Street + Codex

This guide assumes Windows 10/11, Docker Desktop with the WSL 2 backend, Git, and a legally obtained NFL Street GameCube image.

The goal is:

```text
Codex authenticated with your ChatGPT/Codex session
                     |
                     v
               Daywater fork
              /             \
          Ghidra           Dolphin
              \             /
               NFL Street RE
                     |
                     v
             nflstreet-core
                     |
                     v
        RuleBased / Jev runtime agent
```

## 1. Install or verify prerequisites

### Docker Desktop

Install Docker Desktop for Windows and use the WSL 2 backend.

Verify:

```powershell
wsl --version
docker --version
docker compose version
```

If WSL is missing or old:

```powershell
wsl --install
wsl --update
```

Start Docker Desktop before continuing.

Daywater's current README says it needs at least 8 GB RAM, recommends 16 GB, and requires a 2 GB shared-memory allocation for the container.

### Git

Verify:

```powershell
git --version
```

### Codex

Use your normal Codex installation and sign in with your ChatGPT account if you have not already.

The Codex Python SDK can reuse an existing Codex authentication session, so the long-term Daywater integration does not need to shell out to an API key flow.

Optional sanity check:

```powershell
codex --version
```

## 2. Fork Daywater

Open:

https://github.com/MaukWM/daywater

Use GitHub's **Fork** button and create a fork in your own account.

Clone your fork:

```powershell
git clone https://github.com/YOUR_GITHUB_USERNAME/daywater.git
cd daywater
```

Keep the original repository as an upstream remote:

```powershell
git remote add upstream https://github.com/MaukWM/daywater.git
git remote -v
```

Expected shape:

```text
origin    -> your fork
upstream  -> MaukWM/daywater
```

Do not begin by upgrading dependencies.

## 3. Create a working branch

```powershell
git checkout -b nflstreet-foundation
```

The first objective is to understand whether the current project builds and where integration changes belong.

## 4. Give your coding agent the project instructions

Copy `AGENTS.md` from this starter bundle into the root of your Daywater fork:

```text
daywater/
  AGENTS.md
  README.md
  pyproject.toml
  ...
```

Then give the agent:

```text
prompts/01_AUDIT_DAYWATER.md
```

The audit should come before architectural changes.

## 5. Baseline the unmodified Daywater project

Daywater currently supports a prebuilt container and a source build.

For development, use the source build:

```powershell
docker compose up --build -d
```

Inspect status:

```powershell
docker compose ps
docker compose logs --tail=200
```

Daywater's web UI is currently exposed at:

```text
http://localhost:7860
```

Its Inspect AI trace viewer is currently exposed at:

```text
http://localhost:7575
```

### Important: stock Daywater currently expects an OpenAI API-backed model

The current setup wizard asks for an API key and model, and the repository README says only OpenAI is supported by that setup today.

If you do **not** want to use API billing, do not spend time configuring the stock model backend. Your next development task is to add a Codex-backed provider that reuses your existing Codex authentication.

It is still useful to prove that:

- the image builds;
- the server starts;
- Ghidra initializes;
- the UI loads;
- the tests run;
- the container layout is understandable.

## 6. Run the existing checks before modifying architecture

Have the agent determine the correct execution context, then run Daywater's documented checks:

```bash
uv run pytest
uv run pre-commit run --all-files
```

Do not blindly "fix" failures by mass-upgrading dependencies.

Record:

- passing/failing tests;
- startup errors;
- Docker-specific errors;
- Ghidra initialization status;
- Dolphin availability;
- any incompatibility caused by current host/container behavior.

## 7. Add Codex as an agent backend

Give the agent:

```text
prompts/02_ADD_CODEX_BACKEND.md
```

### Preferred implementation

Prefer the official Codex Python SDK over spawning `codex exec` for every task.

Current installation:

```bash
pip install openai-codex
```

The SDK can reuse an existing Codex authentication session and supports persistent threads, turn execution, streaming, and sandbox configuration.

The target architecture is roughly:

```text
Daywater task
     |
     v
AgentBackend interface
     |
     +-- Inspect/OpenAI backend   (existing behavior)
     |
     +-- Codex backend            (new)
              |
              v
       existing Codex auth
```

Do not remove Daywater's existing Inspect implementation.

The change should make the model/agent backend replaceable.

### If authentication cannot cross the container boundary cleanly

Do not copy secret tokens into the repository.

Possible approaches, in preferred order:

1. Safely mount or expose the existing Codex auth/config into the development container.
2. Run the Codex-backed agent process on the host while Daywater exposes a narrow local tool interface.
3. Add a local MCP server around Daywater's tool layer and let Codex connect to it.
4. Use raw CLI subprocess integration only as a fallback/prototype.

Have the agent document the security implications of whichever route is chosen.

## 8. Preserve Daywater as a generic GameCube RE platform

Do not turn the fork into `nflstreet-daywater`.

Generic improvements that belong in Daywater:

- Codex backend;
- tool interfaces;
- memory search improvements;
- controller injection improvements;
- better knowledge persistence;
- Ghidra/Dolphin compatibility fixes.

NFL Street-specific information belongs outside the generic core:

```text
nflstreet-core/
  memory/
  state/
  actions/
  docs/
```

Initially this may simply be a sibling repository or a small export directory. Do not over-engineer the plugin boundary before useful NFL Street data exists.

## 9. Prepare NFL Street for Daywater

Use your legally obtained NFL Street GameCube image.

Do not add the ISO to Git.

Daywater's current README says its container uses a specific Dolphin build and that Dolphin savestates are version-dependent. The README currently names Dolphin `2603a`.

For best compatibility, create Daywater test savestates using the same Dolphin build Daywater expects.

Useful initial states:

```text
in_game_offense.state
in_game_defense.state
before_score_change.state
after_score_change.state
```

Keep savestates out of a public repository unless you have verified that redistribution is appropriate.

## 10. First NFL Street task

Once Daywater can run with the desired agent backend, give it:

```text
prompts/03_NFL_STREET_FIRST_TASK.md
```

The first target is deliberately boring:

> Find and verify the live score representation.

Success means Daywater can:

1. observe a known score;
2. narrow RAM candidates;
3. identify the value/address or structure;
4. reproduce the finding from a fresh state;
5. record the result with evidence.

Do not move on merely because one address happened to contain the right number once.

## 11. Next reverse-engineering targets

After score:

1. controlled player X/Y/Z position;
2. controlled-player identity;
3. player array/base structure;
4. team membership;
5. ball possession;
6. all active player coordinates;
7. player velocity;
8. play phase;
9. offense/defense;
10. Gamebreaker/turbo/state-specific meters;
11. useful action/animation state.

Each discovery should be added to the research log with:

- address or pointer chain;
- primitive type;
- endianness if relevant;
- how it was found;
- how it was verified;
- which game version/region it applies to;
- related Ghidra functions;
- confidence.

## 12. Build the runtime layer only after useful state exists

Create a separate project when Daywater has produced enough verified state.

Suggested structure:

```text
nflstreet-core/
  pyproject.toml
  nflstreet/
    memory.py
    state.py
    controller.py
    actions.py
  config/
    nfl_street_us.yaml
  tests/
```

Its public API should eventually look more like:

```python
state = game.observe()

state.score
state.possession
state.controlled_player.position
state.players

controller.move(x, y)
controller.tap("A")
controller.turbo()
controller.juke_left()
```

Raw addresses should never leak into gameplay policy code.

## 13. Start with a deterministic agent

Before Jev, build:

```text
RuleBasedAgent
```

Example logic:

```python
if state.ball_carrier_is_us:
    if state.nearest_defender_distance < threshold:
        return JUKE_LEFT
    return RUN_UPFIELD
```

This is your integration baseline.

If the deterministic agent cannot behave consistently, adding Jev will make debugging harder.

## 14. Add Jev after observation/action interfaces stabilize

Jev should choose semantic actions, not emit 60 controller frames per second.

Good:

```text
RUN_UPFIELD
CUT_LEFT
TURBO
JUKE_RIGHT
PASS_RECEIVER_2
TACKLE
SWITCH_PLAYER
```

A local controller/execution layer should translate those choices into frame-level stick/button timing.

The runtime architecture becomes:

```text
NFL Street RAM
      |
      v
structured observation
      |
      +--> RuleBasedPolicy
      |
      +--> JevPolicy
               |
               v
         semantic action
               |
               v
       local action executor
               |
               v
            Dolphin
```

## 15. Keep research reproducible

Every verified finding should answer:

- Which NFL Street release/region?
- Which Dolphin build?
- Which savestate/test situation?
- What changed in RAM?
- What did not change?
- What function reads/writes it?
- Can it be reproduced after restarting?
- Is it direct, static, pointer-based, or dynamically allocated?

Use `docs/NFL_STREET_RESEARCH_LOG_TEMPLATE.md`.

## 16. Git hygiene

Add or confirm ignores for:

```gitignore
*.iso
*.gcm
*.rvz
*.sav
*.state
*.raw

.env
.env.*
!.env.example

sessions/
cache/
logs/

.codex/
```

Do not ignore project documentation or sanitized memory maps.

## 17. Updating from upstream later

Check upstream without merging automatically:

```powershell
git fetch upstream
git log --oneline --decorate --graph --all --max-count=30
```

When appropriate:

```powershell
git checkout main
git merge upstream/main
git push origin main
```

Then rebase or merge your feature branch deliberately.

## What "done enough to start NFL Street" looks like

You are ready to start real NFL Street research when all of these are true:

- Daywater builds and starts reliably.
- Ghidra initializes.
- Dolphin tooling is usable.
- An agent can invoke Daywater RE tools.
- Codex can run through your chosen auth path without requiring an OpenAI API key, if that is your chosen configuration.
- A task can persist findings.
- A known game state can be loaded repeatedly.
- You can prove one RAM discovery from end to end.

Do not wait for a perfect fork before beginning NFL Street research.

---

## Sources

Daywater repository and current setup:
https://github.com/MaukWM/daywater

OpenAI Codex Python SDK:
https://github.com/openai/codex/tree/main/sdk/python

Codex Python SDK getting started:
https://github.com/openai/codex/blob/main/sdk/python/docs/getting-started.md

Docker Desktop for Windows:
https://docs.docker.com/desktop/setup/install/windows-install/

Docker Desktop WSL 2:
https://docs.docker.com/desktop/features/wsl/

OpenAI MCP documentation:
https://developers.openai.com/learn/docs-mcp
