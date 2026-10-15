# Disc-backed native files

This is a **host I/O choice**, not a reconstruction of a GameCube DVD routine.
`disc_files.c/.h` uses nod's C API directly (the library underneath Aurora's
DVD API). This avoids Aurora's global DVD state, worker threads and window
initialization for an independent byte check. Aurora's existing `nod::nod`
CMake dependency is exported by `aurora::dvd` to the live app. No game addresses,
floating-point arithmetic or gameplay semantics are introduced.

`nflstreet_disc_open(iso)` owns a disc and data-partition handle;
`nflstreet_disc_read(d, game_path, &bytes, &size)` returns caller-owned bytes;
`nflstreet_disc_entry_count(d, "animdata.dat", &count)` queries the archive;
`nflstreet_disc_close(d)` frees the cached archive and nod handles. A caller
must serialize operations on one handle. Failed reads preserve both outputs.
Zero-length decoded entries succeed and return an allocation that can be freed.

FST paths such as `animdata.dat` return exact file bytes. Virtual paths such
as `animdata/00111.bin` map to `animdata.dat`, DIR1 entry 111, decoded through
the existing `archive.c`/`lzh1.c` (TERF codec 0 or 5). These are extraction
conventions, not actual FST filenames. Leading slash and backslash separators
are accepted. Drive names, traversal and empty components are rejected.
Only the last archive is cached, to avoid rereading a whole DAT per entry;
the separately allocated decoded banks survive disc close. No file is written
or extracted by this API and no extracted-file fallback occurs.

## Verification

The implementation and checker cross-compile with Zig for Windows. Runtime
evidence is **candidate until the external Windows gate passes**; consult the
private `verification.json` for its current status. This agent is restricted
to writing/compiling and does not run Windows executables.

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_disc_files.py
```

Private evidence/builds remain under
`W:/Daywater/research-temp/nflstreet-disc-20261015/`. The gate checks the
preserved DOL/module baselines, the original ISO SHA1 from
`tools/extract_native_assets.py`, all input hashes before/after and source
hashes with `native_assets/` prefixes. It builds the Windows checker and
compares every `unpacked/<archive>/NNNNN.bin` byte and every whole extracted
DAT against nod reads, in two fresh processes for each saved context 14/15/16.
Each process additionally checks 401 context-seeded animation bank reads
across slash variants, archive entry count and invalid-path/output contracts.
The context RAM is read and fingerprinted; no game routine runs on it.
The JSON list `disc_files_mutants.json` supplies eight one-line source edits;
the gate builds each in a private scratch directory and requires comparison
exit 4. Build errors, crashes and ordinary I/O setup failures do not count.

The extraction tree also contains `unpack.log` and `unpack-report.json`.
These are generated host metadata (some logs include CMD/WSL diagnostics),
not game files; claiming identical disc reads of them would be false. The
gate classifies, inventories and hashes every such file, reports their count,
and rejects any other unclassified file. The private report records all
payload path mappings and exact comparison counts.

## Integration and limits

`native/live/live_main.c` opens `NFL_LIVE_DISC` before animation setup, loads
all `animdata.dat` entries into the existing `anim_files/anim_sizes` arrays,
and calls the unchanged `nflstreet_anim_aram_map` and channel setup. It closes
the disc after loading. Disc mode takes precedence over `NFL_LIVE_ANIMDIR`;
the existing directory branch still scans its 200 slots. Bad disc input or
an unsupported bank count fails startup instead of falling back silently.
The live CMake target adds `disc_files.c`, `archive.c`, `lzh1.c` and `aurora::dvd`
(which supplies nod transitively, without using Aurora's global DVD API).

The default native animation path needs no `NFL_LIVE_ANIMDATA` or
`NFL_LIVE_SETS`. For the explicit `NFL_LIVE_HOST_LOCOMOTION=1` inspection
fallback, disc mode supplies `animdata/00111.bin` instead of `NFL_LIVE_ANIMDATA`.
Its `NFL_LIVE_SETS` remains an external **host-generated NSC1 catalog**: no
equivalent exists on the disc. It is never represented as a disc asset.

No change to `scene_frame.c` is needed: it receives the same prepared scene
and RAM. A new consumer can open one disc at startup, call
`nflstreet_disc_read(d, "<archive>/<five-digit-entry>.bin", &b, &n)` at its
existing extracted-file load, pass `b,n` to its existing resource parser,
then free `b` after the parser no longer borrows it. Keep the handle open
while loading adjacent entries and close it after loading.

Not modeled: game boot/state creation, original DVD command timing,
asynchronous streaming, ARAM scheduling, additional codecs, archive hash-name
lookup in this API, and removal of the RAM fixture. The RAM snapshot is still
required for game state. The byte gate does not verify the live window,
rendering, gameplay or original-module all-RAM effects.
