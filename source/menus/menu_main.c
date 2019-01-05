#include <math.h>

#include "common.h"
#include "config.h"
#include "dirbrowse.h"
#include "fs.h"
#include "menu_main.h"
#include "menu_options.h"
#include "menu_ftp.h"
#include "menu_settings.h"
#include "SDL_helper.h"
#include "status_bar.h"
#include "textures.h"
#include "touch_helper.h"
#include "utils.h"

#define MENUBAR_X_BOUNDARY  0
static int menubar_selection = 0, horizantal_selection = 0, menubar_max_items = 4;
static float menubar_x = -400.0;
static char multi_select_dir_old[FS_MAX_PATH];

void AnimateMenuBar(float delta_time) {
	menubar_x += 1 * delta_time;
	
	if (menubar_x > 0)
		menubar_x = MENUBAR_X_BOUNDARY;
}

static void Mount_SD(void) {
	fs = fsdevGetDefaultFileSystem();
	BROWSE_STATE = STATE_SD;

	total_storage = Utils_GetTotalStorage(fs);
	used_storage = Utils_GetUsedStorage(fs);

	Config_GetLastDirectory();
	Dirbrowse_PopulateFiles(true);
}

static void Mount_Prodinfof(void) {
	fs = &prodinfo_fs;
	BROWSE_STATE = STATE_PRODINFOF;

	total_storage = Utils_GetTotalStorage(fs);
	used_storage = Utils_GetUsedStorage(fs);

	strcpy(cwd, ROOT_PATH);
	Dirbrowse_PopulateFiles(true);
}

static void Mount_Safe(void) {
	fs = &safe_fs;
	BROWSE_STATE = STATE_SAFE;

	total_storage = Utils_GetTotalStorage(fs);
	used_storage = Utils_GetUsedStorage(fs);

	strcpy(cwd, ROOT_PATH);
	Dirbrowse_PopulateFiles(true);
}

static void Mount_System(void) {
	fs = &system_fs;
	BROWSE_STATE = STATE_SYSTEM;

	total_storage = Utils_GetTotalStorage(fs);
	used_storage = Utils_GetUsedStorage(fs);

	strcpy(cwd, ROOT_PATH);
	Dirbrowse_PopulateFiles(true);
}

static void Mount_User(void) {
	fs = &user_fs;
	BROWSE_STATE = STATE_USER;

	total_storage = Utils_GetTotalStorage(fs);
	used_storage = Utils_GetUsedStorage(fs);

	strcpy(cwd, ROOT_PATH);
	Dirbrowse_PopulateFiles(true);
}

