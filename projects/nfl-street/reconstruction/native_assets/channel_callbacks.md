# Animation group and program callbacks (Task K)

**Confidence: candidate; full verification pending.** NFL Street GNNE69 rev 0,
DOL SHA1 `46e3de854998a0f66c9be8c72a1164d22cd29eb4`. The owner reports context
14 cases 0–9 passed; case 10 exhausted the native callback budget at
`801770C4`. The fixture correction below is compiled but awaits an owner rerun.
No Windows executable, gate or mutation test was run by this agent. The number of actors that keep
animating is **not measured yet**; each soak prints active channels and poses
changed during its final 60 updates, out of 14 actors.

`channel_callbacks.c/.h` provide both hooks left by `actor_channels.c`:
`nflstreet_channel_callbacks_event` implements the `8001532C` entry, including
its group-key filtering, `80015D8C` fallback and interpreter operand reads;
`nflstreet_channel_callbacks_group` implements the 30 distinct non-null
callbacks in the saved gameplay table (224 entries at `80382950`, through
`r13-0x6718`, gameplay set ID 1). Follow-on clip/channel starts execute their
original-layout helpers, including resource selection and the shared RNG.

The implementation is an instruction-ordered C transcription. The large
`channel_callbacks_native.inc` contains 1,327 statically translated source
functions (81,709 operations), including the direct helper closure and four
indirect curve targets. Each function compiles to C control flow with constant
operations/calls; there is no runtime opcode decoder or original-module
dependency. This keeps the less obvious operand, float and state-transition
ordering visible by original instruction address. `gekko_float.h` supplies
single arithmetic, multiplier rounding, fused ties and integer conversion.
`build_channel_callbacks_native.py` regenerates the include from the specified
decompilation cache's **disassembly** and `callgraph.json`. The include records
SHA256 for every input disassembly file. The Python generator is not needed to
build or run the native module. Do not regenerate it in a mutation scratch copy.

Integration, without changing `actor_channels.c`:

```c
/* Persistent storage, alongside the existing channels context. */
nflstreet_channel_callbacks callbacks;
nflstreet_channel_callbacks_init(&callbacks);
channels.event = nflstreet_channel_callbacks_event;
channels.group = nflstreet_channel_callbacks_group;
channels.user = &callbacks;
/* Keep channels.span, memory, aram and direct as configured for actor_channels. */
int ok = nflstreet_actor_channels_update(&channels, 1.0f);
if (!ok || !callbacks.ok) { /* stop; report failure_pc/failure_address */ }
```

The context owns callback scratch stack storage; retain it between updates.
The stack's emulated addresses match the existing original-check fixture.
Its contents are excluded from the RAM comparison. Other memory uses the
existing channel span. `callbacks.ok` retains the first failure across phase
boundaries, where `actor_channels` itself resets its own status. The hook user
slot belongs to this context; existing host providers need a wrapper if they
also use that slot. No shared-file changes are needed.

Build from the worktree root (this is the only Windows action performed here):

```bash
bash projects/nfl-street/reconstruction/native_assets/build_windows_channel_callbacks_original_check.sh /mnt/w/Daywater/research-temp/nflstreet-clips2-20261015
```

Owner-run gate, from the worktree root:

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_channel_callbacks.py
```

Optional context arguments: `14`, `15`, `16`; a subset produces a partial report.
Private output: `/mnt/w/Daywater/research-temp/nflstreet-clips2-20261015/`.
The gate checks baseline and input hashes, hashes all compilation sources using
`native_assets/`-prefixed entries, and requires two identical fresh process runs
per context. It never treats a rejected native/original case as successful.

The checker runs the full `8003A24C` on all 14 actors on both sides, with **the
original group/event callbacks executing unmodified**. It compares all RAM
outside the oracle stack and the per-actor blend locals after every update.
It includes the saved RAM as is, 400 deterministic edited cases per context,
and three independent 600-update soaks. Edits retain valid saved callback
private layouts and keys, vary manager delays and retirement weights, channel
delays/timing, actor visual fields, elapsed time, and install synthetic programs
containing accepted/rejected events and operands. Both sides use the same
prepared RAM. Callback entry counts and operand coverage are checked too.

Case 10 correction (source evidence, **probable**, original rerun pending):
`801770C4` is `cmpwi r26,0` in the already translated frame interpreter;
`801770C8` branches back to read another word. The rejection was its loop
exhausting the host basic-block budget, not a missing instruction/span.
The original `801770FC` scan initializes through `8017724C` with interpreter
mode 1. That mode uses `80173A74` and the default callback at `r13-0x6730`
(`80015D8C` in context 14). Its `C009` branch (`80015E04` -> `80015F08`)
reads two operands; live `8001532C` accepts `C009` without reading operands.
The old synthetic `[C009, 0, 1, 8001]` therefore had no frames in scan mode:
the operand reads consumed 0 and 1, and `8001` restarted it indefinitely.
The fixture now emits explicit operands before its two frames, using the
larger arity of the live/default callbacks. The same correction covers
`C019` (three scan operands), `C01A` and `C022` (one each). Case 10 and all
event commands remain included; no callback behavior is skipped or budget
increased. A budget-specific failure diagnostic now distinguishes this
condition from unsupported instructions or invalid spans. Full comparison
and mutation results remain pending.

`channel_callbacks_mutants.json` contains eight proposed one-line faults. Apply
each individually to a scratch copy, compile with the same script, and run the
checker there. None is claimed caught before the owner runs it. Some faults
may require stronger edited coverage after that first run.

Not modeled: `80039990` and `80039A18` remain the existing channel module's
omissions (original returns at their entries); `8017BB78` remains the file-backed
ARAM boundary. Inherited channel-module limits, including large-argument
`rem_pio2f`, are errors rather than accepted comparison skips. Console hardware,
privileged SPR/MSR/time-base operations, system calls, CPU exceptions/FPSCR
state and indirect function targets outside the translated closure are not a
console runtime: unsupported instructions and invalid accesses fail with the
source PC/target. Cache ordering operations have no RAM effect. Quantized
paired loads/stores other than GQR0 are rejected. Host NaN payloads and rounding
mode assumptions are those documented in `gekko_float.h`. This is not verified
coverage of every gameplay state or every callback-table ID.
