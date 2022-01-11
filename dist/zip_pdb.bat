for /F %%i in ('e show outdir') do set "electron_outdir=%%i"
cd %electron_outdir%
start winrar a -afzip electron-v13.6.3-win32-x64-pdb.zip *.pdb