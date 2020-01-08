# NX-Shell [![Build Status](https://travis-ci.org/joel16/NX-Shell.svg?branch=master)](https://travis-ci.org/joel16/NX-Shell) ![Github latest downloads](https://img.shields.io/github/downloads/joel16/NX-Shell/total.svg)

NX Shell is a multi-purpose file manager for the Nintendo Switch that aims towards handling media files. NX Shell leans towards more of the file management features such as opening and managing various files types and includes the basic necessity of a standard file manager. This program's design elements are clearly inspired by CyanogenMod/LineageOS's built in file manager, and so all credits towards the design go to the CyanogenMod/LineageOS contributors.


<p align="center">
  <img src="https://i.imgur.com/cvpisEv.jpg" alt="NX-Shell Screenshot"/>
</p>


# Features:

- Delete files and folders.
- Copy/Move files and folders.
- Rename files and folders (standard switch keyboard).
- Create files and folders (standard switch keyboard).
- Displays file size
- Image viewer (GIF, JPG, PNG, WEBP)
- Displays animated GIFs.
- Extract various archives such as ZIP, RAR, 7Z, ISO 9660, AR, XAR and others supported by libarchive. 
- Sorting options (Sort by name, date, size).
- Toggle dark theme.
- Audio playback. (Following formats are supported: FLAC, IT, MOD, MP3, OGG, OPUS, S3M and WAV)
- Touch screen in most cases.

# Build instructions

Ensure that you have the following dependencies installed via [dkp-pacman](https://github.com/devkitPro/pacman):
```bash
sudo dkp-pacman -Syu switch-dev
sudo dkp-pacman -S switch-sdl2 switch-sdl2_gfx switch-sdl2_image switch-sdl2_ttf
sudo dkp-pacman -S switch-opusfile switch-liblzma
sudo dkp-pacman -S switch-libvorbisidec switch-mpg123 switch-flac

```

Clone and compile the program:
```bash
git clone https://github.com/joel16/NX-Shell.git
cd NX-Shell
make
```

# Credits:

- **Preetisketch** for some of the assets used as well as the banner.
- **NicholeMattera** for the implementing the foundation of all touch-screen code.
- **grimfang4** for the original SDL_FontCache headers.
- **theMealena** for CVE_gif*.
