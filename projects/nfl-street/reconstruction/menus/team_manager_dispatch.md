# Team manager source event dispatch

The controlled team manager checker runs the original `D094C` dispatch through `CF34C`, `D399C`, and the script VM for translated source events 2–5. It covers contexts 14, 15, and 16 with two explicit archive-flag results (`0` and `2`). The original manager registration path `D0264(0, 7)` produces a one-row `(group 0, screen 7)` registration.

For event 4, all six runs execute one `CF34C` dispatch, one `D399C` VM entry, 58 class callbacks, four router hits, and four `E9D4` game-callback calls. The checked callback frame is property `0x08`, argument block `0x817f241c`, and argc `3`. With archive flags `0`, the source takes two release/acquire paths and invokes the flush boundary twice. With flags `2`, it takes the alternate reuse callback twice and does not release or acquire those entries. Events 2, 3, and 5 each execute one VM entry and one `E9D4` callback.

## Controlled boundaries and limits

The checker also compares source bytes after original manager events 2–5 against the native pre-input fixture and its per-event snapshots. In each of contexts 14, 15, and 16, all 253,987 source bytes matched both before input and after each event. The prepared source was loaded from a materialized RAM copy; `D0264(0, 7)` registered it while original relocation/construction (`CF8C8`/`CE994`) was skipped. Each event starts from a fresh copy of the prepared state, matching the fixture's independent event snapshots rather than claiming a sequential input path. The ten-word controller mapping was seeded from the fixture's explicit logical-port mapping.

This remains a controlled source-dispatch stage, not a full original startup or full navigation check. It does not exercise the `D0E18` focus transition, map physical controls, or join the ordinary archive loader to the prepared source.

The checker supplies the file resolver at `8012AC68`, font/resource lookup, async source lookup at `801AB4B8` (returning zero while recording arguments and the saved status word), archive flags at `D5AEC`, the `801878B0` flush, resource release at `D5918`, and acquire at `D54A4`. These boundaries let the original source/event routines and VM execute while avoiding live OS or archive I/O. They are explicit supplied results; the checker does not claim those underlying services were independently reconstructed. It does not map physical controls, test `D0E18` focus transitions, or demonstrate the default resource loader.

`801AB4B8` had to be supplied in context 15 because its saved status word is `0x00010101` and the original path starts an asynchronous request that waits in the OS scheduler when run without a service. The checker does not change scheduler state. All three contexts pass after returning a supplied zero from that function.

## Reproduction and evidence

Build with:

```sh
/bin/bash projects/nfl-street/reconstruction/menus/build_windows_team_manager_checks.sh /mnt/w/Daywater/research-temp/nflstreet-team-input-20261011
```

The executable and private logs are under `W:\Daywater\research-temp\nflstreet-team-input-20261011`. The six controlled-boundary logs are named `team-manager-dispatch-context{14,15,16}-flags{0,2}.log`. Prepared comparisons are named `team-manager-native-compare-context{14,15,16}.log`. The prepared run used the untouched original module, the matching saved RAM snapshot plus its materialized fixture copy, the source fixture at `uis_all/00000/00007.bin`, and the private unpacked game data.

The original DOL SHA-1 is `46e3de854998a0f66c9be8c72a1164d22cd29eb4`; the original recomp DLL SHA-1 is `dd521cc38b0db6bca98a0bb261570a2be5e6e0ed`. These match the earlier before/after baseline record. The DOL, DLL, runner source, RAM snapshots, and original menu source were read-only inputs. Their current hashes are recorded in the private JSON summary beside the logs. The six runs finished with exit status zero.

Confidence is **verified for byte-for-byte source equivalence in this prepared manager-dispatch stage across the three saved contexts**, with the stated registration and resource-service boundaries. No claim is made about visible UI behavior, a sequential navigation path, physical input mapping, or full startup.