static void Menu_ControlMenuBar(u64 input, TouchInfo touchInfo) {
	if (input & KEY_DUP) {
		if (menubar_selection == 0) {
			horizantal_selection = 1;
			menubar_selection--;
		}
		else if ((menubar_selection == 5) && (horizantal_selection == 1))
			horizantal_selection = 0;
		else
			menubar_selection--;
	}
	else if (input & KEY_DDOWN) {
		if (menubar_selection == 4) {
			horizantal_selection = 0;
			menubar_selection++;
		}
		else if ((menubar_selection == 5) && (horizantal_selection == 0))
			horizantal_selection = 1;
		else
			menubar_selection++;
	}

	Utils_SetMax(&menubar_selection, 0, menubar_max_items + 1);
	Utils_SetMin(&menubar_selection, menubar_max_items + 1, 0);
	Utils_SetMax(&horizantal_selection, 0, 1);
	Utils_SetMin(&horizantal_selection, 1, 0);

	if (menubar_selection == 5) {
		if (input & KEY_DLEFT)
			horizantal_selection--;
		else if (input & KEY_DRIGHT)
			horizantal_selection++;
	}

	if (input & KEY_A) {
		if (menubar_selection == 5) {
			if (horizantal_selection == 1)
				MENU_DEFAULT_STATE = MENU_STATE_SETTINGS;
			else
				MENU_DEFAULT_STATE = MENU_STATE_FTP;
		}
		else {
			switch (menubar_selection) {
				case 0:
					Mount_SD();
					break;
				case 1:
					Mount_Prodinfof();
					break;
				case 2:
					Mount_Safe();
					break;
				case 3:
					Mount_System();
					break;
				case 4:
					Mount_User();
					break;
			}
		}
	}

	if ((input & KEY_MINUS) || (input & KEY_B)) {
		menubar_selection = 0;
		MENU_DEFAULT_STATE = MENU_STATE_HOME;
	}
	
	if ((touchInfo.state == TouchEnded) && (touchInfo.tapType != TapNone)) {
		if (touchInfo.firstTouch.px >= menubar_x + 400)
			MENU_DEFAULT_STATE = MENU_STATE_HOME;
		else if (tapped_inside(touchInfo, menubar_x, 214, 400, 293)) {
			menubar_selection = 0;
			Mount_SD();
		}
		else if (tapped_inside(touchInfo, menubar_x, 294, 400, 373)) {
			menubar_selection = 1;
			Mount_Prodinfof();
		}
		else if (tapped_inside(touchInfo, menubar_x, 374, 400, 453)) {
			menubar_selection = 2;
			Mount_Safe();
		}
		else if (tapped_inside(touchInfo, menubar_x, 454, 400, 533)) {
			menubar_selection = 3;
			Mount_System();
		}
		else if (tapped_inside(touchInfo, menubar_x, 534, 400, 613)) {
			menubar_selection = 4;
			Mount_User();
		}
		else if (tapped_inside(touchInfo, menubar_x + 20, 630, menubar_x + 80, 710))
			MENU_DEFAULT_STATE = MENU_STATE_FTP;
		else if (tapped_inside(touchInfo, menubar_x + 120, 630, menubar_x + 180, 710))
			MENU_DEFAULT_STATE = MENU_STATE_SETTINGS;
	}
}

