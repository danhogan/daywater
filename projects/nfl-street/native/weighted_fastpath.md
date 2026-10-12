# Guarded weighted-pair experiment

The [weighted pair reduction](../reconstruction/weighted_pair_reduction/README.md)
is a source candidate for the 21-instruction function at `0x801FFEBC` in
NFL Street GNNE69 revision 0. Its gameplay role remains unknown. The
guarded implementation models paired registers, FPSCR classification,
memory writes, PC, and cycle charge for finite values with GQR0 zero and
disjoint input/output buffers. Unsupported cases use the original
generated instructions.

The offline checker matched full CPU state and touched RAM on 100,000
synthetic calls. It varied incoming `f6` and `f12` high lanes because
`ps_sum0` retains those lanes in this routine. A separate Windows
diagnostic DLL compared cloned candidate calls with the original
instructions in a saved match: **32/32 early** and **32/32 after call
10,000** matched, with zero skips or mismatches. Both runs exited cleanly;
the later run observed 600 frames. Private logs are on W: as
`nflstreet-weighted-live-diff-early.log` and
`nflstreet-weighted-live-diff-late.log`.

The [replacement staging helper](stage_weighted_fastpath.py) built a
separate DLL with the guarded candidate in the normal dispatch path. It
completed a 600-frame Windows saved-match run and exited with code zero.
The measured 23.81 FPS is from one smoke run, so it is not a speed
comparison. The experimental DLL and log remain on W: as
`gGNNE69-recomp-weighted-fastpath.dll` and
`nflstreet-weighted-fastpath-smoke.log`.

The probe and replacement staging helpers were reverted after testing.
The private generated source matches its pre-experiment copy, and both
the module-build and bundled baseline DLLs have SHA-1
`dd521cc38b0db6bca98a0bb261570a2be5e6e0ed`.
