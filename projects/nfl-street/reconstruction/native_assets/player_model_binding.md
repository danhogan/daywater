# Owned player model binding orchestration

`player_model_binding.c` owns the source state and sequence of `801630B0` and
tractable model helpers. It replaces a whole model-binding capture boundary
with allocated command headers, actual model copies, index-buffer initialization
and remapping, shared-model creation and ordered required resource/render
services. It is now connected through the [bound-player startup adapter](player_bound_startup.md).

The API uses opaque namespace tokens resolved to bounded mutable views. The
caller supplies a scratch table, shared-model/header span, command allocation
size and mapping-table token. Allocation includes the original count/stride;
temporary remap allocations are explicitly released. All cache, resource,
render and name-comparison callbacks are required. Returns0 on invalid spans
or rejected operations without undoing earlier mutations. Equivalent original
failure behavior is not established.

## Owned source state

The native implementation reconstructs `801EAB68` and `801CB800`: align each
command allocation to32 bytes, store its token/zero length/aligned size/alignment
in its16-byte header, and initialize a model view. `801C65D8` copies13 words
while resetting word+1C. `801C65A0` clears only+4 through+30; the first word is
preserved on a null initial resource. `801C6BF4/801C6BB0` allocate and clear
an index buffer using the resource metadata's+0C count.

The scratch halfword selected by actor byte+1364 is set to1000 for both render
requests, then reset to0. Scratch+10 is also reset. The two index-buffer words
at actor+44C/+490 are preserved across the later13-word model copies.

`801C6AA0` resolves actual source record names, allocates a temporary BE16 map,
searches the supplied target records through required name-comparison services,
and stores each mapped index (orFFFF). `801C648C` rewrites each source entry's
signed BE16 index at+6: negative/out-of-range indices becomeFFFF; valid indices
read their mapped value. The temporary map is released and model+18 becomes
the supplied target table token. Actual source model-entry storage is mutated;
this is not a request-only reconstruction.

If the shared-model first word is0, its model view is copied and a new index
buffer created, followed by a command allocation aligned from literal7404.
The source command size for the two actor slots is supplied by the caller;
the second uses its right-shifted half.

Actual `801EAC04` command-stream rendering remains a required render service.
Cache activation, resource query ownership, name comparisons, mapping selection
and final resource bindings also remain explicit services. Their controlled
responses do not demonstrate asset loading, a complete renderer or a fresh
playable player. Services can access their own resource namespace and write
fields/buffers that they own.

## Verified controlled differential

**Verified:**96 original/native comparisons,16 cases in each of saved contexts
14,15 and16, repeated in two fresh Windows processes. Cases cover zeroed and
prefilled actors, null/non-null initial resource views, empty/present shared
model state, and capture-only/controlled render-binding mutations. Each case
matches the full1A90 actor,4/6/8 allocation requests and complete buffers,
temporary-release records, catalog/shared-model/source-storage writes and2084
ordered service requests. Of those requests2070 are source name comparisons.
Render-request records also compare the complete76-byte scratch table at the
request boundary, including its transient1000 halfword.
Root independently rebuilt and repeated all96 comparisons; report
`W:\Daywater\research-temp\nflstreet-player-model-binding-20261012\verification.json`.

The preserved original DLL executes `801630B0` and owned helpers `801EAB68`,
`801CB800`, `801C65D8`, `801C65A0`, `801C6BF4`, `801C6BB0`, `801C6AA0`,
`801CA2BC` and `801C648C`. The checker supplies allocation/free, cache/resource
owners, rendering, and the underlying name-comparison boundary (`8017CCA0`
inside the original boolean helper). It compares all copied RAM outside a
separate128-KiB original stack scratch area, in addition to explicit allocation
and request comparisons. No protected source RAM file is modified.

Private report:
`W:\Daywater\research-temp\nflstreet-player-model-binding-20261013\verification-oracle.json`.

Strict Windows compilation uses `-Wall -Wextra -Werror`; repeated logs agree.
Original DOL, DLL and runner source and all input hashes remain unchanged.

## Fixture provenance and confidence

Game: NFL Street GameCube GNNE69 revision0; DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Original saved-state captures were
made with Dolphin2603a. These checks are offline original-module execution.

| Source input | Primitive,size,endian | Context14 observed values | Functions/confidence |
|---|---|---|---|
| `(r13-7BBC)->list[0]`,r13=803A9A40 | token words4 bytes,BE | actor80B87D18 | verified saved fixture traversal |
| saved actor+430/+474 | model views13x4 bytes,BE | first words810E8850/810DB3F0 |65D8 copies; verified controlled use of source views |
| view+8 -> metadata+0C | token/count words4 bytes,BE | both countsE7 |6BF4/6BB0; verified index allocation and zeroing |
| metadata+0/+4 | entry token/count words4 bytes,BE | entries810FC5D4/810E7C34,count72 each |648C writes signed BE16 field+6 at24-byte stride; verified remap |
| view+18 -> record table | token/count words4 bytes,BE |8028DD10,count2D,records8028D4A0 |6AA0/CA2BC; verified name request ordering |
| r13-685C | word4 bytes,BE |3E80 |630B0 supplies first command size, second half; verified allocation |
| `8028DD18` scratch span | halfwords,76 bytes,BE | source bytes privately supplied; selected field transient1000 |630B0/EAC04 request boundary; verified literal writes/reset |
| `803102B4` model +`803102E8` header |52+16 bytes,BE | copied source shared view or explicitly cleared fixture |630B0 conditional branch; verified copies/allocations |

The two final saved model views are supplied as both the initial type4 and
later type5 resource responses. This is an explicit controlled service choice,
not a claim that the original resource query returns those same records in a
fresh startup. Source record names, metadata, entries and mapping tables are
read from private captures; no proprietary records are embedded in source.
Test procedure: fresh copied MEM1; derive the two saved model views and resource
metadata; create isolated actor input and shared-state variant; execute original
body with explicit services; run native owner against a separate identical
namespace; compare whole actor, buffers, releases, table/shared/source writes
and requests; repeat in a fresh process. Gameplay/resource meanings beyond the
literal source operations remain unverified.
