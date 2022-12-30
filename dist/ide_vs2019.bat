@echo off

setlocal

set PATH=%USERPROFILE%\.electron_build_tools\third_party\depot_tools;%PATH%

for /F %%i in ('e show root') do set "root_dir=%%i"
cd %root_dir%\src
set CHROMIUM_BUILDTOOLS_PATH=%cd%\buildtools

for /F %%i in ('e show current') do set "config_name=%%i"
for /F %%i in ('e show out') do set "out_name=%%i"
call gn gen out/%out_name% --args="import(\"//electron/build/args/release.gn\") target_cpu=\"x64\"" --ide=vs2019 --sln=electron_%config_name%
