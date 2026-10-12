# Curated reconstruction

Store reviewed source models and explanations here after evidence supports them.
Raw Ghidra pseudocode belongs in the generated binary cache, not here.

The [kinematics replay](kinematics/README.md) is a guarded native Windows C
reconstruction of a connected Thomas position/velocity update. All 21 output
words match eleven fresh live Dolphin calls bit for bit. It runs independently
for captured input blocks; actor input/state dispatch is still outside it.
The [live stick-to-movement trace](input_to_movement.md) identifies the
executed input translation and command setter that precede this replay.
The [cardinal input replay](input_translation/README.md) reconstructs the
observed actor-field writes for three held directions and the heading written
on two fresh direction changes. It shares the native angle calculation with
the kinematics replay; game-service calls and update scheduling remain open.
The [command-to-movement replay](command_to_movement/README.md) computes the
observed bounded result-heading branch, carries actor heading through the
original command setter's three stores, and runs the native movement step.
The [actor-update replay](command_to_movement/actor_update.md) also computes
acceleration from the actor and movement fields for three fresh live calls.
The [consecutive update replay](command_to_movement/update_sequence.md)
matches six calls across two three-update sequences with one observed
intercall reset; its actor input block is still supplied by the game.
The [calculated intercall setup](command_to_movement/reset_acceleration.md)
replaces the captured reset value for the tested normal-movement branch.
The native loop also matches fresh three-update live runs that change from
right to forward and forward to right after the first update. A guarded
cardinal mode computes the four changing heading fields from R/F/L sequences
tested as `RFF`, `FRR`, `RFR`, and `RLL`
instead of reading all actor input blocks from the trace. Full controller
translation and game scheduling remain open.
The [actor runtime interface](command_to_movement/actor_runtime.h) exposes
the guarded stateful movement step independently of the trace replay CLI.
Its [input-vector bridge](command_to_movement/input_vector_bridge.md) also
accepts the game's normalized cardinal vector and matches complete actor
input and movement blocks across four live direction-change sequences.
The longer eight-update right-forward-right run also matches after the
kinematics length calculation was aligned with the original fused operation.
The [partial-strength branch](command_to_movement/partial_strength.md) adds
four fresh live three-update comparisons, including the movement speed cap.

The [odd polynomial pair](odd_polynomial_pair/README.md) models the
29-instruction paired-single routine at `0x80188BB0`. Its numerical core
matched the existing native module bit for bit on 100,000 finite synthetic
inputs under one GQR6 configuration. Its guarded implementation subsequently
matched full CPU state and touched RAM on 99,942 cases, with 58 guarded skips.
The [paired product](paired_product/README.md) models the second measured hot
routine at `0x801FFDF0`; its output words and full CPU state matched the
module on 100,000 synthetic finite inputs with GQR0 set to zero.
The [live Windows probe](../native/live_pair_probe.md) also found exact
output-word agreement for 32 captured calls to each function in a saved match.
Both guarded full-state candidates were exercised in a separate
[combined Windows build](../native/two_fastpaths_experiment.md).
The [live differential probe](../native/live_differential_probe.md) also
compared accepted candidate calls against the original generated functions
on identical live CPU and RAM input.
The [locked-cache extension](../native/locked_cache_fastpath.md) then covered
the product calls whose input and output live at `0xE0000000`, with both
synthetic and live full-state comparisons.
The [three pair projection](three_pair_projection/README.md) at `0x801FFF10`
is a third guarded source candidate. Its full CPU state and touched memory
matched the existing module in 100,000 offline cases and 64 sampled live
Windows calls, including calls after the first 10,000 in a saved match.
The adjacent [weighted pair reduction](weighted_pair_reduction/README.md)
at `0x801FFEBC` has a separate numerical source model. Its three outputs
and guarded CPU-state effects matched the original module in 100,000
disjoint-buffer offline calls. The guarded candidate also matched 64
sampled live Windows calls and completed a separate saved-match build test.

The first score slices are the [reader](score_reader/README.md),
[writer and adjacent clear function](score_writer/README.md), and
[score delta path](score_delta/README.md), and
[score record handler](score_record/README.md). The
[named table wrappers](score_table/README.md) connect that path to the
Windows [in-memory `FNIG` adapter](../native/README.md). Their DOL
locations and bytes are checked, and each has a C source candidate. The
32-byte reader, 52-byte writer, and adjacent 16-byte clear function now have
separate, byte-matching PowerPC source variants with explicit register
constraints. The larger functions do not yet match. The layout shared by the
portable candidates is in `score_fields.h`.

A host-side C check in `tests/test_score_candidates.c` exercises both score
paths, the clear function, and the full-input versus halfword-truncation edge
case. It validates the source model's behavior, not a PowerPC binary match.

```bash
gcc -std=c89 -Wall -Wextra -Werror \
  projects/nfl-street/reconstruction/score_reader/score_reader.c \
  projects/nfl-street/reconstruction/score_writer/score_writer.c \
  projects/nfl-street/reconstruction/score_writer/clear_unknown_40.c \
  projects/nfl-street/reconstruction/tests/test_score_candidates.c \
  -o /tmp/score-candidates-test
/tmp/score-candidates-test
```

`build_score_matches.sh` compiles the PowerPC-only clear, reader, and writer
variants, extracts their relocation-free `.text` sections to W:, and compares
their 16, 32, and 52 bytes with the hash-checked DOL. It does not use the PowerPC-only
sources in the host-side test.

See [toolchain.md](toolchain.md) for the locally verified decomp-toolkit
installation and the current whole-DOL analysis limitation. The score delta
README records the separate native Windows x64 test build. The
`build_windows_score_path.sh` script builds the expanded score path, including
the record handler, for Windows x64.
