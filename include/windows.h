#ifndef NX_SHELL_WINDOWS_H
#define NX_SHELL_WINDOWS_H

#include <switch.h>
#include <vector>

#include "imgui.h"

enum WINDOW_STATES {
    WINDOW_STATE_FILEBROWSER = 0,
    WINDOW_STATE_SETTINGS,
    WINDOW_STATE_OPTIONS,
    WINDOW_STATE_DELETE,
    WINDOW_STATE_PROPERTIES,
    WINDOW_STATE_IMAGEVIEWER,
    WINDOW_STATE_ARCHIVEEXTRACT,
    WINDOW_STATE_TEXTREADER
};

enum FS_SORT_STATE {
    FS_SORT_ALPHA_ASC = 0,
    FS_SORT_ALPHA_DESC,
    FS_SORT_SIZE_ASC,
    FS_SORT_SIZE_DESC
};

typedef struct {
    std::vector<bool> checked;
    std::vector<bool> checked_copy;
    char cwd[FS_MAX_PATH + 1] = "";
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

extern WindowData data;
extern int sort;

namespace FileBrowser {
    bool Sort(const FsDirectoryEntry &entryA, const FsDirectoryEntry &entryB);
}

namespace Windows {
    void ResetCheckbox(WindowData &data);
    void MainWindow(WindowData &data, u64 &key, bool progress);
}

#endif
