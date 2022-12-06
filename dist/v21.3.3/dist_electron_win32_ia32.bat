@echo off

setlocal

set NO_AUTH_BOTO_CONFIG=%USERPROFILE%\https_proxy.boto
set http_proxy=http://127.0.0.1:8118
set https_proxy=http://127.0.0.1:8118

set build_tag=v21.3.3

call e use v21_x86

for /F %%i in ('e show root') do set "root_dir=%%i"
for /F %%i in ('e show out') do set "out_name=%%i"
set out_dir=%root_dir%\src\out\%out_name%

cd %root_dir%\src\electron
call git pull
call e sync

call e build

del %out_dir%\dist.zip
call e build electron:dist
del %out_dir%\electron-%build_tag%-win32-ia32.zip
ren %out_dir%\dist.zip electron-%build_tag%-win32-ia32.zip

cd %root_dir%\src
del %out_dir%\pdb.zip
call python electron\script\zip-symbols.py -b %out_dir%
del %out_dir%\electron-%build_tag%-win32-ia32-pdb.zip
ren %out_dir%\pdb.zip electron-%build_tag%-win32-ia32-pdb.zip
