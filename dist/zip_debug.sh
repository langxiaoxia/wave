#!/bin/sh
cd `e show root`
cd src
electron/script/copy-debug-symbols.py --target-cpu="x64" --out-dir=out/`e show out`/debug --compress -d out/`e show out`
electron/script/strip-binaries.py --target-cpu="x64" -d out/`e show out`
electron/script/add-debug-link.py --target-cpu="x64" --debug-dir=out/`e show out`/debug -d out/`e show out`
cd out/`e show out`
zip electron-v13.6.3-linux-x64-debug.zip debug/*.debug
