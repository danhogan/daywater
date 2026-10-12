@echo off
setlocal
set "DW_W_ROOT=W:\Daywater"
set "VS_DEVCMD=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
if not exist "%VS_DEVCMD%" exit /b 2
call "%VS_DEVCMD%" -arch=amd64 -host_arch=amd64
if errorlevel 1 exit /b 3
set "PATH=%DW_W_ROOT%\tools\cmake-4.4.3-windows-x86_64\bin;%PATH%"
"%DW_W_ROOT%\tools\ModernGekko-Template\lib\ModernGekko\build-win64\moderngekko-port.exe" build "%DW_W_ROOT%\research-temp\nflstreet-moderngekko-game" --backend c --toolchain msvc --output "W:\nfsmod"
exit /b %ERRORLEVEL%
