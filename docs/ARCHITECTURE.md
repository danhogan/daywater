# Architecture

## 1. Daywater

Purpose: generic GameCube reverse engineering.

Responsibilities:

- ISO/binary inspection;
- Ghidra analysis;
- memory scanning/reading;
- watchpoints/writer discovery;
- Dolphin control;
- screenshots/frame comparisons;
- savestate-assisted experimentation;
- persistent RE findings;
- agent orchestration/backends.

Daywater should not know NFL Street gameplay strategy.

## 2. nflstreet-core

Create later, after enough memory knowledge exists.

Purpose: turn raw NFL Street internals into stable domain objects.

Responsibilities:

```text
raw Dolphin memory
      |
      v
NFL Street version-specific adapter
      |
      v
GameState / PlayerState / TeamState
```

Potential API:

```python
state = observer.observe()

state.score_us
state.score_them
state.possession
state.play_phase
state.controlled_player
state.players
```

Also owns controller/action semantics:

```python
controller.move(x, y)
controller.tap("A")

actions.run_upfield()
actions.juke_left()
actions.pass_to(receiver_id)
```

Raw memory addresses belong in versioned configuration or a dedicated memory-map module.

## 3. nflstreet-agent

Purpose: make decisions.

Policies:

```text
RuleBasedPolicy
JevPolicy
future RL/ML policy
```

Input:

```text
structured GameState
```

Output:

```text
semantic action
```

The policy should not know whether the controller is implemented with Dolphin pipes, another Dolphin input backend, or something else.

## Why separate them

This lets you replace:

- the LLM;
- the policy;
- the controller backend;
- the NFL Street game version;
- Daywater itself;

without rewriting the whole stack.

It also keeps the Daywater fork useful for other GameCube games.

## Decision cadence

A slower model such as Jev should not be responsible for every 60 Hz controller frame.

Use:

```text
model decision
      |
      v
semantic action
      |
      v
local deterministic executor
      |
      v
one or more frame-level controller states
```

Example:

```text
Jev: JUKE_LEFT
      |
      v
executor:
  frame N:     hold direction
  frame N+1:   press mapped juke button
  frame N+4:   release button
  frame N+8:   restore movement
```

Exact timings and mappings must be measured, not guessed.

## Knowledge flow

Campaign foundation: see [Decompilation campaigns](DECOMPILATION_CAMPAIGNS.md)
for the executable-scoped knowledge graph, generated Ghidra index, runtime
bridge, and experiment boundary.

```text
Daywater finding
      |
      v
research log
      |
      v
verified memory map / symbol map
      |
      v
nflstreet-core
      |
      v
runtime policies
```

Only `verified` findings should be treated as stable runtime contracts.
