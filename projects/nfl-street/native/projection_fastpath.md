# Guarded projection experiment

The [three pair projection](../reconstruction/three_pair_projection/README.md)
is a source candidate for the 21-instruction function at `0x801FFF10` in
NFL Street GNNE69 revision 0. Its role in gameplay is unknown. The guarded
implementation models paired registers, FPSCR classification, memory writes,
PC, and cycle charge, and falls back to generated code outside the tested
finite, GQR0-zero domain.

The offline Linux whole-DOL differential checker matched full CPU state and
touched RAM on 100,000 synthetic cases, including output/input overlap. A
separate Windows diagnostic DLL compared cloned candidate calls to the
original generated instructions on a saved match. It found **32 of 32**
matches from the first calls and **32 of 32** after call 10,000, without
skips or mismatches. The later 600-frame run reached frame 17,165 and exited
with code zero. Private logs are on W: as
`nflstreet-projection-live-diff-early.log` and
`nflstreet-projection-live-diff-late.log`.

The [staging helper](stage_projection_fastpath.py) then built a separate DLL
with the guarded candidate in the normal dispatch path. A 600-frame Windows
saved-match run reached frame 17,164, exited with code zero, and reported
23.04 observed FPS. This single run is a functional smoke test, not a speed
comparison. The DLL and log stay on W: as
`gGNNE69-recomp-projection-fastpath.dll` and
`nflstreet-projection-fastpath-smoke.log`.

After the run, the staging helper restored the original generated C. The
module-build and bundled baseline DLLs both have SHA-1
`dd521cc38b0db6bca98a0bb261570a2be5e6e0ed`. The experimental DLL is
separate from the launch bundle.
