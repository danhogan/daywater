# Fresh actor base model initialization

`actor_model_init.c` reconstructs the ordinary C state owned by original
`800D4E0C` and its small channel helpers `8002E8F8`, `800D3EB4`, `8004C8A0`
and `8004C918`. It initializes an isolated, normally zeroed, opaque actor span.
This is a substantive base model initializer, with caller supplied resources
and allocations. Player appearance `801639C8`, the later player state helper
`800D0448`, factory registration, resource ownership and playable startup
remain separate work.

The public interface carries semantic resource spans and opaque pointer tokens.
It contains no original RAM addresses. Tokens are written to the original
object layout for use by other isolated owners. The allocator receives explicit
transform/channel kinds, byte lengths and source header counts. The transform
request is `count*64`; each channel request is384 bytes, preserving original
`8015E910` even though that helper ignores its count when allocating. No
successful fallback allocator is provided. Malformed resource spans and failed
allocations return0; earlier allocations and mutations remain owned by the
caller. Original allocation failure behavior is not claimed equivalent.

## Owned operations

The initializer sets the low bit of the object word at+14, copies the supplied
DOL constant bit pattern to+18/+1C and clears+20. A supplied skeleton creates
transform storage, a channel with count+1, and source/transform tokens. Two
primary channels clear their48-byte spans, install source flags and channel
headers, and copy the source triples into channel memory. Two secondary
channels clear their124-byte spans when present, install original subtype
bytes, initialize channels, and copy signed position triples into model words
with scale256. The composite span clears316 bytes when present, installs its
source and subtype, initializes nine position triples and nineteen channel
triples. Final+3EC is cleared. Null secondary/composite branches preserve their
spans exactly as the original helpers do.

Every ordinary channel initially copies source BE16 triples from header+410
with a four-bit left shift modulo16 bits. Secondary activation replaces its
second triple with the unshifted source values. Composite activation replaces
nineteen triples with unshifted source values. The differential compares these
allocated blocks in full, including untouched allocator bytes, rather than
only comparing allocation requests.

## Verified evidence and scope

**Verified controlled initializer behavior:**384 fresh isolated comparisons,
64 combinations of present/absent resources in each of contexts14,15 and16,
with two fresh Windows processes per context. Every case compares the full
zeroed1A90-byte actor, all allocation requests (size, count and kind), and all
allocated storage including untouched bytes. The original preserved DLL
executes `800D4E0C` and the four channel helper bodies. Only allocation
`8018B7B8` and zero fill `8017BC54` are supplied boundaries. Strict Windows
compilation uses `-Wall -Wextra -Werror`. Logs repeat identically. Original
DOL, DLL and runner source hashes are unchanged.

Private report:
`W:\Daywater\research-temp\nflstreet-actor-model-init-20261012\verification-oracle.json`.

`verify_actor_model_init.py` independently rebuilds and repeats those384
comparisons. Its protected-input/executable/source hashes and final status are
recorded in `verification.json` in the same private directory.

The input resources come from the first saved player list entry, reached from
`r13-7BBC`, with r13=`803A9A40`. The checker reads descriptor tokens from the
existing actor fields below and supplies their mapped resource bytes to both
implementations. It reads the original constant word from`8025283C`. These
are fixture input locations; the native public API neither assumes nor reads
them. No actor addresses are inferred from allocator output.

| Fixture field/chain | Primitive, size, endian | Observed source values | Related original writers/readers |
|---|---|---|---|
| `(r13-7BBC) -> list[0]` | pointer words,4 bytes,BE | context14 list80BCD300, actor80B87D18 | saved player-list input; check follows this chain |
| actor+64 -> resource+6 | token4 bytes and header count2 bytes,BE | context14 resource80B96C40,count29 | `800D4E0C` skeleton path; ctor`800E1F7C` descriptor+4 |
| actor+BC / actor+104 -> resource+6 | tokens4 bytes and counts2 bytes,BE | context14 resources80B975A0/80B97D80,counts17/17 | `800D4E0C` primary paths; ctor descriptor+8/+C |
| actor+194 / actor+210 -> resource+6 | tokens4 bytes and counts2 bytes,BE | context14 resources80B98560/80B98B40,counts2/2 | `800D4E0C` secondary paths; ctor descriptor+10/+14 |
| actor+70 (also+34C) -> resource+6 | token4 bytes and count2 bytes,BE | context14 resource80B99120,count19 | `800D4E0C` composite path; ctor descriptor+18 |
| `8025283C` | raw unsigned word,4 bytes,BE |3F800000 | `800D4E0C` reads and stores+18/+1C |

Game provenance: NFL Street GameCube GNNE69 revision0, original DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`; source snapshots were captured
with Dolphin2603a. All new tests are offline original-module execution, with
explicit allocation storage. They do not exercise fresh live Dolphin startup.
The table's input tokens/counts are verified as fixture facts through repeated
reads and the original initializer comparison, not as universal runtime
addresses or complete gameplay meanings. Test procedure: fresh copy each
saved MEM1, zero a separate actor span, pass resource descriptors under each
of64 masks, execute the unchanged DLL and native C with identical allocation
tokens, compare every object/allocated byte, repeat in a new process.

The API names describe isolated component roles. Header+6 is a verified
source count used for allocation and tuple copying; its relationship to the
complete rendered skeleton is not established here. The supplied
`initial_word_bits` is copied exactly to+18/+1C; no scale, rate or timing
meaning is assigned to those words by this check.
