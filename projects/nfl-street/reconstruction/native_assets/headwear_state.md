# Automatic headwear state and selected geometry assembly

`headwear_state.c` joins the selected PLAY row to the fresh GEAR table and
reconstructs original `800DDEBC` / `800DDE18` decisions.
`selected_player_geometry.c` connects this result to the existing face/body
owner and three-slot hair loader. It owns face and hair files, borrows the
processed body, checks disjoint token namespaces and releases new files on
failure without changing caller output. Body variant remains explicit.
No snapshot, original module or Python lookup catalog is a runtime dependency.

## Source trace and field provenance

Game: NFL Street GameCube GNNE69 revision 0. DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Original comparison contexts
came from Dolphin 2603a; this is an offline module oracle, not a new live test.

Original `8014F60C` queries 12 PLAY fields from the DOL table at `80285C2C`
(`80285BE4+48`) and copies them to preview-record offsets +0C through +38.
A separate read-only review verified these original tags and destination slots:

| Input | Primitive/size, byte order | Source/readers | Confidence |
|---|---|---|---|
| PLAY raw tag54525350 (PSRT) | Numeric field, u32/4 after extraction | Field10 → preview `802AAA94+slot*68`;8014F60C,800DDEBC | Verified tag mapping; static writer chain |
| PLAY raw tag54414850 (PHAT) | Numeric field, u32/4 after extraction | Field0 → preview `802AAA6C+slot*68`;8014F60C,800DDEBC | Verified tag mapping; static writer chain |
| GEAR raw tag44494547 (GEID) | Unsigned6-bit DB field |8014B8A0 query key | Verified extraction within real table |
| GEAR raw tag49545247 (GRTI) | Unsigned4-bit DB field |8014B8A0 query key | Verified extraction within real table |
| GEAR raw tag49535247 (GRSI) | Unsigned6-bit DB field |8014BA6C →80149614 | Verified extraction and controlled decision input |
| Derived state | u32/4 |800DDEBC,800DDE18; preview writer800DE000 | Verified controlled decision; labels remain opaque |

No semantic meaning is assigned from the tag spellings. Global addresses above
are original GNNE69 references; native APIs accept table views and identities.

The first lookup uses GRTI=9 and GEID=the PSRT value. If its GRSI is 11, the
state is zero and the second lookup is skipped. Otherwise, the second lookup
uses GRTI=0 and GEID=the PHAT value. Its GRSI maps exactly as follows:

| Raw GRSI | Derived state |
|---|---|
|24|1|
|25|4|
|26|5|
|27|6|
|28|7|
|34|11|
|35|12|
|All other tested6-bit values|0|

The native catalog borrows the original GEAR table from private template
entry2. It validates required numeric fields and rejects missing or duplicate
joins. Those guards are defensive: original failed-query/EOF behavior is not
reconstructed or claimed equivalent. All 402 regular-roster joins succeed.

## Verification

```bash
python3 projects/nfl-street/reconstruction/native_assets/verify_headwear_state.py
python3 projects/nfl-street/reconstruction/native_assets/verify_hair_resources.py
```

**Verified native coverage:** all 402 selected regular-roster players derive
headwear automatically and load their three hair resources: 402 plans and
1,206 model files counted at the first explicit body variant per fresh process.
The combined geometry owner runs across all three explicit body variants:
1,206 face/body/hair assemblies. Existing face binding remains exercised with
fresh entry95 skeleton data. Two intentional hair-token collisions (face and
body namespaces) reject and preserve output. Two fresh native processes agree.

**Verified controlled original comparison:** each of the 282 GEAR rows has
its GEID/GRTI/GRSI fields compared with actual `801B94C8` extraction. Both
selected PLAY key fields are also compared for every player: 1,650 field
comparisons per process. Actual `800DDEBC` / `800DDE18` decisions match all
402 selections, including request arguments, values and query counts. All 64
possible6-bit mapping inputs match original `800DDE18` separately. Three
contexts14/15/16, twice in fresh processes, give 9,900 field comparisons,
2,412 selections and 384 mapping comparisons.

GEAR query services `8014B8A0` / `8014BA6C` are supplied from fresh records whose
field values were extracted by the original helper. This verifies the decision
and data connection under supplied table services; it does not execute full
original database registration/search lifecycle. The earlier explicit-state
hair planning and whole-model relocation/attachment gate still passes.

The private report at
`W:\Daywater\research-temp\nflstreet-headwear-state-20261012\verification.json`
records sources, builds, protected original hashes, five archives, the GEAR
DB block, 32 roster DB blocks, snapshots and repeated logs. Inputs remain
unchanged. All original assets, builds and diagnostics stay on W:.

## Remaining connections

Geometry ownership and headwear choice now have a joined native API. Hair
postattachment to the original target, remaining game-specific skeleton edits,
textures/materials, fresh constructor descriptors and resource-to-actor binding
remain open. Successful geometry assembly does not prove selected-player draw
behavior or playable fresh match startup. Menus and the controllable reference
scene still work. Daywater's generic platform is unchanged.
