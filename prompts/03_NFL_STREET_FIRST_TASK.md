# Task 3: First NFL Street Reverse-Engineering Task

## Objective

Use Daywater to find and verify the memory representation for the current game score in NFL Street for GameCube.

Do not modify game behavior.

Do not invent addresses.

## Method

Use a reproducible in-game state and savestates where useful.

Establish at least two or three distinct score values if practical.

Example reasoning pattern:

```text
known score A
 -> memory scan

known score B
 -> filter candidates

known score C
 -> filter candidates
```

The exact representation may not be a single obvious integer. Follow the evidence.

## Verification requirements

A result is not `verified` merely because an address matched once.

Attempt to show:

- the value corresponds to the intended team's score;
- the opposing team's score can be distinguished if both exist;
- the result survives a fresh reload/reproduction procedure;
- nearby unrelated state does not create a false positive;
- if the address is dynamic, identify the pointer/structure relationship rather than recording a one-session address as permanent.

Use Ghidra/writer discovery if it materially improves confidence.

## Deliverable

Create:

```text
research/nfl_street/score.md
```

Use the research-log format.

Include:

- game region/revision/hash if available;
- Dolphin build;
- address or pointer path;
- primitive representation;
- test procedure;
- observations;
- confidence;
- any known reader/writer functions;
- unresolved questions.

If no value can be verified, document the strongest candidates and the next experiment instead of pretending success.
