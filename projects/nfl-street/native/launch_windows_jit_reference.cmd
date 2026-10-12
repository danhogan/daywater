@echo off
setlocal
set "DW_ROOT=W:\Daywater"
set "DW_RUNNER=%DW_ROOT%\tools\ModernGekko-Template\lib\ModernGekko\build-win64\moderngekko-run.exe"
set "DW_GAME=%DW_ROOT%\research-temp\nflstreet-moderngekko-game"
set "DW_USER=%DW_ROOT%\research-temp\nflstreet-moderngekko-win-user-idle-801f471c"
set "DW_STATE=%DW_ROOT%\research-temp\nflstreet-moderngekko-win-vulkan-automation\artifacts\after-field-select.sav"
set "MODERNGEKKO_BOOT_IMAGE=%DW_ROOT%\research-temp\nflstreet-GNNE69.iso"
set "MODERNGEKKO_STATICRECOMP=0"

if not "%~1"=="" set "DW_STATE=%~1"
if not "%~2"=="" set "DW_USER=%~2"
if not exist "%DW_RUNNER%" exit /b 2
if not exist "%MODERNGEKKO_BOOT_IMAGE%" exit /b 4
if not exist "%DW_STATE%" exit /b 5
if not exist "%DW_USER%\Config\Dolphin.ini" exit /b 7

"%DW_RUNNER%" --game "%DW_GAME%" --allow-interpreter --user-dir "%DW_USER%" --graphics Vulkan --load-state "%DW_STATE%"
exit /b %ERRORLEVEL%
