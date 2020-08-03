#!/bin/bash -ex

apt-get update -y && apt-get -y install sudo
sudo apt-get -y install build-essential sudo wget libxml2

# install dkp deps
touch /trustdb.gpg
wget https://github.com/devkitPro/pacman/releases/download/devkitpro-pacman-1.0.1/devkitpro-pacman.deb
sudo dpkg -i devkitpro-pacman.deb

sudo dkp-pacman --noconfirm -Syu switch-dev switch-sdl2 switch-sdl2_gfx switch-sdl2_image switch-sdl2_ttf switch-opusfile switch-liblzma switch-libvorbisidec switch-mpg123 switch-flac 

source /etc/profile.d/devkit-env.sh
export DEVKITA64=/opt/devkitpro/devkitA64

cd /NX-Shell && make

rm -rf .travis build include libs romfs source
rm .gitattributes .gitignore icon.jpg LICENSE Makefile NX-Shell.elf NX-Shell.nacp NX-Shell.nso NX-Shell.pfs0 README.md .travis.yml
