# Native AI policy services

**Confidence: candidate, pending the external source-pinned Windows gate.**
NFL Street, GameCube USA GNNE69 revision 0; DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. The saved-context metadata does
not identify the Dolphin build. No Windows executable was run by this author.
Compilation establishes no original-behavior or live-movement claim.

`ai_policy.c/.h` implement the five requested service entries `800AAC08`,
`800AFC88`, `800A0D10`, `8006E2E4`, `8007C180` as instruction-ordered native C,
including their direct helper closure and the original saved state table's
247 distinct callback targets. There are 2,583 source functions and 171,954
operations. These are constant C operations and calls, with original-PC
comments; there is no runtime opcode decoder, DLL dependency, original-backed
provider, replacement idle policy, or guessed movement input.

The source builder reuses the existing callback transcription tool without
editing it. Cached disassembly and its SHA256 manifest provide the principal
evidence. Some small state-table callbacks have no cached Ghidra body: the
builder decodes their actual saved code bytes, bounded by the next known
function, only at source construction. `ai_policy_sources.json` records those
ranges/hashes and every unsupported instruction. Capstone is needed only for
regeneration; neither compilation nor execution requires it. Regeneration
also needs the private context-14 RAM and original disassembly cache. Never
regenerate sources while the source-pinned gate is running.

The native API is:

```c
nflstreet_ai_policy policy;
nflstreet_ai_policy_init(&policy, dispatch.ram);
dispatch.service = nflstreet_ai_policy_service;
dispatch.user = &policy;
```

Retain the context while dispatch runs. It owns 60 KiB of call-stack bytes,
addressed by the oracle fixture's `80F38000..80F46FFF` tokens, rather than
borrowing gameplay memory. All other reads and writes use the supplied
big-endian RAM span and r13. Missing spans, unknown indirect targets, depth or
basic-block limits and unreconstructed system instructions return failure;
`failure_pc`, `failure_address`, `unsupported` and
`failure_budget_exhausted` identify the cause. Prior RAM writes remain.

The existing live containment restores MEM1 and holds an actor on a failed
policy call. The live log now reports the nested failure and prints
`AI moving: <n>` once per second, counting distinct actors whose committed XY
changed during that interval. No positive movement count has been measured
here. `live_main.c` installs the provider after roster initialization and also
supplies it to the selector's `80076C48`/`80076E18` event boundary. Its existing
clip-request provider is retained. CMake includes `ai_policy.c`, with MSVC
`/bigobj`; no protected shared module was edited.

The checker uses the original module unhooked: no native stand-in, early
return or skipped effect. It compares all MEM1 outside the call stack after
each actor operation and compares the five policy callbacks' return values.
Every actor assigned to each service in each saved context receives one saved
and 400 edited cases. It also checks real dispatch/finalization on all thirteen
non-controlled actors and 24 independent 32-update sequences with input commit.
Saved service populations in requested order are 4/3/3/1/1 (context 14),
6/3/2/1/1 (15) and 6/2/2/2/1 (16). Context 14's remaining actor executes its
actual `80094E60` update. Edits preserve live pointers and state unions while
varying positions, headings, ratings, completion flags, timers and dirty outputs.

Owner-run gate, from the worktree root:

```bash
python3 projects/nfl-street/reconstruction/native_assets/verify_ai_policy.py
```

It pins all inputs, compilation sources, this document and live integration;
runs two fresh processes per context 14/15/16; requires identical logs and full
coverage; compiles seven JSON one-line mutants separately and requires a RAM
or return mismatch for each. A crash, timeout, native rejection or build error
is never counted as a caught mutant. Private evidence/builds are under
`/mnt/w/Daywater/research-temp/nflstreet-ai-policy-20261015/`.
The separately supplied `nflstreet-delegation/aipolicy_gate.sh` owns the live
Windows build, smoke run and positive movement requirement.

Remaining boundaries: system-state instructions (MSR, GQR/HID and exception
registers, `sc`, `rfi`, time-base reads) listed by source PC in the manifest
reject if reached, as do indirect targets outside the static closure. These
are explicit unsupported paths, not verified harmless no-ops. This does not
advance football physics, collisions, play clock or animation by itself;
the live host still owns their existing update order. The checker retains
policy/clip state across updates but does not simulate original movement or
advance the full original match loop. Complete verification, mutant kills,
live movement count and the exact next reached missing service remain for
the external runner to establish.
