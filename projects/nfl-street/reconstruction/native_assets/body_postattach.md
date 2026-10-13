# Fresh owned body binding

`body_postattach.c/h` owns a fresh body file selected by the verified TBYP/key
and variant mapping, relocates it at an explicit host token, attaches a private
zero-initialized 52-byte view and applies the reconstructed `801C6B38` binding
to a supplied initialized skeleton resource. The model, view, target and
allocator spans must be disjoint. Failure preserves caller output and closes
the private file. Close clears ownership and can repeat.

The allocator wrapper rejects malformed/colliding storage and latches all
allocation failures. The original first-map-allocation fallback can report
success after clearing the name table; this host owner requires complete
binding and rejects that fallback. This strict failure contract is native host
architecture, not a claim about the original actor caller's error policy.

## Evidence — 2026-10-13

**Verified controlled original comparison**, GameCube GNNE69 revision 0, using
the untouched original module/DOL identified in the common baseline reports:
TBYP 0/1/2/7 × variants 0/1/2 gives 12 cases. Each case invokes actual 801C6B38
on independently decoded/relocated body data and explicit fresh target resource 0,
then repeats the bind as an already-bound no-op. Per process, 24 original calls
match complete mutated body files, 52-byte views, target bytes, 36 temporary
allocation requests, full supplied storage, release order and repeated no-op.
Three saved Dolphin 2603a contexts repeat twice in fresh Windows processes.
The allocator/free services are supplied; target selection and whole actor
startup are not replayed.

Sixteen guards cover each occupied output plus all three allocation failure
points and an overlapping model/view token range. Failure leaves the target
unchanged and balances successful callback allocations/releases. No sanitizer
or general heap-fault injection is claimed.

An exploratory comparison initially failed because its uninitialized view
contained unused words that `model_view_attach` intentionally preserves. The
fixture now explicitly starts both views at zero, matching the owned-body
constructor. This initialization is part of the tested boundary; it does not
claim to recover the full original descriptor allocation/initialization policy.

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus \
  python3 projects/nfl-street/reconstruction/native_assets/verify_body_postattach.py
```

Private report:
`W:\Daywater\research-temp\nflstreet-body-postattach-20261013\verification.json`.
Protected original DOL/DLL/runner, private inputs and source fingerprints remain
unchanged. No original buffers are patched on disk. No live Dolphin integration,
frame-dependent skinning matrices or fresh rendered actors are claimed.
