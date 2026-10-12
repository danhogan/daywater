# Bounded original field commit check

`field_commit_original_check.c` executes the original `8011F170` body with
selected Quick Game mode enabled and compares its field key and copied roster
formation arrays with the native handoff snapshot produced by
`team_field_picker_join_probe.c`.

The checker reads `r13-0x6E10` as the DOL-backed eight-byte UI-slot order and
sets `r13-0x6E04` to the slot in the native selection JSON. In the tested
snapshot, slot 1 maps to raw field key 1. It seeds the two selected native
14-word formation arrays at the original source global `0x802A4430`. Original
`80027270` and `8003F7F0` execute; the resulting raw field key and both arrays
are compared at the original `801EA480` call boundary.

The check passed on saved contexts 14, 15, and 16. It also observed the
`801211D4`, `8014C148`, `800040F0`, and `8018BAA8` boundaries in source order.
Those boundaries are captured by the harness. `801EA480` is captured without
execution, so this verifies selected-input transfer only; it does not verify
field cache/profile population, session creation, or match startup. The
comparison covers the selected slot/key and formation arrays in the supplied
native snapshot, not every selectable field or roster combination.

Build with `build_windows_field_commit_original_check.sh`; output is kept on
private W:. The checker takes the original DLL, a saved RAM context, and the
native `field-picker-selection.json` snapshot.

Confidence: **verified controlled selected-input transfer** for the supplied
slot1/key1 snapshot and saved contexts14/15/16. The original protected DLL and
DOL are unchanged. `verify_field_commit.py` builds and repeats all three checks,
recording input/executable hashes and protected baseline hashes on W:.
