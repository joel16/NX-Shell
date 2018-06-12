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

#define MENUBAR_X_BOUNDARY  0
static int menubar_x = -400;
static TouchInfo touchInfo;

static void Menu_ControlMenuBar(u64 input)
{
	if (input & KEY_A)
		MENU_DEFAULT_STATE = MENU_STATE_SETTINGS;

	if ((input & KEY_Y) || (input & KEY_B))
		MENU_DEFAULT_STATE = MENU_STATE_HOME;
}

static void Menu_TouchMenuBar(TouchInfo touchInfo)
{
	if ((touchInfo.state == TouchEnded) && (touchInfo.tapType != TapNone)) 
	{
		if (touchInfo.firstTouch.px >= menubar_x + 400)
			MENU_DEFAULT_STATE = MENU_STATE_HOME;
		else if (tapped_inside(touchInfo, menubar_x + 20, 630, menubar_x + 80, 710))
			MENU_DEFAULT_STATE = MENU_STATE_SETTINGS;
	}
}

static void Menu_TouchMenuBar(TouchInfo touchInfo)
{
	if (touchInfo.state == TouchEnded && touchInfo.tapType != TapNone) {
		if (touchInfo.firstTouch.px >= menubar_x + 400) {
			MENU_DEFAULT_STATE = MENU_STATE_HOME;
		} else if (tapped_inside(touchInfo, menubar_x + 20, 630, menubar_x + 80, 710)) {
			MENU_DEFAULT_STATE = MENU_STATE_SETTINGS;
		}
	}
}

