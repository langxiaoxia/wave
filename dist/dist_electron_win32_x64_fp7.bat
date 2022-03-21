@echo off

setlocal

set NO_AUTH_BOTO_CONFIG=%USERPROFILE%\https_proxy.boto
set http_proxy=http://127.0.0.1:8118
set https_proxy=http://127.0.0.1:8118

call e use x64_fp7

for /F %%i in ('e show root') do set "root_dir=%%i"
for /F %%i in ('e show out') do set "out_name=%%i"
set out_dir=%root_dir%\src\out\%out_name%

cd %root_dir%\src\electron
call git pull
copy /b ELECTRON_VERSION+,,
call e sync

call e build

del %out_dir%\dist.zip
call e build electron:dist
del %out_dir%\electron-v13.1.4-win32-x64.zip
ren %out_dir%\dist.zip electron-v13.1.4-win32-x64.zip

cd %root_dir%\src
del %out_dir%\pdb.zip
call python electron\script\zip-symbols.py -b %out_dir%
del %out_dir%\electron-v13.1.4-win32-x64-pdb.zip
ren %out_dir%\pdb.zip electron-v13.1.4-win32-x64-pdb.zip
