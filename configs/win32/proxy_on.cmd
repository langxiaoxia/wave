@echo off
set DEPOT_TOOLS_UPDATE=0
set NO_AUTH_BOTO_CONFIG=%USERPROFILE%\https_proxy.boto
set http_proxy=http://127.0.0.1:8118
set https_proxy=http://127.0.0.1:8118
call git config --global http.proxy "socks5://192.168.121.230:1080"
call git config --global https.proxy "socks5://192.168.121.230:1080"
