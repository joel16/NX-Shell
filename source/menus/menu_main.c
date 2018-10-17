#include <switch.h>
#include <math.h>

#include "common.h"
#include "config.h"
#include "dirbrowse.h"
#include "menu_main.h"
#include "menu_options.h"
#include "menu_settings.h"
#include "SDL_helper.h"
#include "status_bar.h"
#include "textures.h"
#include "touch_helper.h"
#include "utils.h"

#define MENUBAR_X_BOUNDARY  0
static float menubar_x = -400.0;
static char multi_select_dir_old[512];

void AnimateMenuBar(float delta_time) {  
    menubar_x += 400.0f * (delta_time * 0.001);
	
	if (menubar_x > 0)
		menubar_x = MENUBAR_X_BOUNDARY;
}

static void Menu_ControlMenuBar(u64 input, TouchInfo touchInfo) {
	if (input & KEY_A)
		MENU_DEFAULT_STATE = MENU_STATE_SETTINGS;

	if ((input & KEY_MINUS) || (input & KEY_B))
		MENU_DEFAULT_STATE = MENU_STATE_HOME;
	
	if ((touchInfo.state == TouchEnded) && (touchInfo.tapType != TapNone)) {
		if (touchInfo.firstTouch.px >= menubar_x + 400)
			MENU_DEFAULT_STATE = MENU_STATE_HOME;
		else if (tapped_inside(touchInfo, menubar_x + 20, 630, menubar_x + 80, 710))
			MENU_DEFAULT_STATE = MENU_STATE_SETTINGS;
	}
}

static void Menu_DisplayMenuBar(void) {
	SDL_DrawRect(menubar_x, 0, 400, 720, config_dark_theme? BLACK_BG : WHITE);
	SDL_DrawRect(menubar_x + 400, 0, 1, 720, config_dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT);
	SDL_DrawImage(bg_header, menubar_x, 0);
	SDL_DrawText(menubar_x + 15, 164, 30, WHITE, "NX Shell");
	SDL_DrawImage(config_dark_theme? icon_sd_dark : icon_sd, menubar_x + 20, 254);
	SDL_DrawText(menubar_x + 100, 254, 25, config_dark_theme? WHITE : BLACK, "External storage");
	SDL_DrawText(menubar_x + 100, 284, 20, config_dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "sdmc");
	SDL_DrawRect(menubar_x + 10, 630, 80, 80, config_dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
	SDL_DrawImage(config_dark_theme? icon_settings_dark : icon_settings, menubar_x + 20, 640);
}

static void Menu_HandleMultiSelect(void) {
	// multi_select_dir can only hold one dir
	strcpy(multi_select_dir_old, cwd);
	if (strcmp(multi_select_dir_old, multi_select_dir) != 0)
		FileOptions_ResetClipboard();

	char path[512];
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
	if (input & KEY_PLUS)
		longjmp(exitJmp, 1);

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

		Utils_SetMax(&position, 0, ((strcmp(cwd, ROOT_PATH) == 0? (fileCount - 1) : fileCount)));
		Utils_SetMin(&position, ((strcmp(cwd, ROOT_PATH) == 0? (fileCount - 1) : fileCount)), 0);

		if (input & KEY_LEFT)
			position = 0;
		else if (input & KEY_RIGHT)
			position = ((strcmp(cwd, ROOT_PATH) == 0? (fileCount - 1) : fileCount));

		// Open options
		if (input & KEY_X) {
			if (MENU_DEFAULT_STATE == MENU_STATE_OPTIONS)
				MENU_DEFAULT_STATE = MENU_STATE_HOME;
			else
				MENU_DEFAULT_STATE = MENU_STATE_OPTIONS;
		}

		if (input & KEY_Y)
			Menu_HandleMultiSelect();

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
		int lastPosition = ((strcmp(cwd, ROOT_PATH) == 0? (fileCount - 1) : fileCount));
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

			position = tapped_selection;

			if ((touchInfo.firstTouch.px >= 0) && (touchInfo.firstTouch.px <= 80 )) {
				wait(1);
				Menu_HandleMultiSelect();
			}
			else {
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

	u64 current_time = 0, last_time = 0;

	while(appletMainLoop()) {
		last_time = current_time;
    	current_time = SDL_GetPerformanceCounter();
		double delta_time = (double)((current_time - last_time) * 1000 / SDL_GetPerformanceFrequency());

		SDL_ClearScreen(config_dark_theme? BLACK_BG : WHITE);
		SDL_DrawRect(0, 0, 1280, 40, config_dark_theme? STATUS_BAR_DARK : STATUS_BAR_LIGHT);	// Status bar
		SDL_DrawRect(0, 40, 1280, 100, config_dark_theme? MENU_BAR_DARK : MENU_BAR_LIGHT);	// Menu bar
		
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

		SDL_Renderdisplay();
	}
}
