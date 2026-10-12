# Supported animation sets in the native Windows player

## Result

The experimental paired player now loads a checked two-set catalog and switches
between its supported sets with **V**. WASD/arrows retain native movement;
R restores the starting set and movement seed. A request arriving during a fade
is deferred; the latest request is applied once the current fade ends. Existing
stream phase, four-channel fades, source curve choice, directional signs and
pose decoding remain connected. This is an inspection control, not a completed
automatic gameplay animation selector.

The checked pairs are `0C/13` and `0D/13`. They are opaque original keys; no new
football meaning is assigned. Their original primary/alternate/other identities
are respectively `8080537C/8080537C/80803D34` and
`808097B8/808097B8/808080A4`, all using bank key 2. The animation payload uses
the previously checked probable bank 00111 mapping. The runtime reads extracted
assets and small private profiles. It loads no original DLL, RAM snapshot,
Dolphin, ModernGekko or ISO while playing.

## Evidence and confidence

Game: NFL Street, GameCube GNNE69 revision 0. ISO SHA1
`813c4511ba3135059a13de424555647e8252254b`; DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Saved MEM1 contexts are
position-neutral-14/15/16 from Dolphin 2603a. Reference actor `803C19C0` is
specific to these snapshots. These checks execute the untouched original
module offline; they are not fresh live gameplay checks.

Catalog export changes actor+B65 (uint8, one byte) to 2 and 0 as controlled
inputs. Existing native first/second key selection and ordinary supporting
predicates calculate the keys. Original `80062C30` and `80062E2C` independently
agree. Original `80175AF8` plus the complete `80062594` installer then match
all 64 bytes of each resulting descriptor. Six installs across three contexts
agree. Native export still queries original `8006510C` and `80104238` for their
facts. Those services do not run in the independent viewer: V chooses a checked
catalog entry directly.

| Data | Primitive / size / endian | Checked role |
| --- | --- | --- |
| Actor+B65 | uint8 / 1 | Controlled first-key selection input, not a new gameplay label |
| Descriptor+3C/+3D | uint8 / 1 each | First/second set keys for original 62594 |
| Descriptor+10/+14/+18 | uint32 / 4 each / BE | Three source curve identities |
| Descriptor+37/+38/+39 | uint8 / 1 each | Associated source bank keys |
| Descriptor+3A/+3B | uint8 / 1 each | Fade duration/countdown gating the checked installation |

Confidence: **verified for controlled source installation and joined numerical
playback in these contexts**. Natural gameplay eligibility and full callback
semantics remain incomplete. Installer `80062470` also invokes actor-side
services via `8002E700`; the native catalog does not reproduce those effects.
The join explicitly models a supported request at the checked fade gate, not
all earlier instructions of the original gameplay callback.

## Verification

Three 480-update sweeps execute actual original set installation, curve choice,
directional selection, paired setup/rebind, retiring fades, blend/rate updates,
stream timing and pose-list assembly. Totals:

- 1,440 joined updates, 39 set installs and 105 deferred set requests.
- 93 pair starts, 636 fade updates, 90 completions and 15 source curve changes.
- 6,120 exact original numeric pose-list entries, including frame/slot order.
- Source curve bytes, current identity, pair, descriptor numeric fields including
  set keys, channel status and channel+30..7B match exactly. Relocated pointers
  and broader channel metadata are excluded from the numerical claim.
- Every catalog truncation, duplicate keys and malformed selected profiles are
  rejected without changing outputs/state. Reset returns to the initial set.
- The previous directional oracle still passes 320 boundary cases and 480
  joined updates in context 14.

The actual Windows V key handler passes a 60-tick recorded run with two set
changes and eight exact full actor/movement checkpoints. A 528-tick run passes
all eight starts/releases/braking/stops, four set changes and 36 completed fades.
Final outgoing channels are retired. Both runs return to the starting set.
Inspection captures show coherent skinned poses; this is not original gameplay
pixel equivalence. The default movement check passes and all five legacy
inspection BMPs remain byte-identical.

Build with `build_windows_locomotion_selection.sh` and
`build_windows_live_player.sh`. `set-catalog-export.exe` takes original DLL,
MEM1 and output NSC1 path. `set-playback-original-check.exe` takes original DLL,
MEM1, bank 00111 and matching NSC1 catalog. Viewer usage adds
`--movement <fixture> --sets <bank> <catalog>`; `--set-check` exercises V and
`--direction-check` extends movement checks.

Private outputs, logs, captures, catalogs and the hash manifest live in
`W:\Daywater\research-temp\nflstreet-set-playback-20261009`.
`W:\Daywater\Start-NFLStreet-PairedPlayer.cmd` selects this checked build.
The default movement launcher is unchanged. Original DLL SHA1 remains
`dd521cc38b0db6bca98a0bb261570a2be5e6e0ed`; baseline runner SHA256 remains
`3f9e3a3eb4b5728d9036e9552c01e8c9dc084a1f53805fef95dcf3b1b86f73ea`.
No baseline edits or restoration were required.

## Remaining work

Reconstruct the special predicate and broader target searches before claiming
original automatic gameplay set selection. Connect a second visible actor to
checked paired contact scheduling. Ball launch/catch/bounce/rendering, fresh
actor/match startup, original body orientation/root travel, menus and audio
remain unfinished. The viewer still uses a captured movement/appearance/camera
seed and its existing host time mapping.
