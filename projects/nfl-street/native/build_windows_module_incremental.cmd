@echo off
setlocal
if "%~1"=="" exit /b 2
set "VS_DEVCMD=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
if not exist "%VS_DEVCMD%" exit /b 3
call "%VS_DEVCMD%" -arch=amd64 -host_arch=amd64
if errorlevel 1 exit /b 4
set "PATH=W:\Daywater\tools\cmake-4.4.3-windows-x86_64\bin;%PATH%"
cmake --build "%~1" --target gGNNE69_recomp -j4
exit /b %ERRORLEVEL%
