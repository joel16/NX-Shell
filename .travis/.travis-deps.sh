#!/bin/sh
set -e
set -x

# Get latest libnx and overwrite bundled one
mkdir libnx-update && cd libnx-update
git clone https://github.com/switchbrew/libnx.git
cd libnx/
make
sudo make install
cd ../../
rm -rf libnx-update/
