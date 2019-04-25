# NX-Shell

Work in progress port of 3DShell (Multi purpose file manager) to the Nintendo Switch.

# Features:

- Delete files and folders.
- Copy/Move files and folders.
- Rename files and folders.
- Create folders.
- Displays file size. (files only ATM)
- Image viewer (PNG/JPG/BMP)
- Render animated GIFs.
- Extract .ZIP and .RAR files.
- Sorting options (Sort by name, date, size).
- Dark theme toggle.
- Audio playback. (Following formats are supported: FLAC, IT, MOD, MP3, OGG, OPUS, S3M and WAV)
- Touch screen.

# Build instructions

## Dependencies

Use [dkp-pacman](https://devkitpro.org/wiki/devkitPro_pacman) to install the following dependencies:

```bash
# Build essentials
sudo dkp-pacman -S devkitA64 libnx switch-tools switch-sdl2
# SDL sub-libraries
sudo dkp-pacman -S switch-sdl2_gfx switch-sdl2_image switch-sdl2_ttf switch-sdl2_mixer
# Opus
sudo dkp-pacman -S switch-libopus switch-opusfile
```

**Note:** NX-Shell is based on [libnx](https://github.com/switchbrew/libnx).

## Start building

```bash
git clone --recursive https://github.com/joel16/NX-Shell.git
cd NX-Shell
make
```

# Credits:

- **Preetisketch** for some of the assets used as well as the banner.
- **StevenMattera** for the implementing the foundation of all touch-screen code.
- **rock88** for integrating mupdf, allowing NX-Shell to read pdfs and other epubs.
- **grimfang4** for the original SDL_FontCache headers and **BernardoGiordano** for the port to switch.
- **theMealena** for CVE_gif*.
