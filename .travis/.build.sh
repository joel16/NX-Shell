#!/bin/bash -ex

sudo apt-get update -y
sudo apt-get -y install build-essential

source /etc/profile.d/devkit-env.sh
export DEVKITA64=/opt/devkitpro/devkitA64

cd /NX-Shell && make
rm -rf !(*.nro)
