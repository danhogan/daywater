# AGENTS.md

## Mission

Maintain and extend Daywater as a generic GameCube reverse-engineering platform while using it to research NFL Street.

Do not collapse the generic RE platform and the NFL Street runtime agent into one codebase unless a concrete architectural need appears.

## Immediate priority

1. Preserve current Daywater behavior.
2. Establish a clean baseline.
3. Add a Codex-backed agent option that can reuse an existing Codex/ChatGPT authentication session.
4. Keep the existing Inspect/OpenAI backend working.
5. Only then begin NFL Street-specific reverse engineering.

## Development rules

- Read the repository before proposing architecture changes.
- Prefer minimal changes over rewrites.
- Do not mass-upgrade dependencies as a first response to failures.
- Do not remove Inspect AI merely because Codex is added.
- Introduce an explicit agent-backend abstraction if the current architecture does not already have one.
- Prefer the official `openai-codex` Python SDK for Codex integration.
- Use a raw `codex exec` subprocess only for a prototype or if the SDK cannot meet a demonstrated requirement.
- Never commit credentials, auth state, API keys, game images, proprietary game assets, or savestates.
- Preserve the ability to merge future upstream Daywater changes.
- Keep generic improvements generic.
- Put NFL Street-specific addresses, structures, symbols, and behavior in a separate package/repository or clearly isolated data area.

## Reverse-engineering evidence rules

Never invent:

- RAM addresses;
- pointer chains;
- structure offsets;
- Ghidra function meanings;
- controller mappings;
- enum values;
- game-state semantics.

Every finding must have a confidence level:

- `candidate` — correlation observed, not reproduced enough.
- `probable` — repeated correlation with strong evidence.
- `verified` — reproducible across fresh test runs/states with a clear interpretation.

For every verified memory field, record:

- game title;
- game region/revision/hash if available;
- Dolphin build;
- address/pointer chain;
- primitive type;
- size;
- endianness if relevant;
- test procedure;
- observed values;
- related functions/readers/writers;
- confidence;
- notes.

## Testing philosophy

There are two classes of tests.

### Offline tests

Must run without Dolphin/Ghidra where possible:

- configuration parsing;
- state-model conversion;
- pointer arithmetic;
- action selection;
- serialization;
- backend-interface behavior with fakes/mocks.

### Integration tests

May require Daywater/Dolphin/Ghidra and must be clearly labeled.

Do not claim an emulator integration works unless it has been exercised against the live environment.

## Codex backend target

The desired model architecture is:

```text
Daywater
  |
  +-- existing Inspect/API backend
  |
  +-- Codex backend
         |
         +-- existing Codex auth
```

Prefer:

```python
from openai_codex import Codex
```

The SDK should reuse an existing Codex authentication session when possible.

If containerization prevents safe auth reuse, investigate:

1. controlled auth/config mount;
2. host-side Codex backend communicating with local Daywater;
3. a narrow Daywater MCP server;
4. raw CLI subprocess fallback.

Document tradeoffs before selecting a route.

## NFL Street runtime boundary

Do not put gameplay policy into Daywater.

Daywater should discover facts.

A separate runtime project should consume those facts.

Expected future layers:

```text
daywater
  -> discovers memory/functions

nflstreet-core
  -> turns raw game internals into structured state/actions

nflstreet-agent
  -> RuleBasedPolicy / JevPolicy / future policies
```

## Runtime-agent design rules

The runtime agent should not depend on raw addresses.

Bad:

```python
if read_float(0x80412345) > 3.0:
    ...
```

Good:

```python
state = game.observe()

if state.nearest_defender_distance < 3.0:
    ...
```

Policy output should be semantic:

```text
RUN_UPFIELD
CUT_LEFT
JUKE_RIGHT
PASS_RECEIVER_2
TACKLE
```

A deterministic local executor should convert semantic actions into frame-level controller input.

## First useful success

The first meaningful Daywater + NFL Street milestone is not "AI plays a game."

It is:

> From a reproducible NFL Street game state, the agent can discover, verify, and document one real game-state value in RAM.

Start with score.

Then player position.
