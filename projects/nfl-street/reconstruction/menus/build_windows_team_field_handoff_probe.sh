#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${1:-/mnt/w/Daywater/research-temp/nflstreet-team-field-handoff-20261011}
case "$out" in /mnt/w/Daywater/*) ;; *) exit 2 ;; esac
DAYWATER_TEAM_SCREEN_PROBE_SOURCE="$root/team_field_handoff_probe.c" \
  bash "$root/build_windows_team_screen_checks.sh" "$out"
mv -f "$out/team-screen-probe.exe" "$out/team-field-handoff-probe.exe"
