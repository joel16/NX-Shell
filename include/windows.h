#ifndef NX_SHELL_WINDOWS_H
#define NX_SHELL_WINDOWS_H

#include <switch.h>
#include <vector>

#include "imgui.h"

enum WINDOW_STATES {
    WINDOW_STATE_FILEBROWSER,
    WINDOW_STATE_SETTINGS,
    WINDOW_STATE_OPTIONS,
    WINDOW_STATE_DELETE,
    WINDOW_STATE_PROPERTIES,
    WINDOW_STATE_IMAGEVIEWER,
    WINDOW_STATE_ARCHIVEEXTRACT,
    WINDOW_STATE_TEXTREADER
};

typedef struct {
    std::vector<bool> checked;
    std::vector<bool> checked_copy;
    std::string cwd;
    u64 count = 0;
} WindowCheckboxData;

typedef struct {
    WINDOW_STATES state = WINDOW_STATE_FILEBROWSER;
    u64 selected = 0;
    std::vector<FsDirectoryEntry> entries;
    WindowCheckboxData checkbox_data;
    s64 used_storage = 0;
    s64 total_storage = 0;
} WindowData;

namespace Windows {
    void ResetCheckbox(WindowData &data);
    void MainWindow(WindowData &data, u64 &key);
}

#endif
