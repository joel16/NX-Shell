#ifndef NX_SHELL_WINDOWS_H
#define NX_SHELL_WINDOWS_H

#include "gui.h"
#include "textures.h"

namespace Windows {
    void FileBrowserWindow(MenuItem *item, bool *focus, bool *first_item);
    void ImageWindow(MenuItem *item, Tex *texture);
    void SettingsWindow(MenuItem *item);
}

#endif
