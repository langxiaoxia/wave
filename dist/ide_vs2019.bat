@echo off

setlocal

for /F %%i in ('e show root') do set "root_dir=%%i"
cd %root_dir%\src
set CHROMIUM_BUILDTOOLS_PATH=%cd%\buildtools
gn gen out/x64 --args="import(\"//electron/build/args/release.gn\") target_cpu=\"x64\"" --ide=vs2019
