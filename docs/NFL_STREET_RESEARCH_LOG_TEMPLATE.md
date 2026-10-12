# NFL Street Reverse-Engineering Finding

Copy this section for each finding.

## Finding

**Name:**  
**Status:** candidate / probable / verified  
**Date:**  
**Researcher/agent:**  

## Game identity

**Title:** NFL Street  
**Platform:** GameCube  
**Region:**  
**Revision / hash:**  
**Dolphin build:**  
**Savestate/test scene:**  

## Memory

**Address:**  
**Pointer chain:**  
**Type:**  
**Size:**  
**Endianness:**  
**Structure base:**  
**Structure offset:**  

## Meaning

What does this value appear to represent?

## Discovery procedure

1.
2.
3.

## Observations

| Situation | Expected game value | Observed memory value |
|---|---:|---:|
| | | |
| | | |

## Reproduction

Describe a fresh restart/savestate procedure that reproduces the result.

## Negative controls

What changed in the game without changing this value?

What changed this value without changing nearby unrelated values?

## Ghidra relationships

**Readers:**  
**Writers:**  
**Candidate functions:**  
**Renamed symbols:**  

## Confidence rationale

Why is this `candidate`, `probable`, or `verified`?

## Runtime notes

How should `nflstreet-core` expose this?

Example:

```python
state.score.home
state.controlled_player.position.x
```

## Open questions

-
