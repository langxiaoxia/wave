
  export HOMEBREW_BOTTLE_DOMAIN=https://mirrors.ustc.edu.cn/homebrew-bottles/bottles #ckbrew
  eval $(/usr/local/Homebrew/bin/brew shellenv) #ckbrew

export PATH=$HOME/nodejs/node_global/bin:$PATH

alias ll='ls -l -G'

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

