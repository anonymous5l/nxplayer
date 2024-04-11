#!/bin/bash

BUILD_DIR=cmake-build-switch

cd "$(dirname $0)/.."
git config --global --add safe.directory `pwd`

PKGS=(
    "switch-libass-0.17.1-1-any.pkg.tar.zst"
    "switch-ffmpeg-6.1-5-any.pkg.tar.zst"
    "switch-libmpv-0.36.0-2-any.pkg.tar.zst"
    "switch-nspmini-48d4fc2-1-any.pkg.tar.xz"
    "hacBrewPack-3.05-1-any.pkg.tar.zst"
)
for PKG in "${PKGS[@]}"; do
    dkp-pacman -U --noconfirm ./scripts/deps/${PKG}
done

cmake -B ${BUILD_DIR} \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILTIN_NSP=OFF \
  -DPLATFORM=SWITCH \
  -DUSE_DEKO3D=OFF \
  -DBRLS_UNITY_BUILD=OFF \
  -DCMAKE_UNITY_BUILD_BATCH_SIZE=16

make -C ${BUILD_DIR} nxplayer.nro -j$(nproc)
