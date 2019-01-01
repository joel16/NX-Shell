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
- Audio playback. (Following formats are supported: MP3, OGG, WAV and MOD)
- Touch screen.
- Ebook reader.

# Build instructions

```bash
git clone --recursive https://github.com/joel16/NX-Shell.git
cd NX-Shell/mupdf/
git checkout 1.14.0 && make generate
cd ../ && make
```

# Credits:

- **Preetisketch** for some of the assets used as well as the banner.
- **StevenMattera** for the implementing the foundation of all touch-screen code.
- **rock88** for integrating mupdf, allowing NX-Shell to read pdfs and other epubs.
- **grimfang4** for the original SDL_FontCache headers and **BernardoGiordano** for the port to switch.
- **theMealena** for CVE_gif*.