static void Menu_DisplayMenuBar(void)
{
	SDL_DrawRect(RENDERER, menubar_x, 0, 400, 720, config_dark_theme? BLACK_BG : WHITE);
	SDL_DrawRect(RENDERER, menubar_x + 400, 0, 1, 720, config_dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT);
	SDL_DrawImage(RENDERER, bg_header, menubar_x, 0, 400, 214);
	SDL_DrawText(Roboto_large, menubar_x + 15, 164, WHITE, "NX Shell");
	SDL_DrawImage(RENDERER, config_dark_theme? icon_sd_dark : icon_sd, menubar_x + 20, 254, 60, 60);
	SDL_DrawText(Roboto, menubar_x + 100, 254, config_dark_theme? WHITE : BLACK, "External storage");
	SDL_DrawText(Roboto_small, menubar_x + 100, 284, config_dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "sdmc");
	int settings_width = 0, settings_height = 0;
	SDL_DrawRect(RENDERER, menubar_x + 10, 630, 80, 80, config_dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
	SDL_DrawImage(RENDERER, config_dark_theme? icon_settings_dark : icon_settings, menubar_x + 20, 640, 60, 60);
}

static void Menu_ControlHome(u64 input)
{
	if (input & KEY_PLUS)
		longjmp(exitJmp, 1);

	if (fileCount >= 0)
	{
		if (input & KEY_DUP)
		{
			// Decrease Position
			if (position > 0)
				position--;

			// Rewind Pointer
			else 
				position = ((strcmp(cwd, ROOT_PATH) == 0? (fileCount - 1) : fileCount));
		}
		else if (input & KEY_DDOWN)
		{
			// Increase Position
			if (position < ((strcmp(cwd, ROOT_PATH) == 0? (fileCount - 1) : fileCount)))
				position++;

			// Rewind Pointer
			else 
				position = 0;
		}

		// Open options
		if (input & KEY_X)
		{
			if (MENU_DEFAULT_STATE == MENU_STATE_OPTIONS)
				MENU_DEFAULT_STATE = MENU_STATE_HOME;
			else
				MENU_DEFAULT_STATE = MENU_STATE_OPTIONS;
		}

		// Touch options
		if (touchInfo.state == TouchEnded && touchInfo.tapType != TapNone)
		{
			if (tapped_inside(touchInfo, (1260 - 64), 58, (1260 - 64) + 64, (58 + 64)))
			{
				if (MENU_DEFAULT_STATE == MENU_STATE_OPTIONS)
					MENU_DEFAULT_STATE = MENU_STATE_HOME;
				else
					MENU_DEFAULT_STATE = MENU_STATE_OPTIONS;
			}
		}

		if (input & KEY_Y)
		{
			if (MENU_DEFAULT_STATE == MENU_STATE_MENUBAR)
				MENU_DEFAULT_STATE = MENU_STATE_HOME;
			else {
				menubar_x = -400;
				MENU_DEFAULT_STATE = MENU_STATE_MENUBAR;
			}
		}

		if (input & KEY_A)
		{
			wait(5);
			Dirbrowse_OpenFile();
		}
		else if ((strcmp(cwd, ROOT_PATH) != 0) && (input & KEY_B))
		{
			wait(5);
			Dirbrowse_Navigate(-1);
			Dirbrowse_PopulateFiles(true);
		}
	}
}


static void Menu_TouchHome(TouchInfo touchInfo) 
{
	if (touchInfo.state == TouchStart && tapped_inside(touchInfo, 0, 140, 1280, 720))
		initialPosition = (position == 0) ? 7 : position;
	else if (touchInfo.state == TouchMoving && touchInfo.tapType == TapNone && tapped_inside(touchInfo, 0, 140, 1280, 720))
	{
		int lastPosition = (strcmp(cwd, ROOT_PATH) == 0) ? fileCount - 2 : fileCount - 1;
		if (lastPosition < 8)
			return;
		position = initialPosition + floor(((double) touchInfo.firstTouch.py - (double) touchInfo.prevTouch.py) / 73);
		
		if (position < 7)
			position = 7;
		else if (position >= lastPosition)
			position = lastPosition;
	}
	else if (touchInfo.state == TouchEnded && touchInfo.tapType != TapNone) 
	{
		if (tapped_inside(touchInfo, 20, 66, 68, 114)) 
		{
			menubar_x = -400;
			MENU_DEFAULT_STATE = MENU_STATE_MENUBAR;
		}
		else if (touchInfo.firstTouch.py >= 140)
		{
			int tapped_selection = floor(((double) touchInfo.firstTouch.py - 140) / 73);

			if (position > 7)
				tapped_selection += position - 7;

			position = tapped_selection;
			if ((strcmp(cwd, ROOT_PATH) != 0 && position == 0) || touchInfo.tapType == TapShort) 
				Dirbrowse_OpenFile();
			else if (touchInfo.tapType == TapLong)
				MENU_DEFAULT_STATE = MENU_STATE_OPTIONS;
		}
	}
}

static void Menu_Main_Controls(void)
{
	u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
	Touch_Process(&touchInfo);
	
	if (MENU_DEFAULT_STATE == MENU_STATE_HOME) 
	{
		Menu_ControlHome(kDown);
		Menu_TouchHome(touchInfo);
	}
	else if (MENU_DEFAULT_STATE == MENU_STATE_OPTIONS) 
	{
		Menu_ControlOptions(kDown);
		Menu_TouchOptions(touchInfo);
	}
	else if (MENU_DEFAULT_STATE == MENU_STATE_PROPERTIES) 
	{
		Menu_ControlProperties(kDown);
		Menu_TouchProperties(touchInfo);
	}
	else if (MENU_DEFAULT_STATE == MENU_STATE_DIALOG) 
	{
		Menu_ControlDeleteDialog(kDown);
		Menu_TouchDeleteDialog(touchInfo);
	}
	else if (MENU_DEFAULT_STATE == MENU_STATE_MENUBAR) 
	{
		Menu_ControlMenuBar(kDown);
		Menu_TouchMenuBar(touchInfo);
	}
}

void Menu_Main(void)
{
	Dirbrowse_PopulateFiles(false);
	Touch_Init(&touchInfo);

	while(appletMainLoop())
	{
		SDL_ClearScreen(RENDERER, config_dark_theme? BLACK_BG : WHITE);
		SDL_RenderClear(RENDERER);
		SDL_DrawRect(RENDERER, 0, 0, 1280, 40, config_dark_theme? STATUS_BAR_DARK : STATUS_BAR_LIGHT);	// Status bar
		SDL_DrawRect(RENDERER, 0, 40, 1280, 100, config_dark_theme? MENU_BAR_DARK : MENU_BAR_LIGHT);	// Menu bar
		
		StatusBar_DisplayTime();
		Dirbrowse_DisplayFiles();

		hidScanInput();
		Menu_Main_Controls();

		if (MENU_DEFAULT_STATE == MENU_STATE_OPTIONS)
			Menu_DisplayOptions();
		else if (MENU_DEFAULT_STATE == MENU_STATE_PROPERTIES)
			Menu_DisplayProperties();
		else if (MENU_DEFAULT_STATE == MENU_STATE_DIALOG)
			Menu_DisplayDeleteDialog();
		else if (MENU_DEFAULT_STATE == MENU_STATE_MENUBAR)
		{
			Menu_DisplayMenuBar();

			menubar_x += 35;

			if (menubar_x > -1)
				menubar_x = MENUBAR_X_BOUNDARY;
		}
		else if (MENU_DEFAULT_STATE == MENU_STATE_SETTINGS)
			Menu_DisplaySettings();
		
		SDL_RenderPresent(RENDERER);
	}
}