# Fresh view and callback records (row07 payload slice)

NFL Street GNNE69 North America rev0; DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Confidence: **verified within the
controlled original-DLL boundary** when the private full report passes. Dolphin
build provenance of these existing regression captures is not established here.

`view_nodes.c/h` reconstructs view creation801DF14C, callback registration
801DF39C, ordering leaves801DEF40/801DEE3C, display-word reader801879B4 and
literal zero-return801DF8B4. It reuses unchanged list_nodes/scene_owner/object_pool.
The original functions execute unhooked in the gate, including actual ordering
callbacks; only external heap/error/geterror services return early.

The context borrows RAM, type0 list context, global list-pointer slots, display
word and32constant bytes. Original bindings: viewslot r13-638C (803A36B4),
callbackslot r13-6388 (803A36B8), display8039E07C, constants80263CC8.
This slice constructs neither the global dispatch table nor the lists' owners;
the gate explicitly uses the existing list constructor and fixture publications
for those slots. Bootstrap801DEFD8 and final entry wiring remain separate.

## Writes and reads

All fields below are BE unless a single byte; pointers/words/F32 occupy4bytes,
U16 occupies2bytes. List-node headers remain governed by `list_nodes.md`.
These fields are verified only through the full procedure below, with the
stated game revision, original DLL, captures and unknown Dolphin build.

| View payload offset | Type | Original write/read meaning |
| --- | --- | --- |
| +00 | U32 | zero-extended descriptor U16+00; first-word ordering key |
| +04 | F32 | zero constant |
| +08 | F32 | ((F32 signed display word)-(F32 descriptor U16+10))*half, rounded per instruction |
| +0C/+14 | F32 | descriptor U16+12 converted via unsigned double bias |
| +10/+18 | F32 | descriptor U16+10 converted via unsigned double bias |
| +1C | U32 | 1 |
| +20/+24/+28 | F32 | zero constant |
| +2E/+2F | U8 | descriptor countbyte+3 |
| +30 onward | U32 array | count words from descriptor+4, copied before later default stores |
| +3C/+48 | U32 | 0, overriding copied words if they overlap |
| +44 | U8 | 0, overriding only that copied/preserved byte |
| +F0 | F32 | one constant, overriding a copied word if count reaches it |

Remaining F8-byte payload stays dirty. The count is not inferred as a camera
count: the host supports0..50 for bounded copying; original has no such guard.
Descriptor and destination must be disjoint/stable. Fields D8..EC used by the
existing projection consumer are not initialized by this routine.

Signed display conversion reproduces the xoris/double-bias instruction sequence;
U16 conversions use a different bias. Subtraction and multiplication each round
to binary32. Native builds disable FP contraction. The constant block is pinned
from DOL offset260CC8 and all captures: zeroF32, signed bias4330000080000000,
unsigned bias4330000000000000, half3F000000, one3F800000; padding is retained.
No broad display-mode semantics are inferred from the accessor's one-word read.

Creation allocates, writes defaults, publishes through mode0 list insertion,
then calls original801DF8B4 (li r3,0;blr). It returns the new payload. Exhaustion
returns0 without geterror. Callback registration allocates a16-byte payload,
writes {arg4,arg2,arg3,arg1}, publishes it and returns0. On exhaustion it calls
geterror and returns the exact result, including0 or highbit values. None of
these stored callback identities is invoked as a runtime callback by this slice.

Both ordering leaves return the signed interpretation of wrapping
left.firstword-right.firstword. This differs from unsigned comparison when the
subtraction overflows. Missing cached leaves were decoded from matching DOL
and capture bytes, not guessed: DOL1DBF40/1DBE3C, each16bytes
`80030000806400007C6300504E800020`.

## Procedure and limits

Full mode compares contexts14/15/16 twice,401cases/process: original saved-list
callback registration plus400 edited cases with fresh F8/10-size lists, dirty
payloads, copied-word counts0..50, overlapping default stores, U16 dimensions,
signed-display extremes/rounding edges, ordering/ties/wrap, exhausted lists,
heap0 and exact error0/highbit returns. Four direct comparator checks/case
compare real returns with no RAM exclusion. Eight malformed-host cases are
separate, without attributing those guards to original game validation.

Ordered heap/error/geterror observations include both bounded dirty arenas.
After every operation, actual returns and all24MiB compare except exactly:

- List creation/view creation `[80F46F90,80F47008)`: maximum112-byte nested
  frames plus8-byte caller LR slot. View48 + publisher16 + insertion32 + link16.
- Callback registration `[80F46FA0,80F47008)`: same publish chain under32-byte
  callback frame,96nested bytes plus8-byte caller LR slot.
- Leaf ordering: no exclusion.

Only the just-compared stack interval is synchronized before a narrower next
phase. No list, node, globals, descriptors, payload or padding is masked. Error
service clobbers r3 to test saved returns. Eleven production mutants must fail
through RAM/return/service discrepancies; build errors, host rejections,
crashes and timeouts never count. Fast mode is context15,40edits,no mutants.

This does not prove scene binding, field/HUD type constructors, complete
camera projection, viewport-stack publication, bootstrap, teardown or live
startup. The existing live snapshot launcher is unchanged. Borrowed structures
must be aligned, bounded, disjoint and stable during services; rejection may
follow original writes/services and does not roll back. The native API returns
host status separately from the guest result. See prior list/pool contracts.

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_view_nodes.py
```

Private evidence/builds/logs:
`W:/Daywater/research-temp/nflstreet-view-nodes-20261018/{fast,full}/`.
