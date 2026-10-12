# Original ball frame sequence — 2026-10-10

## Native connection

`ball_frame.c` joins the record effects of original `800C0FE4` to the checked
attached and bounded airborne updates. It uses owned records and structured
resolved-actor inputs; no runtime RAM reading, emulator, original DLL or DOL.

1. Copy all seven current position/orientation words into history before update.
2. Use the resolved attachment inputs when associated, or the checked detached
   airborne service otherwise. Unsupported/mismatched association rejects.
3. Clear force and angular input triples to original positive zero.
4. Return the visual pose and the final environment-position query request.

The original environment spatial index is **not reconstructed**. The oracle
supplies its result and checks the actual query position/order and original
result write. Native code returns that request; the viewer records its count
without inventing a region or collision result. Detached updates preserve the
prior visual-model pose, as this caller does; the existing inspection renderer
continues drawing the ball record's position/quaternion directly.

The optional `--ball-frame-inspection` requires airborne inspection/check mode.
It uses this frame sequence for both owners and detached movement, checks the
complete history and cleared input span after every frame, and records each
spatial query. It explicitly supplies the inspection force `(0,0,-9)` on each
detached frame because the original caller clears force after updating. This
force is not a recovered original gravity constant. Existing modes and user
launchers stay on their preceding behavior.

Private interactive launcher:
`W:\Daywater\research-temp\nflstreet-ball-frame-20261010\play-ball-frame-inspection.cmd`.
Its movement/animation/contact/ownership controls retain the prior inspection
behavior. Natural snap/pass/catch, launch selection, ground collision/bounce,
force producers and fresh match startup remain unfinished. Host stops below
plane0 remain inspection stops, not reconstructed collision.

## Original verification

Game NFL Street, GameCube GNNE69 revision0; DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`. Saved position-neutral14/15/16
from Dolphin2603a. Confidence: **verified controlled caller record effects and
query interface**, with supplied association/model/force/clock/environment
boundaries. This does not establish fresh full-match gameplay fidelity.

`ball_frame_attached_original_check.c` executes the untouched full `800C0FE4`,
including actual actor association and attachment services. Two actual actors,
selector sweeps including -1, transformed/scaled roots, changed offsets and
deltas0/1/1⁄60/-0.5 yield **576 complete 244-byte records**, visual-model poses
and ordered query inputs matching exactly. Nonzero force/angular inputs are
cleared, and deliberately different prior history is overwritten. Late model
rejection preserves the entire native record/output; missing resolved input also
rejects without changes.

`ball_frame_airborne_original_check.c` executes the same full caller with actual
association resolving null, eight64-update scenarios per saved context,
force/delta/target-plane variations and field boundaries. **1,536 complete
records and query inputs match exactly**, including consecutive force clearing.
Nine unsupported/nonfinite/truncated cases preserve complete record/output.

Total: **2,112 full original caller record comparisons**. Original constant
`80252164` is checked to be positive zero before execution. The final
`801087B8` environment lookup alone is supplied; its input position and result
write at visual+294 are verified. It is not skipped silently or counted as a
reconstructed spatial index. Both oracles build with `build_windows_ball_frame.sh`.

## Field evidence

| Field | Type/size/endian | Evidence and limitations |
|---|---|---|
| History position +28 | float32[3]/12/BE | `800C0FE4` copies current+C before either branch; attached velocity service reads it. Checked all2,112 complete records. |
| History orientation +34 | float32[4]/16/BE | Same caller copies current+18 before either branch; all four words checked. Downstream consumers not fully mapped. |
| Force+7C/angular+88 | float32[3] each/12 each/BE | Same caller clears six words after update using80252164. Nonzero controlled values and repeated detached frames checked; producer policy unresolved. |
| Visual environment word +294 | opaque uint32/4/BE | record+0 BE pointer resolves visual; `800C0FE4` stores801087B8 result. Oracle supplies12345678 and checks write. Spatial-record pointer interpretation is probable from8010862C; native production is unfinished. |

Reference ball pool: r13=803A9A40, root slot803A2670; active record starts at
pool+8 with strideF4. Actor reference pointers803C04EC/803C19C0 and saved visual
pointer80DEEE24 are context observations, not native runtime addresses. Existing
attachment and flight notes document their readers/writers and observed fields.

## Windows integration

Automated native direction/set and contact checks exercise the new frame path.
Direction/set:528 ticks,492 full history/clear checks,121 detached updates,
four set changes and six assignments/two releases. Contact:200 ticks,203 history/
clear checks,64 detached updates and two completed inspection contacts. All12
direction/set and14 contact captures match the preceding airborne build exactly.
Exact counts and hashes are recorded in private `verification.json`.
They verify this independently running inspection, not original frame timing,
GX pixel equivalence, natural contacts or live original gameplay. The host still
supplies ownership choices, actor/model profiles and force; animation eligibility,
world/root scheduling, spatial lookup and gameplay state producers remain open.

Original DLL and runner source stay untouched and hash checked. All original
assets, outputs, fixtures, captures, commands and diagnostic logs stay on W:.
The generic Daywater platform is unchanged.
