#ifndef NX_SHELL_GUI_H
#define NX_SHELL_GUI_H

#include <vector>
#include <string>
#include <SDL.h>
#include <switch.h>

enum MENU_STATES {
    MENU_STATE_HOME,
    MENU_STATE_OPTIONS,
    MENU_STATE_DELETE,
    MENU_STATE_PROPERTIES,
    MENU_STATE_SETTINGS,
    MENU_STATE_IMAGEVIEWER,
    MENU_STATE_ARCHIVEEXTRACT
};

typedef struct {
    int state = 0;
    bool copy = false;
    bool move = false;
    int selected = 0;
    int file_count = 0;
    std::string selected_filename;
    FsDirectoryEntry *entries = nullptr;
    std::vector<bool> checked;
    std::vector<bool> checked_copy;
    std::string checked_cwd;
    int checked_count = 0;
    s64 used_storage = 0;
    s64 total_storage = 0;
} MenuItem;

extern SDL_Window *window;
extern MenuItem item;

namespace GUI {
    inline void ResetCheckbox(void) {
		item.checked.clear();
		item.checked_copy.clear();
		item.checked.resize(item.file_count);
		item.checked.assign(item.checked.size(), false);
		item.checked_cwd.clear();
		item.checked_count = 0;
	};

    int RenderLoop(void);
}

#endif
