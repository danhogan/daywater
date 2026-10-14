#!/usr/bin/env bash
# Builds the live prototype with MSVC on Windows: copies these sources (and
# the reconstruction sources they use) to the private W: work area, then
# configures against the Aurora checkout there.
set -euo pipefail
here=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
work=${1:-/mnt/w/Daywater/research-temp/nflstreet-live-20261013}
case "$work" in /mnt/w/Daywater/*) ;; *) exit 2 ;; esac
mkdir -p "$work/src"
cp "$here"/CMakeLists.txt "$here"/*.c "$here"/*.h "$work/src/"
cp "$here"/../../reconstruction/native_assets/*.[ch] "$work/src/" 2>/dev/null || true
win=$(wslpath -w "$work")
cat > "$work/build.cmd" <<CMD
@echo off
setlocal
call "C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\Common7\\Tools\\VsDevCmd.bat" -arch=amd64 -host_arch=amd64 >nul
if errorlevel 1 exit /b 3
set "PATH=W:\\Daywater\\tools\\cmake-4.4.3-windows-x86_64\\bin;%PATH%"
cmake -S "$win\\src" -B "$win\\build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DAURORA_SOURCE_DIR=W:/Daywater/tools/aurora
if errorlevel 1 exit /b 4
cmake --build "$win\\build" --target nflstreet_live
exit /b %ERRORLEVEL%
CMD
cd "$work" && cmd.exe /c "$win\\build.cmd"
