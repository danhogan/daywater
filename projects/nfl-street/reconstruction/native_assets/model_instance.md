# Original model-instance initialization

`model_instance.c` reconstructs original `80162FC4` and its called initialization,
palette/material binding helpers, followed by model setter `801EBFE0`. It writes
an original-layout 240-byte instance, preserving untouched allocator bytes.
The supplied tokens identify model view, palette descriptor and material table.
No original resource table is embedded or dereferenced by this initializer.

Game: NFL Street, GNNE69 revision0; DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`; source captures Dolphin2603a.
Confidence: **verified initializer/setter bytes**, not a full player constructor.

Initialization retains the original OR behavior for flags +4/+8 and selector
words +0x14/+0x18. It sets all64 selector bytes to FF, initializes reference
words +E0/+E4/+E8/+EC and binds palette/material tokens. Selecting a nonnull
model changes +C and marks group selectors pending with FFFFFFFF. Untouched
words, including cached group-list storage, remain as allocated.

## Reproduction

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 projects/nfl-street/reconstruction/native_assets/verify_model_instance.py
```

Private report: `W:\Daywater\research-temp\nflstreet-model-instance-20261013\verification.json`.
Three saved contexts repeat in two fresh Windows processes each. Every process
executes32 original initializer/setter sequences: zero/A5/FF/pattern allocator
fills and eight null/non-null model/palette/material masks. All240 bytes match.
All original callees execute; there are no intercepted constructor services.
Reports record executable/source/input/log and unchanged protected baseline hashes.

Group selection, model chunk-list construction, complete actor state, palette
allocation policy and material-table creation are separate. See
[selected model instances](match_model_instances.md).
