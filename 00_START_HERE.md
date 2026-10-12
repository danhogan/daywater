# Daywater + NFL Street: Start Here

This bundle is a starter pack for building an NFL Street reverse-engineering and runtime-agent project on top of Daywater.

## Recommended order

1. Read `SETUP_GUIDE.md`.
2. Fork Daywater and clone your fork.
3. Give the coding agent the `AGENTS.md` file from this bundle.
4. Give it `prompts/01_AUDIT_DAYWATER.md` first.
5. If the baseline is healthy, give it `prompts/02_ADD_CODEX_BACKEND.md`.
6. Once Codex-backed Daywater is usable, begin NFL Street research with `prompts/03_NFL_STREET_FIRST_TASK.md`.
7. Record verified discoveries using `docs/NFL_STREET_RESEARCH_LOG_TEMPLATE.md`.
8. Keep NFL Street-specific runtime code outside Daywater unless a generic Daywater capability truly belongs upstream.

## Intended architecture

```text
                 DEVELOPMENT / RESEARCH

                      Daywater fork
                  +-------------------+
                  | Ghidra            |
                  | Dolphin           |
                  | RAM tools         |
                  | controller tools  |
                  | screenshots       |
                  | RE knowledge      |
                  +---------+---------+
                            |
                  verified game knowledge
                            |
                            v
                 nflstreet-core (later)
                  +-------------------+
                  | memory map        |
                  | structured state  |
                  | actions           |
                  | controller layer  |
                  +---------+---------+
                            |
                            v
                nflstreet-agent (later)
                  +-------------------+
                  | RuleBasedPolicy   |
                  | JevPolicy         |
                  | telemetry/tests   |
                  +-------------------+
```

Daywater is the research environment. The NFL Street runtime agent is a separate consumer of the knowledge Daywater discovers.

## Files in this bundle

- `SETUP_GUIDE.md` — step-by-step Windows setup and first milestones.
- `AGENTS.md` — standing instructions for Codex/Claude when working on the Daywater fork.
- `docs/ARCHITECTURE.md` — project boundaries and design rules.
- `docs/NFL_STREET_RESEARCH_LOG_TEMPLATE.md` — template for documenting verified addresses/functions.
- `prompts/01_AUDIT_DAYWATER.md` — first coding-agent task.
- `prompts/02_ADD_CODEX_BACKEND.md` — second task: add Codex-authenticated backend without an API key.
- `prompts/03_NFL_STREET_FIRST_TASK.md` — first actual NFL Street reverse-engineering task.

## Ground rules

- Do not commit game ISOs, extracted copyrighted game assets, savestates, API keys, auth tokens, or local Daywater session data.
- Do not invent RAM addresses or function meanings.
- Label every reverse-engineering result as `candidate`, `probable`, or `verified`.
- Prefer small, reversible changes to the Daywater fork.
- Keep the upstream Daywater remote so updates can still be merged.
