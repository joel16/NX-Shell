#!/bin/bash -ex

sudo apt-get -y install build-essential
source /etc/profile.d/devkit-env.sh
export DEVKITA64=/opt/devkitpro/devkitA64

cd /NX-Shell
git submodule update --init --recursive
cd mupdf/ && make generate
cd ../ && make

rm -rf build include mupdf romfs source
rm icon.jpg LICENSE Makefile Makefile.mupdf NX-Shell.elf NX-Shell.nacp NX-Shell.nso NX-Shell.pfs0 README.md