# Fresh SKL1 skeleton resources

`skeleton_resource.c` decodes an independently supplied `animdata.dat` entry,
owns its bytes, and reconstructs original `8017EB24` relocation. It relocates
header pointers, tree-node references through `8017EA74` / callback `8017EA30`,
and name-table references, sets the relocated flag and makes repeated calls
no-ops. Token identities are supplied by the caller; no RAM snapshot or
original module is needed by this loader.

## Evidence and confidence

**Verified controlled behavior:** entries95,96,97,99,100,101 match the complete
original relocated files and repeat calls, in contexts14/15/16 twice in fresh
Windows processes:36 complete resources and36 repeated no-ops. No relocation
callback is replaced in the original comparison. Sources, archives, snapshots,
protected DOL/DLL/runner and executable hashes are recorded by the verifier.

```bash
python3 projects/nfl-street/reconstruction/native_assets/verify_skeleton_resource.py
```

Report: `W:\Daywater\research-temp\nflstreet-skeleton-resource-20261012\verification.json`.
Game: NFL Street GameCube GNNE69 revision0. DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Comparison CPU contexts originated
from Dolphin2603a; this is not a new live emulator test.

`80016180` registers `animdata.dat` through `r13-50BC`. `80041108` acquires
these six indices and invokes `8017EB24`; this acquisition meaning is
**probable from original code**, separately from the tested relocation helper.
Entry95 is a29-record target used for the face binding checks. Its raw header
is SKL1 /0000001D /000007D4 /00000590. Original names include main,rhip,rknee,
rankle,rball. The native checker supplies the same fresh file to both sides.

All multi-byte fields below are big endian; offsets are relative to the owned
resource or node, not fixed global RAM. Confidence is **verified within the six
files and repeat procedure above**:

| Field | Primitive/size | Reader/writer | Interpretation |
|---|---|---|---|
| Header+4 | u16/2 |8017EB24 | Relocated bit8000 |
| Header+6 | u16/2 |8017EB24,8017EBD0 | Name-record count |
| Header+8,+C | u32/4 each |8017EB24 | Tree root and name-table references |
| Node+0,+1 | u8/1 each |8017EA74 | Child arity and traversal threshold/index |
| Node+4 | u32/4 |8017EA30,8017EB24 | Relocated reference; root overwritten with self |
| Node+8+4i | u32/4 |8017EA30,8017EA74 | Child references |
| Name row20i+0 | u32/4 |8017EB24 | Relocated reference |
| Name row20i+4 | null-terminated bytes |8017EBD0 | Bone name |

Native preflight bounds the tree, rejects cycles and depth beyond256, and
validates references before mutation. Those rejection limits are defensive
native behavior, not claims about original malformed-input handling.

## Limits

This loader does not reconstruct the extra scale, position or other skeleton
edits made by `80041108` and its helpers, nor skeleton pairing or animation
activation. Successful face binding to raw-relocated entry95 does not prove the
complete gameplay skeleton state. Hair, textures/materials and fresh actor
startup still need connected ownership and verification. Bytes and diagnostics
remain private on W:.
