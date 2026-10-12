# Controller source screen — 2026-10-10

## Verified controlled stage

Game: NFL Street, GameCube GNNE69 revision0. Reference captures: Dolphin2603a
saved match contexts14/15/16. The checker supplies a scratch menu manager and
executes source menu assets; it does not restore a live original menu.

The controller source screen now has an original-module differential check for
class construction, child-first creation, selected-root activation, and 160
timed updates. Across saved contexts 14, 15, and 16, the check compares the
complete 38,682-byte owned source, entry flags, scheduler records, controller
selection/device state, and captured requests. It executes the original class
callbacks, VM, and `8011BC68` controller callback from the untouched original
module.

Per context, the comparison covered:

- 152 ordered class callbacks and all 38,682 source bytes after construction.
- 12 child-first `FFFE` scripts and one selected-root `FFFC` script.
- Ten scheduled records and 160 updates, comparing all 13 words in each
  scheduled record, scheduler flags, controller state, and source bytes.
- 2,601 device requests and one outgoing broadcast (`D(5)`). Device requests
  are counted and drained between updates; no request is silently dropped.

Confidence: **verified controlled source-screen stage across three saved
contexts**. The inputs are supplied: font lookups for entries 0–3, static
archive flags0 for IDs 1, 3, 9, and 10, profile values, and device status
responses. Class dispatch order is compared. Font/resource lookups are supplied and
constructor acquisition calls are captured; native resource I/O is not exercised.
This does not verify resource streaming or profile database I/O. The tested
logical bindings are identity bindings for the present port. General common
callbackA/direct lookup and139/inverse lookup follow the original ten-entry
functions (`80129AB8`/`80129ACC`); wider binding arrangements and input uses
remain outside this screen-stage comparison.

The native `construct`, `start`, and `advance` APIs join those checked stages.
The public startup join matches the separately checked source stages in all
three contexts. A supplied unsupported archive causes atomic rejection with
identical prior screen bytes and state. This establishes the bounded startup
join; a full visible screen lifecycle is still unfinished.

## Verified controlled input stage

The original checker now compares five translated source events in each of
contexts 14, 15, and 16: events 2, 3, 5, 4, and 7, each followed by 20 timed
updates (100 updates per context). For each event, the checker compares the
complete owned source, controller and selection state, scheduled records,
requests, and flags against the original module. The tested frame supplies
logical port 0 and float value 1.0. This matches the source dispatcher frame:
result slot, port, float word, root, and trailing zero. The upstream physical
controller translation, port discovery, and repeat behavior are not tested.

The sequence exercises profile cycling, neutral and side selection, and a
delayed completion broadcast with arguments `(0xFFFFFFFF, 0xFFFFFFFF)`. The
original and native paths also produce the linked-screen request to screen 5.
The completion result is source-generated and differentially checked, but the
checker observes no committed profile write (`committed.count` remains 0).
Confidence: **verified controlled source-input stage across three saved
contexts**; this does not establish visible input or a full match transition.

The independent native probe continues from the source completion request to
the Quick Game coordinator and issues a team-screen 7 load/activation request.
This verifies the native request handoff only. It does not verify the team
screen visibly opening, team or field selection, or selected-match startup.

The input wrapper supplies false responses for source properties `0x104` and
`0x105`, matching the controlled false-result hooks for original routines
`80009E5C` and `80009E6C`. Its bounded `0x1EF` service accepts raw values 0 or
1 and maps nonzero to the original float comparison before capturing the
`801278B4` request. These are explicit supplied boundaries, not Windows device
discovery or physical input translation.

## VM byte-descriptor operations

VM opcodes `4A` and `4B` are compared with the original VM on all three saved
contexts. Each context passes 256 exact file/stack/PC/flags comparisons and 11
atomic unsupported, budget, or invalid-descriptor guards. `4A` sign-extends
the selected byte; `4B` writes the low byte. Null and zero-capacity descriptors,
capacity boundaries, signed bytes `80`/`FF`, and `UINT_MAX` indices are covered.

## Independent diagnostic controller rendering

`controller_preview.c` uses local source assets and the checked screen APIs,
with four source fonts and source archives1/3/9/10. Original registration
`80166344` identifies archive9 as `uis_ctrl` and10 as `uis_fron`; the preview
loads all13 button entries needed by the controller artwork. It reuses the
existing Options affine host rasterizer, without changing the Options program.
Six captures cover startup and the checked five-event input sequence. Native
capture execution completes the delayed source broadcast/link5 handoff. The
runtime does not load an original DLL, RAM snapshot, Dolphin or ModernGekko.

Startup, neutral and selected-side captures were visually inspected: original
controller artwork, prompt text and profile display appear; the controller
moves to the center when neutral and returns to the selected panel. Profile
labels/device presence are supplied inspection values. This is diagnostic
rendering, **not verified original GX rasterization** or a fresh game startup.
The Windows window adapter builds, but manual desktop interaction has not been
exercised. Existing launchers and the main inspection are unchanged.

`controller-preview.exe raw-controller.bin unpacked-root [--captures W-prefix]`
opens the inspection or writes the six captures. Host arrows/Enter/Backspace
are numeric source-event shortcuts for logical port0; they do not establish
GameCube hardware mappings. Escape closes the host window.

## Limits and next gates

This is a controlled original-code comparison, not live Dolphin menu
navigation. It does not show the source screen visibly rendered or activated
in the game, discover physical Windows controllers, establish a controller
mapping, or create a selected match. Translated input and completion now match
in the controlled supplied context. Remaining work is main-window integration,
physical input/device discovery and original profile data, followed by team/field
selection and selected-match startup.

## Reproduction and baselines

`verify_controller_screen.py` automates the strict Windows build, original
checks against contexts 14/15/16, translated input sequences and timed updates,
independent native probe and six rendered captures, asset/capture provenance,
and pre/post baseline hashes. `--no-build` reuses the
existing executables. Private outputs, logs, context/source/font hashes, and
the full verification record are under:

`W:\Daywater\research-temp\nflstreet-controller-start-20261010`

The verified baseline hashes are:

- `main.dol` SHA-1: `46e3de854998a0f66c9be8c72a1164d22cd29eb4`
- Original `gGNNE69_recomp.dll` SHA-1:
  `dd521cc38b0db6bca98a0bb261570a2be5e6e0ed`
- `moderngekko_run.cpp` SHA-256:
  `3f9e3a3eb4b5728d9036e9552c01e8c9dc084a1f53805fef95dcf3b1b86f73ea`

The original executable and source assets remain private; no original module or
baseline file was modified by verification.
