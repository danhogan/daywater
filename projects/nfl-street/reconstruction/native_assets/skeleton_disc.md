# Disc-backed six-resource skeleton input owner

This row 03 slice replaces the runtime NSC1 dependency for existing skeleton
startup. `skeleton_disc.c` is host data preparation and composition with the
unchanged `skeleton_startup.c` consumer. It adds no original routine reconstruction.
Confidence is candidate unless the private full report matches current source
hashes, then verified within the composition contract below.

## API and ownership

`nflstreet_skeleton_disc_open(iso)` reads the pinned GNNE69 rev0 main.dol through
`disc_dol.c`, resolves constants from initialized sections, copies them into the
source owner, and reads `animdata.dat` using existing `disc_files.c`. It releases
the DOL and all nod handles before returning. Wrong DOL revision, missing files,
invalid header/ranges/archive, allocation failure or null input rejects creation
and releases partial construction. There is no generated catalog or snapshot
runtime input. The existing disc reader's TERF decoding remains unchanged.

`nflstreet_skeleton_disc_catalog(source, &catalog)` publishes a borrowed view
valid until source close. `nflstreet_skeleton_disc_startup(source, identity, &out)`
forwards that view and owned archive to `nflstreet_skeleton_startup_open`.
Successful startup owns six fresh decoded resources independent of the source;
close it using `nflstreet_skeleton_startup_close`. Failed startup leaves output
unchanged. Startup output must be zero-initialized or closed; nonempty output
is rejected so repeated calls cannot overwrite owned resources. Close the input owner with `nflstreet_skeleton_disc_close`, accepting
null. Callers must not copy/free the opaque source or use catalog pointers after
source close. APIs are not synchronized for concurrent source close/use.

```c
nflstreet_skeleton_disc_source *source = nflstreet_skeleton_disc_open(iso);
if (!source) return 0;
nflstreet_skeleton_startup bundle = {0};
int ok = nflstreet_skeleton_disc_startup(source, identity_base, &bundle);
nflstreet_skeleton_disc_close(source);
if (!ok) return 0;
/* Use all six owned resources and six vector words. */
nflstreet_skeleton_startup_close(&bundle);
```

## Evidence and scope

Addresses come directly from `export_skeleton_startup_catalog.py` and the
previous complete original `80041108` verification documented in
`skeleton_startup.md`: 384 bytes at `80246490`, 12 at `80246610`, 114 at
`8024661C`, BE32 scale at `80246690`. Game is NFL Street GameCube GNNE69 rev0,
DOL SHA1 `46e3de854998a0f66c9be8c72a1164d22cd29eb4`. All spans must fit an
initialized section. These bytes are original constant tables; the adapter does
not assign additional game meanings or embed their contents in source.

The new gate first checks an independent private NSC1 export against a raw pinned
DOL parse, then compares adapter fields and all six downstream initialized files,
resource identities/indices and six vector words against the existing consumer
fed independent animdata/NSC1 exports. Full mode runs contexts 14/15/16 twice in
fresh Windows processes, each with 201 distinct valid identity bases and 1,206
complete output resources, eight failure guards and output-after-source-close.
Snapshot bytes seed identity selection only. Fast mode uses context 15 once,
11 cases, no mutants. Seven production mutants change table origins, scale,
identity forwarding, archive selection and nonempty output rejection. Build failures/crashes cannot count
as catches; exit 4 plus explicit comparison diagnostic is required.

No new original-module execution, all-RAM original comparison, emulator run,
rendering, live startup join or gameplay policy is claimed. Earlier original
skeleton proof remains separate; the gate verifies new input composition.
Other constants catalogs and mutable match startup remain open.

Run from the repository root:

```sh
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 -B projects/nfl-street/reconstruction/native_assets/verify_skeleton_disc.py
```

Private inputs, build outputs and evidence remain under W:, with report at
`/mnt/w/Daywater/research-temp/nflstreet-skeleton-disc-20261017/verification.json`.
The gate pins ISO/DOL revision and hashes snapshots, independent exports, nod
provider, all transitive source/header files, executable and mutant evidence.
No existing consumer/header/gate changes are needed.
