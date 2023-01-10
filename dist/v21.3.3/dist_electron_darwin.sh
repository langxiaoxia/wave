#!/bin/sh

export DEPOT_TOOLS_UPDATE=0
export NO_AUTH_BOTO_CONFIG=$HOME/https_proxy.boto
export http_proxy=http://127.0.0.1:8118
export https_proxy=http://127.0.0.1:8118

build_tag=v21.3.3
build_os=darwin
build_archs="arm64 x64"

for build_arch in $build_archs; do
  e use v21_$build_arch

  src_dir=$(e show src)
  out_dir=$(e show out --path)

  # Layer 1: Checkout.
  cd $src_dir
  git pull
  e sync

  # Layer 2: Builds.
  # step-electron-build
  rm -f $out_dir/gen/electron/electron_version.args
  rm -f $out_dir/gen/electron/electron_version.h
  rm -f $out_dir/dist.zip
  rm -f $out_dir/symbols.zip
  rm -fr $out_dir/breakpad_symbols
  rm -f $out_dir/dsym.zip
  e build

  # step-maybe-generate-breakpad-symbols
  e build -t electron:electron_symbols

  # step-electron-dist-build
  e build electron:dist

  # step-maybe-zip-symbols
  $src_dir/script/zip-symbols.py -b $out_dir

  # step-electron-publish
  mv $out_dir/dist.zip $out_dir/electron-$build_tag-$build_os-$build_arch.zip
  mv $out_dir/symbols.zip $out_dir/electron-$build_tag-$build_os-$build_arch-symbols.zip
  mv $out_dir/dsym.zip $out_dir/electron-$build_tag-$build_os-$build_arch-dsym.zip
done
