# Observed Thomas update schedule

In a fresh Dolphin 2603a run from saved match `8287f395`, eight consecutive
Thomas updates followed this order:

```text
preparation -> movement entry -> movement exit
then, for each next update:
input translation -> reset -> preparation -> movement entry -> movement exit
```

The input translator entered at `0x8003DDEC` and returned to `0x8003E85C`.
The reset entered at `0x800F7C90` and returned to `0x800F8710`. Preparation
entered at `0x800F705C`, with return address `0x800F721C`. Movement entered
at `0x80036B28` and returned to `0x800F8744`. The hash-checked original DOL
contains direct branch-and-link instructions at `0x8003E858`, `0x800F870C`,
and `0x800F8740` to those three functions. Preparation's observed return
address follows an indirect `bctrl` at `0x800F7218`. The
[schedule verifier](verify_update_schedule.py) checks these instructions,
call order, and return addresses against the trace.

The Dolphin frame dump numbered the eight preparation and movement calls
`30` through `37`, with one additional dumped frame between successive
updates. Input translation and reset for the next update were observed after
that frame boundary and before its preparation. The frame dump establishes
relative order in this run; it does not establish a 60 Hz wall-clock rate or
prove that all actors and game states use this cadence. The first update's
input translation happened before tracing began.

The expanded [sequence verifier](verify_update_sequence.py) checked every
captured reset call against the Windows x64 native reset executable. The
Windows translator reproduced all seven later 0x30-byte input returns, and
the native actor loop reproduced all eight complete actor input and movement
blocks. An earlier trace with only its first reset captured still passes the
same verifier. This is enough to drive a fixed-input, per-dumped-frame native
simulation of the observed actor branch. The full scheduler still needs
input sampling, update order for other actors, game services, and behavior
across a longer-lived match.

**Reference:** NFL Street GameCube GNNE69 revision 0, ISO SHA-1
`813c4511ba3135059a13de424555647e8252254b`, DOL SHA-1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`, Dolphin 2603a,
saved-match state SHA-256
`ea3d1fa29797d1521b2ab83aee866c190ed092865b7739ceb537d15aba5e7f32`.
Private W: trace `thomas-right-scheduler-eight.jsonl`, SHA-256
`550eb7c84023c9645e401d9eaa943b95b4d312e8ebd07b456a609670d2de3ad6`.
All raw RAM, game files, and Windows builds remain private.

**Confidence:** `verified` for the eight consecutive calls, their observed
order, return addresses, DOL call instructions, and complete Windows replay
outputs in this saved-match branch. A general frame scheduler for the game
remains a candidate pending runs with other states and actors.
