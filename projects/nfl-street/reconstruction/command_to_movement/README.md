# Native command-to-movement replay

The [larger actor-update replay](actor_update.md) now computes acceleration
before this command handoff for three live Thomas calls. This narrower tool
remains useful for the eight earlier command captures that lack a preparation
entry snapshot.

The original GNNE69 revision 0 DOL function `0x80036B18` stores three
movement-command values in the actor's kinematics block: r5 at `+0x18`,
r4 at `+0x38`, and f1 at `+0x34`. Its PowerPC instructions at
`0x80036B18..0x80036B24` establish those stores directly. The caller at
`0x800F705C` passes actor input heading `+0x204` as r4. In five live
Dolphin 2603a captures, the command return block was identical to the
following `0x80036B28` movement entry block.

`command_to_movement_replay.c` computes the caller's result heading through
the observed `0x800F6FA4` and `0x800F6F38` branches, applies the three
stores, then invokes the guarded
[native kinematics reconstruction](../kinematics/README.md). It takes the
actor input block, pre-command state block, and f1 acceleration bits. The
24-bit angular-distance and turn-toward operations follow the original DOL
instructions at `0x80188F28` and `0x800F6F38`. The turn limit `0x0005B05B`
was read at `r13-0x70DC = 0x803A2964` from the supplied second-down MEM1
image (SHA-256
`8c0152c92796e9783674be81703025ede544fcb71c1d62fe84ced9680facdd0a`);
two independent right-turn captures also changed their prior heading by
exactly that amount. When the original branch consults actor `+0x318`, the
replay accepts it only if both possible targets produce the same result.
It rejects an ambiguous target.

In these eight captures, f1 matched the prior `+0x34` bits, so the verifier
uses those bits and rejects captures where they differ. The caller's
acceleration calculation remains unreconstructed. This is an exact replay
of a bounded handoff, not an independently scheduled actor update.

For each of three command, three held-stick, and two fresh heading-transition captures,
the actual Windows x64 program matched both the complete 0x54-byte block
immediately after the command store and all 21 words after movement. The
[verifier](verify_capture.py) checks the original PCs, r4 against actor
heading, r5 against the stored result, the command/movement boundary, and
both output blocks. The executable and raw traces remain private on W:.
The Windows executable SHA-256 is
`1cd96f6134070bd5bd92f5105f2e07ccbd416a33a53059864cb67a53d8e11b28`.

Build it with
`bash projects/nfl-street/reconstruction/build_windows_command_to_movement_replay.sh`.
To check a private trace, run the verifier with `--trace`, `--executable`,
and `--windows`, as shown in the [input replay instructions](../input_translation/README.md).

**Confidence:** `verified` for the three direct stores, observed actor-heading
source, turn-limit value in the supplied state, and the eight complete live
handoffs. The bounded result-heading calculation matches those captures;
its actor `+0x318` branch remains guarded rather than fully modeled.
Acceleration update, game-service calls, and continuous update scheduling
remain open. The game still does not run independently.
