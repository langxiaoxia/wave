for /F %%i in ('e show outdir') do set "electron_outdir=%%i"
cd %electron_outdir%
start winrar a -afzip pdb.zip *.pdb