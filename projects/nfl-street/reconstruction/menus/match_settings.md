# Fresh-match settings from original `801E51D4`

`match_settings.c` owns the settings record used by the `801E51D4` fresh-match
path. Its field order is taken from the actual `FNIG` query at `0x80266F44`,
and it keeps the original database tags in `nflstreet_match_setting_field`.
The query engine remains a caller-supplied service. A successful run returns
the 25 query values, the normalized 25-word insert descriptor, the converted
`GTYG` floating argument, both final context values, and the original status.

The exact query strings read from original RAM are:

```text
0x80266F44 select 'GTYG' into 0x86 and 'ISFG' into 0x82 and 'DOTG' into 0x82 and 'HTWG' into 0x82 and 'GPWG' into 0x82 and 'GTWG' into 0x82 and 'GFWG' into 0x82 and 'GWWG' into 0x82 and 'GTHG' into 0x82 and 'IPHG' into 0x82 and 'PDHG' into 0x82 and 'OTHG' into 0x82 and 'HCHG' into 0x82 and 'CSHG' into 0x82 and 'GTAG' into 0x82 and 'IPAG' into 0x82 and 'PDAG' into 0x82 and 'OTAG' into 0x82 and 'HCAG' into 0x82 and 'CSAG' into 0x82 and 'MITG' into 0x82 and 'RTQG' into 0x82 and 'ELQG' into 0x82 and 'WODG' into 0x82 and 'PNIG' into 0x82 from 'FNIG'\n
0x80267118 select 'NLQO' into 0x82 from 'TPOG'\n
0x8026713C select COL_TEAM_OFFPLAYBOOK into 0x82 and COL_TEAM_DEFPLAYBOOK into 0x82 from 'MAET' where ('DIGT' = 0x82)\n
0x802671A0 update 'FNIG' set 'GTYG' = 0x86 and 'ISFG' = 0x82 and 'DOTG' = 0x82 and 'HTWG' = 0x82 and 'GPWG' = 0x82 and 'GTWG' = 0x82 and 'GFWG' = 0x82 and 'GWWG' = 0x82 and 'GTHG' = 0x82 and 'IPHG' = 0x82 and 'PDHG' = 0x82 and 'OTHG' = 0x82 and 'HCHG' = 0x82 and 'CSHG' = 0x82 and 'GTAG' = 0x82 and 'IPAG' = 0x82 and 'PDAG' = 0x82 and 'OTAG' = 0x82 and 'HCAG' = 0x82 and 'CSAG' = 0x82 and 'MITG' = 0x82 and 'RTQG' = 0x82 and 'ELQG' = 0x82 and 'WODG' = 0x82 and 'PNIG' = 0x82\n
```

The ordered record words are `GTYG`, `ISFG`, `DOTG`, `HTWG`, `GPWG`, `GTWG`,
`GFWG`, `GWWG`, `GTHG`, `IPHG`, `PDHG`, `OTHG`, `HCHG`, `CSHG`, `GTAG`,
`IPAG`, `PDAG`, `OTAG`, `HCAG`, `CSAG`, `MITG`, `RTQG`, `ELQG`, `WODG`,
`PNIG`. The first word is binary32. The original loads that value into PPC
`f1` for the final update; the native descriptor performs the same binary32 to
binary64 conversion before the query callback.

After the initial query succeeds, original code assigns the incoming mode to
`ISFG`. For mode zero, it applies these source comparisons and replacements:

| Field | If original word equals | Replacement |
|---|---:|---:|
| `GTYG` | binary32 `999.99` (`0x4479FF5C`) | binary32 `10.0` (`0x41200000`) |
| `DOTG` | 2047 | 780 |
| `HTWG` | 15 | 0 |
| `GPWG` | 127 | 0 |
| `GTWG` | 127 | 78 |
| `GFWG` | 127 | 0 |
| `GWWG` | `0xFFFFFFFF` | `0x01680000` |
| `OTHG`, `OTAG` | 4 | 3 |
| `HCHG`, `HCAG` | 3 | 2 |
| `CSHG`, `CSAG` | 255 | 0 |
| `MITG` | `0xFFFFFFFF` | 0 |
| `RTQG`, `WODG` | 15 | 0 |
| `PNIG` | 255 | 0 |

If `RTQG` is still 1023, the `TPOG` / `NLQO` query supplies its replacement.
The `MAET` pair query is made for side 0 when either `IPHG` or `PDHG` is 63,
using `GTHG` as the `DIGT` value. It is made for side 1 when either `IPAG` or
`PDAG` is 63, using `GTAG`. The query returns both playbook values, while the
original copies back only fields that held 63. Pair resolution and the final
`FNIG` update occur for nonzero modes too; those modes skip only the mode-zero
normalization and dependent-default query. The two returned context words are
the final `GTHG` and `GTAG` values.

The two caller output words are initialized to `0x3FF` before any query. They
remain `0x3FF` if the initial/default/pair/insert query returns an error; only a
successful insert path replaces them with `GTHG` and `GTAG`.

## Original-module differential

`match_settings_original_check.c` executes the untouched `801E51D4` body from
the original Windows DLL. It intercepts only `801B4510`, supplies explicit
outputs for all query strings, and compares the exact query order, all 24
integer insert arguments, the PPC floating argument, final status, and both
context outputs against `match_settings.c`. Nine success runs passed: modes 0,
1, and 2 in each materialized context 14, 15, and 16. These RAM images are
derived from saved RAM with native source/font fixture patches. Eight additional
original/native error comparisons passed for the initial, dependent-default,
side-pair, and final insert queries with both positive and negative statuses;
each retained `0x3FF` in both context outputs. Mode zero exercised the mode-dependent
defaults, both dependent side queries, and final insert; nonzero modes exercised
both side queries and insert while bypassing mode-zero normalization.

Build and run on the Windows host:

```bash
python3 projects/nfl-street/reconstruction/menus/verify_match_settings.py
```

This is a controlled original-module differential, not live gameplay or proof
that the query service created a playable session. The fixture contexts are
not live fresh-game states, and query outputs were supplied explicitly by the
harness. Finding confidence is **verified** for the local
normalization and call control flow under those outputs; query-engine ownership
and game session creation remain outside this check.

Evidence provenance: NFL Street GameCube GNNE69 revision 0, original DOL SHA1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`; ModernGekko DLL SHA256
`828d39f464050f8a01bbf8fdbae09d17ca7f35c128cec9ce49d38789eea82236`.
The three input RAM hashes and private outputs are recorded at
`W:\Daywater\research-temp\nflstreet-match-settings-20261011\`.
`verification.json` in that private folder contains the input hashes and run
summary.

The root reproduction gate `verify_match_settings.py` independently rebuilds
and repeats those17 comparisons twice using the preserved original
`position-neutral-14/15/16.mem1` contexts. Its private report is
`W:\Daywater\research-temp\nflstreet-match-settings-20261012\verification.json`.
The inputs remain saved reference states with supplied query responses; this
is not a fresh live match setup. Their source captures used Dolphin2603a.
