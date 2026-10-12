#!/usr/bin/env bash
# Original resources, binary and diagnostic captures remain private on W:.
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=/mnt/w/Daywater/research-temp/nflstreet-team-live-20261011
bash "$root/build_windows_team_screen_preview.sh" "$out"
if (($# == 0)); then
  set -- --inspect-roster --player-inspection --live
fi
"$out/team-screen-preview.exe" \
  'W:\Daywater\research-temp\nflstreet-original-menus-20261009\nested\uis_all\00000\00007.bin' \
  'W:\Daywater\private\nflstreet\GNNE69-r0\unpacked' \
  'W:\Daywater\research-temp\nflstreet-native-windows-local\game\sys\main.dol' \
  1000 --capture 'W:\Daywater\research-temp\nflstreet-team-live-20261011\interactive' "$@"