static void Menu_DisplayMenuBar(void) {
	const char *menubar_items[] = {
		"External storage",
		"CalibrationFile",
		"SafeMode",
		"System",
		"User",
	};

	const char *menubar_desc[] = {
		"/",
		"PRODINFOF:/",
		"SAFE:/",
		"SYSTEM:/",
		"USER:/",
	};

	SDL_DrawRect(menubar_x, 0, 400, 720, config.dark_theme? BLACK_BG : WHITE);
	SDL_DrawRect(menubar_x + 400, 0, 1, 720, config.dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT);
	SDL_DrawImage(bg_header, menubar_x, 0);
	SDL_DrawText(menubar_x + 15, 164, 30, WHITE, "NX Shell");

	int printed = 0, selection = 0; 

	if (menubar_selection == menubar_max_items + 1) {
		if (horizantal_selection == 1)
			SDL_DrawRect(menubar_x + 110, 630, 80, 80, config.dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
		else
			SDL_DrawRect(menubar_x + 10, 630, 80, 80, config.dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
	}
	else
		selection = menubar_selection;

	for (int i = 0; i < menubar_max_items + 1; i++) {
		if (printed == 5)
				break;
		if (selection < 5 || i > (selection - 5)) {
			if ((i == selection) && (menubar_selection != menubar_max_items + 1))
				SDL_DrawRect(menubar_x, 214 + (80 * printed), 400, 80, config.dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);

			SDL_DrawText(menubar_x + 100, 229 + (80 * printed), 25, config.dark_theme? WHITE : BLACK, menubar_items[i]);
			SDL_DrawText(menubar_x + 100, 259 + (80 * printed), 20, config.dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, menubar_desc[i]);

			if (!strcmp(menubar_items[i], "External storage"))
				SDL_DrawImage(config.dark_theme? icon_sd_dark : icon_sd, menubar_x + 20, 224 + (80 * printed));
			else
				SDL_DrawImage(config.dark_theme? icon_secure_dark : icon_secure, menubar_x + 20, 224 + (80 * printed));

			printed++;
		}
	}

	SDL_DrawImage(config.dark_theme? icon_ftp_dark : icon_ftp, menubar_x + 20, 640);
	SDL_DrawImage(config.dark_theme? icon_settings_dark : icon_settings, menubar_x + 120, 640);
}

static void Menu_HandleMultiSelect(int position) {
	// multi_select_dir can only hold one dir
	strcpy(multi_select_dir_old, cwd);
	if (strcmp(multi_select_dir_old, multi_select_dir) != 0)
		FileOptions_ResetClipboard();

	char path[FS_MAX_PATH];
	File *file = Dirbrowse_GetFileIndex(position);
	strcpy(path, cwd);
	strcpy(path + strlen(path), file->name);
	strcpy(multi_select_dir, cwd);
			
	if (!multi_select[position]) {
		multi_select[position] = true;
		multi_select_indices[position] = multi_select_index; // Store the index in the position
		Utils_AppendArr(multi_select_paths[multi_select_index], path, multi_select_index);
		multi_select_index += 1;
	}
	else {
		multi_select[position] = false;
		strcpy(multi_select_paths[multi_select_indices[position]], "");
		multi_select_indices[position] = -1;
	}

	Utils_SetMax(&multi_select_index, 0, 50);
	Utils_SetMin(&multi_select_index, 50, 0);
}

static void Menu_ControlHome(u64 input, TouchInfo touchInfo) {
	if (input & KEY_MINUS) {
		if (MENU_DEFAULT_STATE == MENU_STATE_MENUBAR)
			MENU_DEFAULT_STATE = MENU_STATE_HOME;
		else {
			menubar_x = -400;
			MENU_DEFAULT_STATE = MENU_STATE_MENUBAR;
		}
	}

	if (fileCount >= 0) {
		if (input & KEY_DUP)
			position--;
		else if (input & KEY_DDOWN)
			position++;

		if ((hidKeysHeld(CONTROLLER_P1_AUTO) & KEY_RSTICK_UP) || (hidKeysHeld(CONTROLLER_P1_AUTO) & KEY_LSTICK_UP)) {
			wait(5);
			position--;
		}
		else if ((hidKeysHeld(CONTROLLER_P1_AUTO) & KEY_RSTICK_DOWN) || (hidKeysHeld(CONTROLLER_P1_AUTO) & KEY_LSTICK_DOWN)) {
			wait(5);
			position++;
		}

		Utils_SetMax(&position, 0, (fileCount - 1));
		Utils_SetMin(&position, (fileCount - 1), 0);

		if (input & KEY_LEFT)
			position = 0;
		else if (input & KEY_RIGHT)
			position = (fileCount - 1);

		// Open options
		if (input & KEY_X) {
			if (MENU_DEFAULT_STATE == MENU_STATE_OPTIONS)
				MENU_DEFAULT_STATE = MENU_STATE_HOME;
			else
				MENU_DEFAULT_STATE = MENU_STATE_OPTIONS;
		}

		if (input & KEY_Y)
			Menu_HandleMultiSelect(position);

		if (input & KEY_A) {
			wait(5);
			Dirbrowse_OpenFile();
		}
		else if ((strcmp(cwd, ROOT_PATH) != 0) && (input & KEY_B)) {
			wait(5);
			Dirbrowse_Navigate(true);
			Dirbrowse_PopulateFiles(true);
		}
	}

	if (touchInfo.state == TouchStart && tapped_inside(touchInfo, 0, 140, 1280, 720))
		initialPosition = (position == 0) ? 7 : position;
	else if (touchInfo.state == TouchMoving && touchInfo.tapType == TapNone && tapped_inside(touchInfo, 0, 140, 1280, 720)) {
		int lastPosition = position = fileCount - 1;
		if (lastPosition < 8)
			return;
		position = initialPosition + floor(((double) touchInfo.firstTouch.py - (double) touchInfo.prevTouch.py) / 73);
		
		if (position < 7)
			position = 7;
		else if (position >= lastPosition)
			position = lastPosition;
	}
	else if (touchInfo.state == TouchEnded && touchInfo.tapType != TapNone) {
		if (tapped_inside(touchInfo, (1260 - 64), 58, (1260 - 64) + 64, (58 + 64))) {
			if (MENU_DEFAULT_STATE == MENU_STATE_OPTIONS)
				MENU_DEFAULT_STATE = MENU_STATE_HOME;
			else
				MENU_DEFAULT_STATE = MENU_STATE_OPTIONS;
		}
		else if (tapped_inside(touchInfo, 20, 66, 68, 114)) {
			menubar_x = -400;
			MENU_DEFAULT_STATE = MENU_STATE_MENUBAR;
		}
		else if (touchInfo.firstTouch.py >= 140) {
			int tapped_selection = floor(((double) touchInfo.firstTouch.py - 140) / 73);

			if (position > 7)
				tapped_selection += position - 7;

			if ((touchInfo.firstTouch.px >= 0) && (touchInfo.firstTouch.px <= 80)) {
				wait(1);
				Menu_HandleMultiSelect(tapped_selection);
			}
			else {
				position = tapped_selection;
				if ((strcmp(cwd, ROOT_PATH) != 0 && position == 0) || touchInfo.tapType == TapShort) {
					wait(1);
					Dirbrowse_OpenFile();
				}
				else if (touchInfo.tapType == TapLong)
					MENU_DEFAULT_STATE = MENU_STATE_OPTIONS;
			}
		}
	}
}

void Menu_Main(void) {
	TouchInfo touchInfo;
	Touch_Init(&touchInfo);

	Dirbrowse_PopulateFiles(false);
	memset(multi_select, 0, sizeof(multi_select)); // Reset all multi selected items

	u64 current_time = SDL_GetPerformanceCounter(), last_time = 0;

	while(appletMainLoop()) {
		last_time = current_time;
		current_time = SDL_GetPerformanceCounter();
		double delta_time = (double)((current_time - last_time) * 1000 / SDL_GetPerformanceFrequency());

		SDL_ClearScreen(config.dark_theme? BLACK_BG : WHITE);
		SDL_DrawRect(0, 0, 1280, 40, config.dark_theme? STATUS_BAR_DARK : STATUS_BAR_LIGHT);	// Status bar
		SDL_DrawRect(0, 40, 1280, 100, config.dark_theme? MENU_BAR_DARK : MENU_BAR_LIGHT);	// Menu bar
		
		StatusBar_DisplayTime();
		Dirbrowse_DisplayFiles();

		hidScanInput();
		Touch_Process(&touchInfo);
		u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
	
		if (MENU_DEFAULT_STATE == MENU_STATE_HOME)
			Menu_ControlHome(kDown, touchInfo);
		else if (MENU_DEFAULT_STATE == MENU_STATE_OPTIONS) {
			Menu_DisplayOptions();
			Menu_ControlOptions(kDown, touchInfo);
		}
		else if (MENU_DEFAULT_STATE == MENU_STATE_PROPERTIES) {
			Menu_DisplayProperties();
			Menu_ControlProperties(kDown, touchInfo);
		}
		else if (MENU_DEFAULT_STATE == MENU_STATE_DELETE_DIALOG) {
			Menu_DisplayDeleteDialog();
			Menu_ControlDeleteDialog(kDown, touchInfo);
		}
		else if (MENU_DEFAULT_STATE == MENU_STATE_MENUBAR) {
			Menu_DisplayMenuBar();
			AnimateMenuBar(delta_time);
			Menu_ControlMenuBar(kDown, touchInfo);
		}
		else if (MENU_DEFAULT_STATE == MENU_STATE_SETTINGS)
			Menu_DisplaySettings();
		else if (MENU_DEFAULT_STATE == MENU_STATE_FTP)
			Menu_FTP();

		SDL_Renderdisplay();

		if (kDown & KEY_PLUS)
			longjmp(exitJmp, 1);
	}
}
