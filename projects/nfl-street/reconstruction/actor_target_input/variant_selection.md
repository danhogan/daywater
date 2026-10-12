# First locomotion animation-set key

## Result

`variant_selection.c` reconstructs the first-key decision in original
`0x80062C30` and joins its calculated key to the existing original hierarchy
resolver. The second key remains an upstream input. This connects a decision
about which animation set to use with its actual primary/alternate/other curves.
It does not yet make the Windows player change animation sets automatically.

The native API uses neutral actor fields and explicit availability bits for
lazy service results. A missing required fact rejects the decision without
changing outputs. Early exits do not demand unused service facts. Curve lookup
also rejects missing data without changing the descriptor or returned choice.
No original module, RAM reader or Dolphin dependency is present in this API.

## Original evidence and confidence

Game: NFL Street, GameCube GNNE69 revision 0. ISO SHA1
`813c4511ba3135059a13de424555647e8252254b`; DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Reference snapshots are
position-neutral14/15/16, from the Dolphin 2603a saved-match captures.
Verification runs the untouched original module offline; these are controlled
source comparisons, not newly observed live gameplay transitions.

| Source input | Primitive / size / endian | Numerical role and reader |
| --- | --- | --- |
| Actor+3 | uint8 /1 | Exact comparison with 1; 80062C30 |
| Actor+2 / global argument | uint8 /1 and uint32 argument | Equality comparison; global selector is supplied, not given a football meaning |
| Actor+21F | uint8 /1 | Accepted values 0,1,2,3,6,7,31,22,23; other values return key 1 |
| Actor+408 | uint32 /4 /BE | Exact comparison with 4 in different-selector branch |
| Actor+B65 | uint8 /1 | Maps 2→12,1→14,3→11,4→10,all others→13 |
| Actor+310 pointee byte0 | pointer /4 /BE then uint8 /1 | Exact callback tag 18 shortcut |
| Result of 80062834 | integer predicate | Written to output flag, nonzero selects key 7 |
| Result of 8006510C | integer predicate | Queried lazily after zero prior predicate and callback tag other than 18; nonzero selects key 3 |
| Actor+4C8 / result of 8004D838 | uint32 /4 /BE | Special-status test for 3/4 |
| Actor+4D3 / result of 8004DA10 | uint8 /1 then integer predicate | Enables special 3/4; disabled overrides key to1 for statuses 22/23 |
| Descriptor+3C/+3D | uint8 /1 each | First/second hierarchy keys, written by 6337C and the native resolver join |

Confidence is **verified for tested original key/flag decisions and joined
curve payloads**. No gameplay state names or ownership meanings are assigned
to these opaque values. In snapshot 14 the reference actor 0x803C19C0 has kind 1,
side 1, status 0, selector word 0, variant byte 2 and captured keys 0C/13.

The function preserves the incoming flag on its early returns. Same-selector
branches set it from 62834; a true result produces flag 1, false produces flag 0.
For statuses 22/23, a special word 3/4 plus disabled predicate overrides the key
without changing that flag. These details matter when generating the second key.

Dependent producers remain outside this component. The tests obtain their
actual results by executing 62834,6510C,4D838 and4DA10, restore the starting
RAM, then execute the complete original 62C30 decision. The native decision
receives those facts. This is not a native reconstruction of all their callers
or side effects. In particular, 6510C can mutate actor flags in other branches;
those effects are not reproduced by passing its boolean result.

The nonzero 6510C-result branch is present from source analysis, but its producer
was not forced to a naturally occurring true result in these snapshots. Its
broader gameplay interpretation remains probable. The callback 18 path selecting
the same key 3 is directly checked against the original.

## Joined source lookup

`nflstreet_variant_resolve` computes the first key, combines it with the supplied
second key, and calls the existing bounded group 4A hierarchy resolver. It then
stores both keys in the 64-byte manager payload. The resolver reads owned source
bytes and opaque identities. The current checks supply second key 13; generation
of that key by 80062E2C remains unfinished.

## Checks and reproduction

Build with `build_windows_locomotion_selection.sh`.
`variant-original-check.exe` takes the original DLL and a private MEM1 snapshot.
All snapshots, binaries, logs and diagnostics remain on W: under
`Daywater/research-temp/nflstreet-variant-first-20261009`.

Three contexts match 9,234 complete original key/flag decisions, including all
256 status bytes, equal/different selectors and variant bytes 0..5. Six additional
controls per context exercise non1 kind, different-selector word 4, true 62834,
callback 18, special-status disabled override and enabled retention. Observed
key/flag pairs are 2/91,8/91,7/1,3/0,1/0,12/0. The initial sentinel flag 91 verifies
that early exits leave the caller's byte untouched.

144 joined hierarchy payloads match the original 64-byte installation: compute
first key, run actual 80175AF8/800211D8 through the hierarchy, execute 80062340,
and compare the complete payload with the native join. Missing-service and
missing-asset rollback checks also pass. No Windows rendering integration is
claimed here; the existing viewer and launcher are unchanged.

Private verification JSON records unchanged baseline hashes: original DLL SHA1
`dd521cc38b0db6bca98a0bb261570a2be5e6e0ed`, runner SHA256
`3f9e3a3eb4b5728d9036e9552c01e8c9dc084a1f53805fef95dcf3b1b86f73ea`.
No original DLL/source mutation or restoration was required.

Next: reconstruct the needed predicate producers and second-key generation,
then connect supported changes to the native animation controller. Other players,
contacts, ball gameplay and fresh match startup remain unfinished.

The subsequent [second-key and native predicate milestone](variant_second.md) computes both keys and joins them to curve installation. The remaining special predicate, broader search branches and visible set changes are still unfinished.
