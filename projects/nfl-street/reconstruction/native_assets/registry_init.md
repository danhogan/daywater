# Registry table initialization and guarded entry (row 05 slice)

NFL Street GNNE69 North America rev0, DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. `registry_init.c/h` reconstructs
`80195234` and wrapper `80195368`. Existing registry/pool/factory/scene modules
remain unchanged. This closes the table **initialization** gap behind explicit
heap services; it does not establish teardown or the entire registry lifecycle.

## Verified controlled boundary

The claim applies when private `full/verification.json` has status `passed`.
The untouched ModernGekko DLL executes both routines. Heap `8018B7E4` and
geterror `801AF630` return early with controlled responses. Compare original
returns, exact ordered service arguments/responses, table bytes and global/flag
observations at each call, and all 24 MiB except the precise32-byte stack
`[80F46FE8,80F47008)`. Wrapper8 + initializer16 + caller LR slot accounts for
that interval. No table, neighboring byte or global is masked.

The initializer requests `heap(1,count*20,0)`. Heap0 returns0 without publishing
root/count. On nonzero return, its signed-positive loop zeros **only the first
BE u32 word** of each20-byte record. Remaining16 bytes and neighboring memory
stay dirty. Then it publishes count, publishes root and returns1. Count0 with
nonzero heap return still publishes count0/root, touches no table bytes and can
publish an unaligned identity because there is no table word access. Low-level
reinitialization disregards the wrapper flag and does not change that byte.

| Identity | Type / size | Reader / writer / interpretation |
| --- | --- | --- |
| r13-4F18 | BE u32 /4 | initializer publishes registry count after clearing; scene sizing consumes it |
| r13-4F14 | BE u32 pointer /4 | initializer publishes table root; registration/factory consume it |
| r13-6568 | u8 /1 | wrapper reads; successful initialization writes exactly1 |
| table+i*20 | BE u32 /4 | initializer clears only record size word; registration later fills record |

The guarded wrapper checks its flag first. Any nonzero byte returns `90001`
without heap/geterror access, ignoring count. With flag0 it calls the initializer:
success writes flag1 and returns0; failure calls geterror and returns that exact
code, including0 or a high-bit value, without writing the flag. A failure/error0
is therefore not distinguishable from success by the return code alone.

## Bounded composition

Every16th edited case prepares a new controlled stage on the same RAM and runs
initializer wrapper -> type0 registration -> type0 scene owner -> three factory
allocation attempts (two objects and exhaustion). Production uses unchanged
`registered_pool.c`, `scene_owner.c`, `actor_factory.c` and `object_pool.c`.
Matching original game routines and their layout/pool/memset/type0-init children
execute unhooked, compared after each component. These are explicitly ordered
component calls, not a replayed boot/entrypoint or insertion path.

Composition uses a precise168-byte stack union `[80F46F60,80F47008)`, covering
the deepest scene/list/layout/pool-total/pool-layout chain and caller LR slot.
Only external heap/error/geterror and supplied constructor `800E1F7C` return
early. Constructor service writes a controlled payload word, and its original
r3 is clobbered so factory object-return preservation is checked. Null and
non-null constructors are covered. Full table/pool/scene/list bytes and object
bytes at constructor entry are recorded in ordered service observations.
Type0 dispatch/init evidence is pinned against the DOL and capture slices as in
the existing scene gate; no dispatch-table producer is invented.

## API and host choices

`nflstreet_registry_init_context` holds borrowed bounded BE RAM, count/root/flag
slot identities and explicit heap/geterror services. Call
`nflstreet_registry_open(&context,count,&code)` at the guarded original entry,
or `nflstreet_registry_init(&context,count,&success)` for low-level behavior.
The latter's guest success is1/0. The wrapper's guest code is0/90001/geterror.
Each function separately returns host success; host rejection clears the guest
result and is not attributed to original validation.

Host safety restricts count*20 to nonwrapping arithmetic and requires bounded,
aligned accessed words. Unsupported negative/raw wrapping counts are rejected
before heap; the original uses signed count comparison and does not perform
this validation. Count0 requires only a bounded zero-length identity, since no
table word is accessed. A nonzero flag is checked before count/services/root
validation, matching the original early branch. Invalid heap responses can
reject after an observed request; no free or rollback is invented.

To join existing adapters after successful initialization, pass the published
root slot/count and same RAM into `nflstreet_registered_context`, then call
`nflstreet_registered_register`. Pass the resulting count snapshot/alignment
and existing type table into `nflstreet_scene_owner_context`. Factory allocation
continues through `nflstreet_registered_create` and caller-sized host scratch.
Callers retain all RAM and services; this API does not supply the actual heap,
class implementations, scene insertion/removal or teardown.

## Gate and provenance

Full mode uses contexts14/15/16 twice with one saved-as-is flag1 case and400
edited cases/process. Edited cases cover dirty record tails/neighbors, counts
0..512 including255/256, nonzero flag bytes, repeated wrapper attempts,
low-level reinitialization and heap failure, count0 publication, error0/high-bit
propagation, and25 composed stages/process. Seven malformed-host checks are
separate. Eight production mutants target heap owner, tail clearing, final word,
count/root publication, already-initialized code, error propagation and flag
value. Build failure/crash/host rejection never counts as a caught mutant.
Fast mode uses context15 once,40 edits and no mutants.

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_registry_init.py
```

Private evidence/builds/logs/mutants:
`W:/Daywater/research-temp/nflstreet-registry-init-20261018/{fast,full}/`.
Saved position-neutral captures are controlled regression states; Dolphin build
provenance is not established here. No fresh boot, natural heap implementation,
full lifecycle, real class construction, live gameplay or new emulator claim.
