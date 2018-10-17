#ifndef NX_SHELL_COMMON_H
#define NX_SHELL_COMMON_H

#include <setjmp.h>

#define ROOT_PATH "/"
#define START_PATH ROOT_PATH
#define MAX_FILES 2048
#define FILES_PER_PAGE 8
#define wait(msec) svcSleepThread(10000000 * (s64)msec)

extern jmp_buf exitJmp;

extern int MENU_DEFAULT_STATE;
extern int BROWSE_STATE;

enum MENU_STATES {
    MENU_STATE_HOME = 0,
    MENU_STATE_OPTIONS = 1,
    MENU_STATE_MENUBAR = 2,
    MENU_STATE_SETTINGS = 3,
    MENU_STATE_FTP = 4,
    MENU_STATE_SORT = 5,
    MENU_STATE_DELETE_DIALOG = 6,
    MENU_STATE_PROPERTIES = 7
};

enum BROWSE_STATES {
    STATE_SD = 0,
    STATE_NAND = 1
};

extern char cwd[512];

#endif