pause 

set NO_AUTH_BOTO_CONFIG=%USERPROFILE%\https_proxy.boto
set http_proxy=http://127.0.0.1:8118
set https_proxy=http://127.0.0.1:8118

call e use x64

for /F %%i in ('e show root') do set "root_dir=%%i"
for /F %%i in ('e show outdir') do set "out_dir=%%i"

cd %root_dir%\src\electron
git pull
call e sync

del %out_dir%\pdb.zip
del %out_dir%\dist.zip

call e build

cd %root_dir%\src
python electron\script\zip-symbols.py -b %out_dir%
rename %out_dir%\pdb.zip %out_dir%\electron-v13.6.3-win32-x64-pdb.zip

call e build electron:dist
rename %out_dir%\dist.zip %out_dir%\electron-v13.6.3-win32-x64.zip

pause