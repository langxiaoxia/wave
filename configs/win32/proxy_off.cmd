@echo off
set DEPOT_TOOLS_UPDATE=
set NO_AUTH_BOTO_CONFIG=
set http_proxy=
set https_proxy=
call git config --global --unset http.proxy
call git config --global --unset https.proxy
