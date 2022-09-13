#pragma once

#include <string>
#include <switch.h>
#include <vector>
#include <mutex>

#include "textures.hpp"

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
    std::string cwd = "";
    std::string device = "";
    u64 count = 0;
} WindowCheckboxData;

typedef struct {
    WINDOW_STATES state = WINDOW_STATE_FILEBROWSER;
    u64 selected = 0;
    std::vector<FsDirectoryEntry> entries;
    WindowCheckboxData checkbox_data;
    s64 used_storage = 0;
    s64 total_storage = 0;
    std::vector<Tex> textures;
    long unsigned int frame_count = 0;
    float zoom_factor = 1.0f;
} WindowData;

extern WindowData data;
extern int sort;
extern std::vector<std::string> devices_list;
extern std::recursive_mutex devices_list_mutex;

namespace FileBrowser {
    bool Sort(const FsDirectoryEntry &entryA, const FsDirectoryEntry &entryB);
    bool TableSort(const FsDirectoryEntry &entryA, const FsDirectoryEntry &entryB);
}

namespace ImageViewer {
    void ClearTextures(void);
    bool HandleScroll(int index);
    bool HandlePrev(void);
    bool HandleNext(void);
    void HandleControls(u64 &key, bool &properties);
}

namespace Windows {
    void SetupWindow(void);
    void ExitWindow(void);
    void ResetCheckbox(WindowData &data);
    void MainWindow(WindowData &data, u64 &key, bool progress);
    void ImageViewer(bool &properties, bool &file_stat);
}
