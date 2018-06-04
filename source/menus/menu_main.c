#include <switch.h>

#include "common.h"
#include "config.h"
#include "dirbrowse.h"
#include "menu_main.h"
#include "menu_options.h"
#include "menu_settings.h"
#include "SDL_helper.h"
#include "status_bar.h"
#include "textures.h"

static void Menu_ControlMenuBar(u64 input)
{
	if (input & KEY_A)
		MENU_DEFAULT_STATE = MENU_STATE_SETTINGS;

	if ((input & KEY_Y) || (input & KEY_B))
		MENU_DEFAULT_STATE = MENU_STATE_HOME;
}

static void Menu_DisplayMenuBar(void)
{
	SDL_DrawRect(RENDERER, 0, 0, 400, 720, config_dark_theme? BLACK_BG : WHITE);
	SDL_DrawRect(RENDERER, 400, 0, 1, 720, config_dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT);
	SDL_DrawImage(RENDERER, bg_header, 0, 0, 400, 214);
	SDL_DrawText(Roboto_large, 15, 164, WHITE, "NX Shell");
	SDL_DrawImage(RENDERER, config_dark_theme? icon_sd_dark : icon_sd, 20, 254, 60, 60);
	SDL_DrawText(Roboto, 100, 254, config_dark_theme? WHITE : BLACK, "External storage");
	SDL_DrawText(Roboto_small, 100, 284, config_dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "sdmc");
	int settings_width = 0, settings_height = 0;
	SDL_DrawRect(RENDERER, 10, 630, 80, 80, config_dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
	SDL_DrawImage(RENDERER, config_dark_theme? icon_settings_dark : icon_settings, 20, 640, 60, 60);
}

static void Menu_HomeControls(u64 input)
{
	if (input & KEY_PLUS)
		longjmp(exitJmp, 1);

	if (fileCount > 0)
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

		if (input & KEY_X)
		{
			if (MENU_DEFAULT_STATE == MENU_STATE_OPTIONS)
				MENU_DEFAULT_STATE = MENU_STATE_HOME;
			else
				MENU_DEFAULT_STATE = MENU_STATE_OPTIONS;
		}
		else if (input & KEY_Y)
		{
			if (MENU_DEFAULT_STATE == MENU_STATE_MENUBAR)
				MENU_DEFAULT_STATE = MENU_STATE_HOME;
			else
				MENU_DEFAULT_STATE = MENU_STATE_MENUBAR;
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

static void Menu_Main_Controls(void)
{
	u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
	
	if (MENU_DEFAULT_STATE == MENU_STATE_HOME)
		Menu_HomeControls(kDown);
	else if (MENU_DEFAULT_STATE == MENU_STATE_OPTIONS)
		Menu_ControlOptions(kDown);
	else if (MENU_DEFAULT_STATE == MENU_STATE_PROPERTIES)
		Menu_ControlProperties(kDown);
	else if (MENU_DEFAULT_STATE == MENU_STATE_DIALOG)
		Menu_ControlDeleteDialog(kDown);
	else if (MENU_DEFAULT_STATE == MENU_STATE_MENUBAR)
		Menu_ControlMenuBar(kDown);
}

void Menu_Main(void)
{
	Dirbrowse_PopulateFiles(false);

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
			Menu_DisplayMenuBar();
		else if (MENU_DEFAULT_STATE == MENU_STATE_SETTINGS)
			Menu_DisplaySettings();
		
		SDL_RenderPresent(RENDERER);
	}
}