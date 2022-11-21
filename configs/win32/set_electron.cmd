@echo off

for /F %%i in ('e show out --path') do set "out_dir=%%i"
set PATH=%out_dir%;%PATH%
