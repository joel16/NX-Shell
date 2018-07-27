#include <math.h>

#include "common.h"
#include "config.h"
#include "dirbrowse.h"
#include "menu_settings.h"
#include "SDL_helper.h"
#include "status_bar.h"
#include "textures.h"
#include "touch_helper.h"
#include "utils.h"

static bool displayAbout;
static int confirm_width = 0, confirm_height = 0;
static int dialog_width = 0, dialog_height = 0;

static void Save_SortConfig(int selection)
{
	switch (selection)
	{
		case 0:
			config_sort_by = 0;
			break;
		case 1:
			config_sort_by = 1;
			break;
		case 2:
			config_sort_by = 2;
			break;
		case 3:
			config_sort_by = 3;
			break;
	}

	Config_Save(config_dark_theme, config_sort_by);
}

static void Menu_DisplaySortSettings(void)
{
	int selection = 0, max_items = 3, height = 0;
	TouchInfo touchInfo;
	Touch_Init(&touchInfo);

	int title_height = 0;
	TTF_SizeText(Roboto_large, "Settings", NULL, &title_height);

	const char *main_menu_items[] =
	{
		"By name (ascending)",
		"By name (descending)",
		"By size (largest first)",
		"By size (smallest first)"
	};

	int radio_button_width = 0, radio_button_height = 0; // 1180
	SDL_QueryTexture(icon_radio_dark_on, NULL, NULL, &radio_button_width, &radio_button_height);

	while(appletMainLoop())
	{
		SDL_ClearScreen(RENDERER, config_dark_theme? BLACK_BG : WHITE);
		SDL_RenderClear(RENDERER);
		SDL_DrawRect(RENDERER, 0, 0, 1280, 40, config_dark_theme? STATUS_BAR_DARK : STATUS_BAR_LIGHT);	// Status bar
		SDL_DrawRect(RENDERER, 0, 40, 1280, 100, config_dark_theme? MENU_BAR_DARK : MENU_BAR_LIGHT);	// Menu bar

		StatusBar_DisplayTime();

		SDL_DrawImage(RENDERER, icon_back, 40, 66);
		SDL_DrawText(RENDERER, Roboto_large, 128, 40 + ((100 - title_height)/2), WHITE, "Sorting Options");

		int printed = 0; // Print counter

		for (int i = 0; i < max_items + 1; i++)
		{
			if (printed == FILES_PER_PAGE)
				break;

			if (selection < FILES_PER_PAGE || i > (selection - FILES_PER_PAGE))
			{
				if (i == selection)
					SDL_DrawRect(RENDERER, 0, 140 + (73 * printed), 1280, 73, config_dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
				
				TTF_SizeText(Roboto, main_menu_items[i], NULL, &height);
				SDL_DrawText(RENDERER, Roboto, 40, 140 + ((73 - height)/2) + (73 * printed), config_dark_theme? WHITE : BLACK, main_menu_items[i]);

				printed++;
			}
		}

		config_sort_by == 0? SDL_DrawImage(RENDERER, config_dark_theme? icon_radio_dark_on : icon_radio_on, (1170 - radio_button_width), 152) : 
			SDL_DrawImage(RENDERER, config_dark_theme? icon_radio_dark_off : icon_radio_off, (1170 - radio_button_width), 152);
		
		config_sort_by == 1? SDL_DrawImage(RENDERER, config_dark_theme? icon_radio_dark_on : icon_radio_on, (1170 - radio_button_width), 225) : 
			SDL_DrawImage(RENDERER, config_dark_theme? icon_radio_dark_off : icon_radio_off, (1170 - radio_button_width), 225);

		config_sort_by == 2? SDL_DrawImage(RENDERER, config_dark_theme? icon_radio_dark_on : icon_radio_on, (1170 - radio_button_width), 298) : 
			SDL_DrawImage(RENDERER, config_dark_theme? icon_radio_dark_off : icon_radio_off, (1170 - radio_button_width), 298);
		
		config_sort_by == 3? SDL_DrawImage(RENDERER, config_dark_theme? icon_radio_dark_on : icon_radio_on, (1170 - radio_button_width), 371) : 
			SDL_DrawImage(RENDERER, config_dark_theme? icon_radio_dark_off : icon_radio_off, (1170 - radio_button_width), 371);
		
		SDL_RenderPresent(RENDERER);

		hidScanInput();
		Touch_Process(&touchInfo);
		u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

		if (kDown & KEY_B)
			break;

		if (kDown & KEY_DDOWN)
			selection++;
		else if (kDown & KEY_DUP)
			selection--;

		Utils_SetMax(&selection, 0, max_items);
		Utils_SetMin(&selection, max_items, 0);

		if (kDown & KEY_A)
			Save_SortConfig(selection);

		if (touchInfo.state == TouchStart)
		{
			int touch_selection = floor(((double) touchInfo.firstTouch.py - 140) / 73);

			if (touch_selection >= 0 && touch_selection <= max_items)
				selection = touch_selection;
		}
		else if (touchInfo.state == TouchEnded && touchInfo.tapType != TapNone)
		{
			if (tapped_inside(touchInfo, 40, 66, 108, 114))
				break;
			else if (touchInfo.firstTouch.py >= 140)
			{
				int tapped_selection = floor(((double) touchInfo.firstTouch.py - 140) / 73);
				Save_SortConfig(tapped_selection);
			}
		}
	}
	Dirbrowse_PopulateFiles(true);
}

static void Menu_ControlAboutDialog(u64 input)
{
	if ((input & KEY_A) || (input & KEY_B))
		displayAbout = false;
}

static void Menu_TouchAboutDialog(TouchInfo touchInfo)
{
	if (touchInfo.state == TouchEnded && touchInfo.tapType != TapNone) 
	{
		// Touched outside
		if (tapped_outside(touchInfo, (1280 - dialog_width) / 2, (720 - dialog_height) / 2, (1280 + dialog_width) / 2, (720 + dialog_height) / 2))
			displayAbout = false;
		// Confirm Button
		if (tapped_inside(touchInfo, 1010 - confirm_width, (720 - dialog_height) / 2 + 225, 1050 + confirm_width, (720 - dialog_height) / 2 + 265 + confirm_height))
			displayAbout = false;
	}
}

static void Menu_DisplayAboutDialog(void)
{
	int text1_width = 0, text2_width = 0, text3_width = 0, text4_width = 0, text5_width = 0;
	TTF_SizeText(Roboto_small, "NX Shell vX.X.X", &text1_width, NULL);
	TTF_SizeText(Roboto_small, "Author: Joel16", &text2_width, NULL);
	TTF_SizeText(Roboto_small, "Graphics: Preetisketch and CyanogenMod/LineageOS contributors", &text3_width, NULL);
	TTF_SizeText(Roboto_small, "Touch screen: StevenMattera", &text4_width, NULL);
	TTF_SizeText(Roboto_small, "E-Book Reader: rock88", &text5_width, NULL);

	TTF_SizeText(Roboto, "OK", &confirm_width, &confirm_height);

	SDL_QueryTexture(dialog, NULL, NULL, &dialog_width, &dialog_height);

	SDL_DrawImage(RENDERER, config_dark_theme? dialog_dark : dialog, ((1280 - (dialog_width)) / 2), ((720 - (dialog_height)) / 2));
	SDL_DrawText(RENDERER, Roboto, ((1280 - (dialog_width)) / 2) + 80, ((720 - (dialog_height)) / 2) + 45, config_dark_theme? TITLE_COLOUR_DARK : TITLE_COLOUR, "About");
	SDL_DrawTextf(RENDERER, Roboto_small, ((1280 - (text1_width)) / 2), ((720 - (dialog_height)) / 2) + 70, config_dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "NX Shell v%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO);
	SDL_DrawText(RENDERER, Roboto_small, ((1280 - (text2_width)) / 2), ((720 - (dialog_height)) / 2) + 100, config_dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "Author: Joel16");
	SDL_DrawText(RENDERER, Roboto_small, ((1280 - (text3_width)) / 2), ((720 - (dialog_height)) / 2) + 130, config_dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "Graphics: Preetisketch and CyanogenMod/LineageOS contributors");
	SDL_DrawText(RENDERER, Roboto_small, ((1280 - (text4_width)) / 2), ((720 - (dialog_height)) / 2) + 160, config_dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "Touch screen: StevenMattera");
	SDL_DrawText(RENDERER, Roboto_small, ((1280 - (text5_width)) / 2), ((720 - (dialog_height)) / 2) + 190, config_dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "E-Book Reader: rock88");

	SDL_DrawRect(RENDERER, (1030 - (confirm_width)) - 20, (((720 - (dialog_height)) / 2) + 245) - 20, confirm_width + 40, confirm_height + 40, config_dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
	SDL_DrawText(RENDERER, Roboto, 1030 - (confirm_width), ((720 - (dialog_height)) / 2) + 245, config_dark_theme? TITLE_COLOUR_DARK : TITLE_COLOUR, "OK");
}

void Menu_DisplaySettings(void)
{
	int selection = 0, max_items = 2, height = 0;
	TouchInfo touchInfo;
	Touch_Init(&touchInfo);

	int title_height = 0;
	TTF_SizeText(Roboto_large, "Settings", NULL, &title_height);

	const char *main_menu_items[] =
	{
		"Sorting options",
		"Dark theme",
		"About"
	};

	int toggle_button_width = 0, toggle_button_height = 0; //1180
	SDL_QueryTexture(icon_toggle_on, NULL, NULL, &toggle_button_width, &toggle_button_height);

	displayAbout = false;

	while(appletMainLoop())
	{
		SDL_ClearScreen(RENDERER, config_dark_theme? BLACK_BG : WHITE);
		SDL_RenderClear(RENDERER);
		SDL_DrawRect(RENDERER, 0, 0, 1280, 40, config_dark_theme? STATUS_BAR_DARK : STATUS_BAR_LIGHT);	// Status bar
		SDL_DrawRect(RENDERER, 0, 40, 1280, 100, config_dark_theme? MENU_BAR_DARK : MENU_BAR_LIGHT);	// Menu bar

		StatusBar_DisplayTime();

		SDL_DrawImage(RENDERER, icon_back, 40, 66);
		SDL_DrawText(RENDERER, Roboto_large, 128, 40 + ((100 - title_height)/2), WHITE, "Settings");

		int printed = 0; // Print counter

		for (int i = 0; i < max_items + 1; i++)
		{
			if (printed == FILES_PER_PAGE)
				break;

			if (selection < FILES_PER_PAGE || i > (selection - FILES_PER_PAGE))
			{
				if (i == selection)
					SDL_DrawRect(RENDERER, 0, 140 + (73 * printed), 1280, 73, config_dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);

				if (config_dark_theme)
					SDL_DrawImage(RENDERER, config_dark_theme? icon_toggle_dark_on : icon_toggle_off, 1180 - toggle_button_width, 213 + ((73 - toggle_button_height) / 2));
				else
					SDL_DrawImage(RENDERER, config_dark_theme? icon_toggle_on : icon_toggle_off, 1180 - toggle_button_width, 213 + ((73 - toggle_button_height) / 2));
				
				TTF_SizeText(Roboto, main_menu_items[i], NULL, &height);
				SDL_DrawText(RENDERER, Roboto, 40, 140 + ((73 - height)/2) + (73 * printed), config_dark_theme? WHITE : BLACK, main_menu_items[i]);

				printed++;
			}
		}

		if (displayAbout)
			Menu_DisplayAboutDialog();

		SDL_RenderPresent(RENDERER);

		hidScanInput();
		Touch_Process(&touchInfo);
		u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

		if (displayAbout)
		{
			Menu_ControlAboutDialog(kDown);
			Menu_TouchAboutDialog(touchInfo);
		}
		else
		{
			if (kDown & KEY_B)
				break;
			
			if (kDown & KEY_DDOWN)
				selection++;
			else if (kDown & KEY_DUP)
				selection--;

			Utils_SetMax(&selection, 0, max_items);
			Utils_SetMin(&selection, max_items, 0);

			if (kDown & KEY_A)
			{
				switch (selection)
				{
					case 0:
						Menu_DisplaySortSettings();
						break;
					case 1:
						config_dark_theme = !config_dark_theme;
						Config_Save(config_dark_theme, config_sort_by);
						break;
					case 2:
						displayAbout = true;
						break;
				}
			}

			if (touchInfo.state == TouchStart)
			{
				int touch_selection = floor(((double) touchInfo.firstTouch.py - 140) / 73);

				if (touch_selection > 0 && touch_selection <= max_items)
					selection = touch_selection;
			}
			else if (touchInfo.state == TouchEnded && touchInfo.tapType != TapNone) 
			{
				if (tapped_inside(touchInfo, 40, 66, 108, 114))
					break;
				else if (touchInfo.firstTouch.py >= 140)
				{
					int tapped_selection = floor(((double) touchInfo.firstTouch.py - 140) / 73);

					switch (tapped_selection)
					{
						case 0:
							Menu_DisplaySortSettings();
							break;
						case 1:
							config_dark_theme = !config_dark_theme;
							Config_Save(config_dark_theme, config_sort_by);
							break;
						case 2:
							displayAbout = true;
							break;
					}

					Config_Save(config_dark_theme, config_sort_by);
				}
			}
		}
	}

	MENU_DEFAULT_STATE = MENU_STATE_HOME;
}