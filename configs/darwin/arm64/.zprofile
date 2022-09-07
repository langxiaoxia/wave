eval "$(/opt/homebrew/bin/brew shellenv)"

  export HOMEBREW_BOTTLE_DOMAIN=https://mirrors.ustc.edu.cn/homebrew-bottles/bottles #ckbrew
  eval $(/opt/homebrew/bin/brew shellenv) #ckbrew

source $(brew --prefix nvm)/nvm.sh

export PYENV_ROOT="$HOME/.pyenv"
command -v pyenv >/dev/null || export PATH="$PYENV_ROOT/bin:$PATH"
eval "$(pyenv init -)"

alias ll='ls -l -G'

function vp_off() {
    unset VPYTHON_BYPASS
}

function vp_on {
    export VPYTHON_BYPASS="manually managed python not supported by chrome operations"
}

function proxy_off() {
    unset DEPOT_TOOLS_UPDATE
    unset NO_AUTH_BOTO_CONFIG
    unset http_proxy
    unset https_proxy
    git config --global --unset http.proxy
    git config --global --unset https.proxy
}

function proxy_on() {
    export DEPOT_TOOLS_UPDATE=0
    export NO_AUTH_BOTO_CONFIG=$HOME/https_proxy.boto
    export http_proxy=http://localhost:8118
    export https_proxy=http://localhost:8118
    git config --global http.proxy "socks5://192.168.121.230:1080"
    git config --global https.proxy "socks5://192.168.121.230:1080"
}

function proxy_check() {
    echo "check env proxy:"
    export | grep proxy
    echo "check git proxy:"
    git config --global -l | grep proxy
}

function set_electron() {
    out_dir=`e show out --path`
    export PATH=$out_dir/Electron.app/Contents/MacOS:$PATH
}

function get_electron() {
    which electron
}

function set_chrome_env() {
    export DEPOT_TOOLS_UPDATE=0
    export GIT_CACHE_PATH=$HOME/.git_cache
    export PATH=$HOME/depot_tools:$PATH
}

