#!/bin/bash -ex

sudo apt-get update -y
sudo apt-get -y install build-essential
source /etc/profile.d/devkit-env.sh
export DEVKITA64=/opt/devkitpro/devkitA64

cd /NX-Shell
cd mupdf/ && git checkout 1.14.0 && make generate
cd ../ && make

rm -rf .travis build include mupdf romfs source
rm .gitattributes .gitignore .gitmodules icon.jpg LICENSE Makefile Makefile.mupdf NX-Shell.elf NX-Shell.nacp NX-Shell.nso NX-Shell.pfs0 README.md .travis.yml
