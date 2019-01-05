#ifndef NX_SHELL_COMMON_H
#define NX_SHELL_COMMON_H

#include <switch.h>
#include <setjmp.h>

#define ROOT_PATH "/"
#define START_PATH ROOT_PATH
#define MAX_FILES 2048
#define FILES_PER_PAGE 8
#define wait(msec) svcSleepThread(10000000 * (s64)msec)

extern jmp_buf exitJmp;

extern int MENU_DEFAULT_STATE;
extern int BROWSE_STATE;
extern FsFileSystem *fs;
extern FsFileSystem sdmc_fs, prodinfo_fs, safe_fs, system_fs, user_fs;
extern u64 total_storage, used_storage;

enum MENU_STATES {
    MENU_STATE_HOME = 0,
    MENU_STATE_OPTIONS = 1,
    MENU_STATE_MENUBAR = 2,
    MENU_STATE_SETTINGS = 3,
    MENU_STATE_FTP = 4,
    MENU_STATE_DELETE_DIALOG = 5,
    MENU_STATE_PROPERTIES = 6
};

enum BROWSE_STATES {
    STATE_SD = 0,
    STATE_PRODINFOF = 1,
    STATE_SAFE = 2,
    STATE_SYSTEM = 3,
    STATE_USER = 4
};

extern char cwd[FS_MAX_PATH];

#endif
