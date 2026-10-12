# Complete fresh skeleton startup

`skeleton_startup.c` owns the six-resource initialization performed by original
`80041108`. It loads animdata entries95,96,97,99,100,101, relocates each SKL1,
copies the original position records, applies scale words, mirrors the paired
records and produces the six expanded vector words. All six files are owned
until close. Failure releases partial construction and leaves output unchanged.

The selected geometry check now uses the fully initialized entry95 and101
as its face/hair binding targets. The other four prepared resources are owned
by the bundle for later actor initialization; they are not yet connected to
fresh actors. This native path consumes local DAT archives and a private NSC1
catalog, with no DOL, original module or snapshot as runtime input.

## Private startup constants

`export_skeleton_startup_catalog.py` checks the original DOL revision and exports
only the required constants into a private530-byte NSC1 data file on W:.
No proprietary constant arrays are embedded in committed source. Export is an
offline preparation step; the native runtime reads the prepared data file.

The catalog contains384 bytes from original80246490,12 from80246610,114 from
8024661C, and the scale word from80246690. A16-byte host-format header identifies
NSC1/version1 and the supported table sizes. The native parser rejects unsupported
versions or sizes. The hair helper's last iteration keeps pointers to the final
24-byte block before its18-byte tail copy; complete original comparison verifies
the resulting contiguous114-byte table interpretation.

```bash
python3 projects/nfl-street/reconstruction/native_assets/export_skeleton_startup_catalog.py
python3 projects/nfl-street/reconstruction/native_assets/verify_skeleton_startup.py
```

## Original behavior and fields

Game: NFL Street GameCube GNNE69 revision0; DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Saved comparison contexts came
from Dolphin2603a. Confidence is **verified within the complete startup calls
and independent native coverage described below**.

| Field / operation | Primitive and byte order | Original reader/writer |
|---|---|---|
| SKL1+6 | BE u16 count |80040CA8,80040DB0,8017EC48 |
| Base/hair records+410+6i | Three BE16 words per record |80040CA8 /80040DB0 copy supplied original tables |
| Hand record+416,+418,+41A | Three BE16 words |80040D5C selects one of two six-byte records |
| Resources96,97,99,100,101 +10,+14,+18 | Three BE float32 words |80041108 supplies the original scale word |
| Resource96 record+410+6i | Three BE16 words |8017EC48 copies97's first word and negates its other two with16-bit wrap |
| Resource95 +46A,+46C,+46E,+48E,+490,+492 | Signed BE16 inputs |80041108 sign-extends and multiplies by16 |
| 8026C2AC/B0/B4 and8026C2A0/A4/A8 | Six BE32 output words |80041108 publishes the expanded values |
| r13-7BD4 through r13-7BC0, step4 | Six BE32 references |80041108 publishes the six acquired resources |

Original global locations identify the GNNE69 reference, not native runtime
addresses. Native APIs accept caller identities. Vector units and gameplay
roles remain unassigned. Observed scale/vector words are recorded in the
private verification report, alongside all source/input hashes.

## Verification procedure and results

The checker executes the actual original `80041108` caller. Only archive
acquisition `801A69C0` is supplied, by decoding the fresh archive entry into the
requested comparison storage. The original relocation, tree callback, position
helpers, pairing/mirroring and scale/vector operations all execute. Native code
receives the independently extracted catalog; the checker first verifies those
constants equal the protected context's original constant ranges.

For contexts14/15/16, twice each in fresh Windows processes:

- 36 complete initialized files match, including all bytes outside edited fields.
- 36 published resource references and36 vector words match.
- The six acquisition indices, handle and flags match in original order.
- A missing final resource rejects native partial construction and preserves output.

Two fresh independent native geometry processes each pass1,206 selected
face/body/hair assemblies across402 regular-roster rows and three explicit
body variants. Prepared face/hair targets bind2,412 face models and3,618 hair
models; repeat binding creates no additional allocations. Existing namespace
collision checks remain exercised. Earlier hair binding, headwear selection
and explicit-state hair resource gates still pass after the shared checker
extension. These remain controlled startup/binding and native coverage tests,
not full original gameplay comparisons.

Report: `W:\Daywater\research-temp\nflstreet-skeleton-startup-20261012\verification.json`.
It records constants, archives,32 roster DB blocks,GEAR table, source/header,
executable and protected DOL/DLL/runner hashes. Inputs and originals remain
unchanged. Builds, generated catalogs and diagnostics stay private on W:.

## Remaining startup work

This closes the previously omitted six-resource skeleton position/scale setup.
Texture/material ownership, pool/descriptor activation, fresh constructor
inputs and resource-to-actor binding remain open before a selected match can
start. Menus and the controllable reference scene still work; no new playable
selected match or original appearance fidelity is claimed. The generic
Daywater platform is unchanged.
