# Field source raster preview

`field_screen_preview.c` renders a relocated field-selection source snapshot
using the existing source traversal, class0 image constructor, class8 text
constructor, font layout and texture decoder. Multi-line field text uses the
retained line plans from `team_classes`; line placement follows the existing
`card_prompt_preview.c` renderer. The separate `nflstreet_field_screen_preview_render`
entry point accepts an already constructed snapshot and its current class/font
state, so a field host can render after its own source events without repeating
field callbacks or taking ownership of their effects.

## Current capture

The standalone build was exercised on original entry8
`uis_all/00000/00008.bin` with the private GNNE69-r0 unpacked font and image
assets. It rendered 19 source nodes, 159 class0 and 11 class8 objects, one
wrapped text plan, and eight decoded image resources. The capture was visually
inspected; it contains the source field thumbnails, title/description and lock
art. The static source still shows placeholder strings and large unfilled
areas.

Private capture:
`W:\Daywater\research-temp\nflstreet-field-screen-preview-20261011\field-screen-static-entry8-v2.bmp`.

This is a host diagnostic raster of the constructed source artwork. It does
not claim original GX rendering, field-cache event completion, unlocked-field
policy, interactive navigation or selected-field match startup. The source
callback probe currently retains its own supplied cache/profile boundaries;
the render API is ready to consume its source bytes and `team_classes` state
after a successful event.

## Live host adapter

`nflstreet_field_screen_preview_live` now provides a Win32 display loop for a
caller that already owns a persistent field source VM. It forwards F2..F7 as
numeric source events 2..7 and forwards a host elapsed value of 16 to the
caller’s tick function, then rerenders the caller-owned source bytes and class
plans. Escape closes the window. These are host inspection controls, not a
verified original controller-to-event mapping. Newly referenced class0 image
archive/entry pairs are decoded from the supplied unpacked tree during redraw.

`team_field_picker_join_probe.c` now supplies the persistent field VM and
callbacks to this adapter. Startup uses the source FFFE/FFFC sequence, then the
Win32 loop can receive source events and ticks without replacing the caller’s
state. The host renders before acknowledging an exact resource-effect prefix;
audio, broadcast and route effects are retained as inspection records and are
not played or dispatched by this preview.

An opt-in deterministic Win32 self-test posts numeric events `5,4,5,7`, with 60
settled host ticks between them. It exercises the actual window message loop,
source event/tick callbacks, GDI painting and post-paint resource handling. The
reproducible gate passed: it observed source cursor 1, one completed transition
and link to screen 5, then ran the source FFFD finish and checked the native
commit request for UI slot 1/raw DIVE key 1/owner value 1. It also checked that
resource, button-resource, audio, broadcast, route and focus queues were empty
after handling. The latest self-test status recorded 278 successful
post-paint callbacks. The joined gate additionally ran six fresh cases and
validated the private selection JSON: both teams have seven unique selected
players, both 14-player formation arrays are retained, and the selected field
is slot 1/key 1.

The final numeric source event 6 is a back/cancel path: it does not execute
FFFD or write a field-selection JSON record. The final-6 negative gate passed.
These are automated Win32 messages and GDI paints, not a manual mouse or
controller session. They establish this native host path and the source-level
field handoff record; they do not establish original GX fidelity, physical
controller mapping, audio playback, route dispatch, `801EA480` match creation,
or a complete game match startup.

## Reproduce

Build the standalone raster with `build_windows_field_screen_preview.sh`, then
run:

```text
field-screen-preview.exe <original-entry8.bin> <private-unpacked-root> <W-private-capture.bmp>
```

The builder restricts executable output to `/mnt/w/Daywater`. The standalone
renderer requires Windows `W:\Daywater\...` inputs and capture output. It does
not modify source archives, DOL or DLL.

The joined host and deterministic Win32 test are built with
`build_windows_team_field_picker_join.sh`. Their executable and captures stay
under the private `W:\Daywater\research-temp\nflstreet-team-field-picker-join-20261011\`
tree. The interactive path is opt-in with `DAYWATER_FIELD_JOIN_LIVE=1`; the
automated posted-event path is opt-in with
`DAYWATER_FIELD_JOIN_LIVE_SELF_TEST=1` and a private
`DAYWATER_FIELD_JOIN_CAPTURE_PREFIX`.
