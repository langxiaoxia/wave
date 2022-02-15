@echo off

setlocal

call e use dev_testing
for /F %%i in ('e show root') do set "root_dir=%%i"
cd %root_dir%\src
set CHROMIUM_BUILDTOOLS_PATH=%cd%\buildtools
call gn gen out/x64_testing --args="import(\"//electron/build/args/testing.gn\") target_cpu=\"x64\"" --ide=vs2019 --sln=x64_testing_all

call e use dev_release
for /F %%i in ('e show root') do set "root_dir=%%i"
cd %root_dir%\src
set CHROMIUM_BUILDTOOLS_PATH=%cd%\buildtools
call gn gen out/x64_release --args="import(\"//electron/build/args/release.gn\") target_cpu=\"x64\"" --ide=vs2019 --sln=x64_release_all
