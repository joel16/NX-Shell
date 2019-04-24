#!/bin/bash -ex

sudo apt-get update -y
sudo apt-get -y install build-essential
source /etc/profile.d/devkit-env.sh
export DEVKITA64=/opt/devkitpro/devkitA64

cd /NX-Shell && make

rm -rf .travis build include libs romfs source
rm .gitattributes .gitignore icon.jpg LICENSE Makefile NX-Shell.elf NX-Shell.nacp NX-Shell.nso NX-Shell.pfs0 README.md .travis.yml
