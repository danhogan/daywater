# Fresh selected-player resolution — 2026-10-11

The Windows C resolver joins a selected team/DIGP directly to the actual PLAY
row and original face/roster-hair TERF archives. It requires one supported HSH1
chunk before treating an absent name as a lookup miss. Unsupported archive
layouts reject without changing caller output. No snapshot, original DLL,
Python catalog or supplied lookup result is needed during native resolution.

## Verified bounded checks

`verify_team_player_resolve.py` reruns untouched original801A8088 name lookups,
body-key routing and descriptor-constructor checks, then strictly builds/runs
the native resolver. All402 regular-team PLAY rows resolve, all402 selected
head payloads decode as TMdl, and2,416 uppercase/lowercase/miss queries match
the original oracle. This validates index selection and payload acquisition;
it does not prove full original resource registration, renderer state or actor
appearance. Both hair candidates remain candidates until the original derived
hair selector is joined. Head misses use the original0x28a fallback.

The real-data check caught synthetic-fixture tag spelling errors in the new
manifest. Raw appearance getters now use actual PLAY tags:

| Manifest member | Raw tag | Original reader |
| --- | --- | --- |
| phar | 0x52414850 | 8014ECB8 |
| plht | 0x54484C50 | 8014EC90 |
| pjen | 0x4E454A50 | 8014F050 |
| plex | 0x58454C50 | 8014EC68 |
| tghp / phgt alias | 0x54474850 | 8014F028 |

These are raw values. For example8014ECB8 normalizes raw0x3f to0x0c; the
manifest retains the raw field and does not claim that normalization or any
unknown semantic interpretation. PHGT is not a separate database field.

The [selected-head inspection](team_player_selected_head.md) uses a resolved
index to show that athlete's own geometry. Its stored-scale composition and
host pose remain candidate; body, hair, materials and attachment are unfinished.

Confidence: **verified within the stated lookup/payload scope**, GameCube NFL
Street GNNE69-r0, DOL SHA1`46e3de854998a0f66c9be8c72a1164d22cd29eb4`.
Protected DOL/DLL/runner hashes remain unchanged. Private checks/builds/logs
and report: `W:\Daywater\research-temp\nflstreet-team-resolve-20261011`.
This is offline original-module verification, not new live Dolphin integration.
