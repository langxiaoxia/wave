@echo off

setlocal

set DEPOT_TOOLS_UPDATE=0
set NO_AUTH_BOTO_CONFIG=%USERPROFILE%\https_proxy.boto
set http_proxy=http://127.0.0.1:8118
set https_proxy=http://127.0.0.1:8118

set build_tag=v21.3.3
set build_os=win32
set build_archs=x64 ia32

for %%a in (%build_archs%) do (
  call :BuildElectron %%~a
)

endlocal
exit /b

:BuildElectron
set build_arch=%~1

call e use v21_%build_arch%

for /F %%i in ('e show src') do set "src_dir=%%i"
for /F %%i in ('e show out --path') do set "out_dir=%%i"

:: Layer 1: Checkout.
cd %src_dir%
call git pull
call e sync

:: Layer 2: Builds.
:: step-electron-build
if exist %out_dir%\gen\electron\electron_version.args del %out_dir%\gen\electron\electron_version.args
if exist %out_dir%\gen\electron\electron_version.h del %out_dir%\gen\electron\electron_version.h
if exist %out_dir%\dist.zip del %out_dir%\dist.zip
if exist %out_dir%\symbols.zip del %out_dir%\symbols.zip
if exist %out_dir%\breakpad_symbols rmdir /s/q %out_dir%\breakpad_symbols
if exist %out_dir%\pdb.zip del %out_dir%\pdb.zip
call e build

:: step-maybe-generate-breakpad-symbols
call e build -t electron:electron_symbols

:: step-electron-dist-build
call e build electron:dist

:: step-maybe-zip-symbols
call python %src_dir%\script\zip-symbols.py -b %out_dir%

:: step-electron-publish
move /Y %out_dir%\dist.zip %out_dir%\electron-%build_tag%-%build_os%-%build_arch%.zip
move /Y %out_dir%\symbols.zip %out_dir%\electron-%build_tag%-%build_os%-%build_arch%-symbols.zip
move /Y %out_dir%\pdb.zip %out_dir%\electron-%build_tag%-%build_os%-%build_arch%-pdb.zip

exit /b
