#ifndef NX_SHELL_MENU_FILEOPTIONS_H
#define NX_SHELL_MENU_FILEOPTIONS_H

#include "touch_helper.h"

void FileOptions_ResetClipboard(void);
void Menu_ControlDeleteDialog(u64 input, TouchInfo touchInfo);
void Menu_DisplayDeleteDialog(void);
void Menu_ControlProperties(u64 input, TouchInfo touchInfo);
void Menu_DisplayProperties(void);
void Menu_ControlOptions(u64 input, TouchInfo touchInfo);
void Menu_DisplayOptions(void);

#endif
