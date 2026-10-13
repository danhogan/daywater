# Owned player appearance initializer state

`player_appearance_init.c` reconstructs original `801639C8` direct writes and
its vector, node descriptor, table, registry and mapping helpers in ordinary C.
This extends the owned constructor state. Complex model, texture and node
resource binding owners remain required services. This is not a completed
renderer, resource loader or fresh playable player.

The public API accepts a bounded actor span, at least45 descriptor bytes,
opaque actor/resource tokens, bounded registries and a data catalog. It has no
original RAM address dependency. Catalog data is supplied by its owner; no
proprietary asset tables are embedded. The actor token only represents
self-references within the caller's isolated namespace. Required service and
allocation callbacks have no successful fallback. A rejected callback returns0
and preserves earlier mutations and allocations; equivalent original failure
behavior is not claimed.

## State owned by the native implementation

Descriptor words+20 through+38 and bytes+40 through+44 flow into the exact
original actor slots. The indexed catalog word selected by descriptor+38 is
stored at actor+1358. The resource owner supplies the base and side resource
words; the native initializer retains both output writes and every repeated
request. It owns nine12-byte vector initializations and twelve240-byte node
initializations, preserving original untouched bytes and dirty-word ORs.

The node table owns its45-entry zero initialization after an explicit
(count45,stride4) allocation. The channel table owns64 catalog-default words
after an explicit (count64,stride4) allocation. The second node receives the
original+350 table reference and+35E literal. Mapping bytes and dirty words
follow the original conditional writer, including its exact
`(slot>>3)&0x1FFFFFFC` dirty-word index and `1<<(slot&3)` bit. This unusual
indexing is retained directly from the source, without assigning additional
semantics.

The actor registry writes the supplied actor token into one52-byte entry and
stores that entry's token at actor+4A8; its counter advances. The binding
registry appends actor-token+1074. Actor+108C/+1090/+1094/+1098 retain the
original binding descriptor writes. Both registries are bounded spans with
caller-owned counters, rather than hidden game globals.

`80164CA0` is owned as its repeated side-resource request followed by reading
the supplied side header+18 value. `80164DD8` is owned as the catalog's seven-
word record sequence: find the named side node, optionally reset its target
when the first byte of the flag word is nonzero, then apply the five literal
record words. Actual target lookup/reset/application remain explicit services.
No original source bytes for those catalog records are checked in.

## Original comparison

**Verified controlled behavior:**48 comparisons across saved contexts14,15
and16, with two fresh Windows processes per context. Each process checks
zeroed and prefilled actor spans, source-derived descriptor values and a
controlled variant using actual catalog rows0/1, and both capture-only and
explicit binding-mutation service fixtures. Every case compares all1A90 actor
bytes, both full allocated spans including untouched bytes, both full registry
spans/counters, and89 ordered normalized service requests and outputs.

The preserved original DLL executes the complete `801639C8` body and owned
small helper bodies: `801EB100/801CA47C`, `80162FC4`, `80163020`, `801EBF7C`,
`801EB1C4`, `801EBF60`, the node setters, `801EBE64`, `801EBEFC`, `801EBDD4`
/`801C7368`, `801EB204`, `801636C0`, `8016362C/801EB70C`, `80164CA0` and
`80164DD8`. The checker supplies allocation, resource lookup, model binding,
texture selection, node copying/binding and side-node target operations.
`801630B0`, `801C7528`, `801EBBA8`, `801EBC78` and the actual rendering
resource owners are not reconstructed by this module. The `801C7528`/`801EBBA8`/
`801EBC78` texture hookup and the `80164DD8` settings now have native owners in
`slot_materials.md`, verified on their own; they are not connected to this
module's service interface yet. Service mutation cases
are explicitly controlled test writes, not original gameplay semantics.

Private report:
`W:\Daywater\research-temp\nflstreet-player-appearance-20261013\verification-oracle.json`.
Root independently rebuilt and repeated all 48 comparisons; report
`W:\Daywater\research-temp\nflstreet-player-appearance-20261012\verification.json`.
The connected constructor gate is documented in [player startup](player_startup.md).

Original DOL, DLL and runner source hashes and all supplied input hashes remain
unchanged. Strict Windows compilation passes `-Wall -Wextra -Werror`. Repeated
process logs agree.

## Fixture provenance and confidence

Game: NFL Street GameCube GNNE69 revision0; DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Source MEM1 snapshots were captured
with Dolphin2603a. These are offline original-module comparisons and do not
exercise fresh live Dolphin startup.

| Input/read chain | Type,size,endian | Observed context14 values | Reader/writer and confidence |
|---|---|---|---|
| `(r13-7BBC)->list[0]`,r13=803A9A40 | pointer words,4 bytes,BE | actor80B87D18 | fixture input; verified saved-state fact |
| actor+400..418 inverse descriptor+20..38 | words,7x4 bytes,BE |1CA,C6E1,C6E2,175,1,1,0 | writes from801639C8; verified controlled transfer |
| actor+1360..1364 inverse descriptor+40..44 | bytes,5x1 |0,0,0,0,1 | writes from801639C8; verified controlled transfer |
| `8028DD64 + descriptor[38]*4` | indexed word,4 bytes,BE | row0=3F800000 |801639C8→actor+1358; verified table input/transfer, meaning unspecified |
| `8028CF08 + descriptor[42]*100` | mapping row,25x4 bytes,BE | first two actual rows supplied privately |801639C8/801EB204; verified conditional mapping behavior |
| `80268D88` | word,4 bytes,BE | literal source default supplied privately |801EBE64 channel initialization; verified copied default |
| `8028D480` | opaque static token,4-byte pointer representation,BE | literal source binding token |8016362C→actor+1090; verified token transfer |
| `8028DD90` | seven-word records,28 bytes each,BE |25 nonzero-key records then sentinel |80164DD8; verified request ordering/payload |
| r13-58E8 and `8030FFDC+count*34` | count/token words4 bytes,BE | controlled starting count2 |801636C0; verified bounded registry write/advance |
| r13-6238 and `8034FB88+count*4` | count/token words4 bytes,BE | controlled starting count3 |801EB70C; verified bounded registry write/advance |

The descriptor is reconstructed by inverting known writes in saved actor
state. It is not a captured original fresh constructor descriptor. Resource
lookup responses, side texture values and target handles are explicit fixture
service results. Test procedure: fresh MEM1 copy per case; source catalog reads
and inverse descriptor construction; original body execution with supplied
services; native execution with identical catalog/services/registries;
whole-span and ordered-request comparisons; repeat in a new process. No new
universal address, field meaning or fresh asset loading claim follows from
these controlled results.
