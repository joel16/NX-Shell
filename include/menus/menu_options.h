#ifndef NX_SHELL_MENU_OPTIONS_H
#define NX_SHELL_MENU_OPTIONS_H

#include "touch_helper.h"

/*
*	Copy Flags
*/
#define COPY_FOLDER_RECURSIVE 2
#define COPY_DELETE_ON_FINISH 1
#define COPY_KEEP_ON_FINISH   0
#define NOTHING_TO_COPY      -1

void FileOptions_ResetClipboard(void);
void Menu_ControlDeleteDialog(u64 input, TouchInfo touchInfo);
void Menu_DisplayDeleteDialog(void);
void Menu_ControlProperties(u64 input, TouchInfo touchInfo);
void Menu_DisplayProperties(void);
void Menu_ControlOptions(u64 input, TouchInfo touchInfo);
void Menu_DisplayOptions(void);

#endif
