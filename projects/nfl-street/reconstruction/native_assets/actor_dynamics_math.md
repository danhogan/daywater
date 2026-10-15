# Actor dynamics math leaves

## Verified scope

`actor_dynamics_math.c` is the first, independently verified stage for the
secondary head/hand dynamics reconstruction. It implements these original
leaves against saved big-endian RAM:

- `80188D98`: the signed 24-bit polynomial `atan2` used for matrix angles;
- `8004BED8`: the integer range limiter (including its high/low clamp order);
- `801DE5DC`, `801DE62C`, `801DDFFC`, and `801DE118`/`801DE094`/`80188EAC`:
  vector difference, dot product, length, and conditional normalization;
- `80189F10`, `801DEB54`, and `80189F54`: stack-top translation, rotated
  vector, and three 24-bit matrix angles.

All arithmetic uses the existing Gekko-compatible fused multiply-add helper.
The disassembly for this dependency path was checked: neither `8004BF1C`,
`8004C268`, `80189F54`, `80188D98`, nor their normalization path uses
`frsqrte`; normalization uses `sqrt` followed by single-precision division.

## Oracle gate

```bash
PYTHONPATH=projects/nfl-street/reconstruction/menus python3 \
  projects/nfl-street/reconstruction/native_assets/verify_actor_dynamics_math.py
```

For each saved contexts 14, 15 and 16, twice, the original module and native
code run 20,003 deterministic randomized cases for each group. Every result
is bit-exact: scalar angle/clamp returns, vector outputs, and stack-dependent
translation, rotation, and angle outputs. Private build/logs are in
`W:\Daywater\research-temp\nflstreet-actor-dynamics-20261014`.

## Not yet modeled

This stage does not claim either dynamics update (`8004BF1C`, `8004C268`),
their state evolution, or their integration in `actor_palettes.c`. Those are
the next stage and remain deliberately unmodified.
