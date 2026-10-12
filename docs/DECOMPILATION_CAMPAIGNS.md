# Decompilation campaigns

## Repository audit and implementation plan

Daywater already has a `Project` for a game ISO and savestates in `sessions/`,
task runs, project findings, a SHA keyed Ghidra cache, and a Dolphin session
with memory reads, input sequences, and write watchpoints. The first milestone
extends these pieces rather than introducing another project database.

1. Scope curated knowledge to an existing project **and** an executable SHA.
2. Keep Ghidra output in its generated content addressed cache. Add function
   body ranges and disassembly to its current function, call graph, and string
   indexes.
3. Bridge runtime addresses and observed writer PCs to indexed functions and
   persist the observation and relationships.
4. Add a minimal experiment specification and run output that links changed
   memory and watchpoint evidence. Expose the bridge as an agent tool.

## Storage

`sessions/<project>/campaigns/<binary-sha1>/knowledge/graph.json` is the
single writer campaign graph. It supports function, global, runtime address,
structure, structure field, enum, string, savestate, experiment, hypothesis,
evidence, and finding entities. Every entity has a status (`unknown`,
`triaged`, `named`, `understood`, `typed`, `reconstructed`), confidence
(`candidate`, `probable`, `verified`), notes, tags, timestamps, and history.
Relations have explicit predicates and optional evidence IDs. Query by kind,
status, or tag with `Campaign.query`. Existing `findings.json` and Ghidra
`notes.json` remain readable and are not silently migrated or promoted.

Experiment output lives in the campaign's `experiments/` directory. Generated
Ghidra files remain in `cache/binaries/<sha1>/`: `functions.json` (now with
function body ranges), `callgraph.json`, `strings.json`,
`decompiled/<addr>.txt`, and `disassembly/<addr>.txt`. The executable SHA keys
the index; each investigation also returns a SHA-256 of its pseudocode so
changes can be compared. Re-run analysis with `force=True` to add disassembly
to an existing cache. Cache data is regenerable; `reconstruction/` is for
curated source and explanations.

## Runtime-to-static bridge

`investigate_address(campaign, cache_dir, address, session=...)` reads bytes,
collects write PCs with the existing Dolphin GDB watchpoint, resolves each PC
against indexed function ranges, loads pseudocode, disassembly, callers,
callees and referenced strings, and persists evidence and graph relations.
Pass `writer_pcs=[...]` to connect an earlier observation without a live
session. A supplied PC is labeled as such; it is not represented as a new
watchpoint hit. The agent receives the same structured result through its
`investigate_address` tool when static RE capability is enabled.

For a prior watchpoint result from the shell:

```bash
python -m src.core.campaign_cli PROJECT_ID BINARY_SHA1 0x80001000 --writer-pc 0x80123450
```

Without a live session or supplied PC, the command persists the address and
returns the explicit missing-observation limitations. The agent tool can
perform the live read/watchpoint when its task has a Dolphin debugger session.

No field meaning, Ghidra rename, or verified confidence is inferred from a
single observation. A PC outside the selected binary remains unresolved. The
selected cache must correspond to the executable loaded in Dolphin; address
mapping across relocatable modules is future work.

## Experiments

`ExperimentSpec` records a starting savestate ID, timestamped Dolphin pipe
commands, memory addresses, an optional write watchpoint, and run duration.
`run_experiment` requires a session already booted from that savestate and
checks the ID supplied by the caller. It records before/after bytes, changed
addresses, writer PCs, and links evidence to the experiment. Current input
timing is in seconds, since the Dolphin pipe offers no exact frame stepping.
The runner does not start Dolphin or verify savestate loading itself.

For verified memory fields, add game title, revision/hash, Dolphin build,
address or pointer chain, primitive type, size, endianness, procedure,
observations, and related readers/writers in the finding attributes and
support it with repeatable experiment evidence. The graph permits that data
but does not yet enforce a verification checklist.

## Next work

The existing cache covers decompilation, callers, callees and referenced
strings. Direct Ghidra xrefs, globals, type application, signatures, local
renames, comments and structure/enum creation require a safe transactional
PyGhidra editing layer and integration tests against a real binary. They are
not represented by sidecar renames. The campaign graph can track proposed
names and types meanwhile. Live Dolphin/Ghidra integration is not established
by the offline tests.
