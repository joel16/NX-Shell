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
static u32 confirm_width = 0, confirm_height = 0;
static int dialog_width = 0, dialog_height = 0;

static void Save_SortConfig(int selection) {
	switch (selection) {
		case 0:
			config.sort = 0;
			break;
		case 1:
			config.sort = 1;
			break;
		case 2:
			config.sort = 2;
			break;
		case 3:
			config.sort = 3;
			break;
	}

	Config_Save(config);
}

static void Menu_DisplaySortSettings(void) {
	int selection = 0, max_items = 3;
	u32 height = 0;
	TouchInfo touchInfo;
	Touch_Init(&touchInfo);

	u32 title_height = 0;
	SDL_GetTextDimensions(30, "Sorting Options", NULL, &title_height);

	const char *main_menu_items[] = {
		"By name (ascending)",
		"By name (descending)",
		"By size (largest first)",
		"By size (smallest first)"
	};

	int radio_button_width = 0, radio_button_height = 0; // 1180
	SDL_QueryTexture(icon_radio_dark_on, NULL, NULL, &radio_button_width, &radio_button_height);

	while(appletMainLoop()) {
		SDL_ClearScreen(config.dark_theme? BLACK_BG : WHITE);
		SDL_DrawRect(0, 0, 1280, 40, config.dark_theme? STATUS_BAR_DARK : STATUS_BAR_LIGHT);	// Status bar
		SDL_DrawRect(0, 40, 1280, 100, config.dark_theme? MENU_BAR_DARK : MENU_BAR_LIGHT);	// Menu bar

		StatusBar_DisplayTime();

		SDL_DrawImage(icon_back, 40, 66);
		SDL_DrawText(128, 40 + ((100 - title_height)/2), 30, WHITE, "Sorting Options");

		int printed = 0; // Print counter

		for (int i = 0; i < max_items + 1; i++) {
			if (printed == FILES_PER_PAGE)
				break;

			if (selection < FILES_PER_PAGE || i > (selection - FILES_PER_PAGE)) {
				if (i == selection)
					SDL_DrawRect(0, 140 + (73 * printed), 1280, 73, config.dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);

				SDL_GetTextDimensions(25, main_menu_items[i], NULL, &height);
				SDL_DrawText(40, 140 + ((73 - height)/2) + (73 * printed), 25, config.dark_theme? WHITE : BLACK, main_menu_items[i]);

				printed++;
			}
		}

		config.sort == 0? SDL_DrawImage(config.dark_theme? icon_radio_dark_on : icon_radio_on, (1170 - radio_button_width), 152) : 
			SDL_DrawImage(config.dark_theme? icon_radio_dark_off : icon_radio_off, (1170 - radio_button_width), 152);
		
		config.sort == 1? SDL_DrawImage(config.dark_theme? icon_radio_dark_on : icon_radio_on, (1170 - radio_button_width), 225) : 
			SDL_DrawImage(config.dark_theme? icon_radio_dark_off : icon_radio_off, (1170 - radio_button_width), 225);

		config.sort == 2? SDL_DrawImage(config.dark_theme? icon_radio_dark_on : icon_radio_on, (1170 - radio_button_width), 298) : 
			SDL_DrawImage(config.dark_theme? icon_radio_dark_off : icon_radio_off, (1170 - radio_button_width), 298);
		
		config.sort == 3? SDL_DrawImage(config.dark_theme? icon_radio_dark_on : icon_radio_on, (1170 - radio_button_width), 371) : 
			SDL_DrawImage(config.dark_theme? icon_radio_dark_off : icon_radio_off, (1170 - radio_button_width), 371);
		
		SDL_Renderdisplay();

		hidScanInput();
		Touch_Process(&touchInfo);
		u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

		if (kDown & KEY_B)
			break;

		if ((kDown & KEY_DDOWN) || (kDown & KEY_LSTICK_DOWN) || (kDown & KEY_RSTICK_DOWN))
			selection++;
		else if ((kDown & KEY_DUP) || (kDown & KEY_LSTICK_UP) || (kDown & KEY_RSTICK_UP))
			selection--;

		Utils_SetMax(&selection, 0, max_items);
		Utils_SetMin(&selection, max_items, 0);

		if (kDown & KEY_A)
			Save_SortConfig(selection);

		if (touchInfo.state == TouchStart) {
			int touch_selection = floor(((double) touchInfo.firstTouch.py - 140) / 73);

			if (touch_selection >= 0 && touch_selection <= max_items)
				selection = touch_selection;
		}
		else if (touchInfo.state == TouchEnded && touchInfo.tapType != TapNone) {
			if (tapped_inside(touchInfo, 40, 66, 108, 114))
				break;
			else if (touchInfo.firstTouch.py >= 140) {
				int tapped_selection = floor(((double) touchInfo.firstTouch.py - 140) / 73);
				Save_SortConfig(tapped_selection);
			}
		}
	}
	Dirbrowse_PopulateFiles(true);
}

static void Menu_ControlAboutDialog(u64 input) {
	if ((input & KEY_A) || (input & KEY_B))
		displayAbout = false;
}

static void Menu_TouchAboutDialog(TouchInfo touchInfo) {
	if (touchInfo.state == TouchEnded && touchInfo.tapType != TapNone) {
		// Touched outside
		if (tapped_outside(touchInfo, (1280 - dialog_width) / 2, (720 - dialog_height) / 2, (1280 + dialog_width) / 2, (720 + dialog_height) / 2))
			displayAbout = false;
		// Confirm Button
		if (tapped_inside(touchInfo, 1010 - confirm_width, (720 - dialog_height) / 2 + 225, 1050 + confirm_width, (720 - dialog_height) / 2 + 265 + confirm_height))
			displayAbout = false;
	}
}

static void Menu_DisplayAboutDialog(void) {
	u32 text1_width = 0, text2_width = 0, text3_width = 0, text4_width = 0, text5_width = 0;
	SDL_GetTextDimensions(20, "NX Shell vX.X.X", &text1_width, NULL);
	SDL_GetTextDimensions(20, "Author: Joel16", &text2_width, NULL);
	SDL_GetTextDimensions(20, "Graphics: Preetisketch and CyanogenMod/LineageOS contributors", &text3_width, NULL);
	SDL_GetTextDimensions(20, "Touch screen: StevenMattera", &text4_width, NULL);
	SDL_GetTextDimensions(20, "E-Book Reader: rock88", &text5_width, NULL);
	SDL_GetTextDimensions(25, "OK", &confirm_width, &confirm_height);

	SDL_QueryTexture(dialog, NULL, NULL, &dialog_width, &dialog_height);

	SDL_DrawImage(config.dark_theme? dialog_dark : dialog, ((1280 - (dialog_width)) / 2), ((720 - (dialog_height)) / 2));
	SDL_DrawText(((1280 - (dialog_width)) / 2) + 80, ((720 - (dialog_height)) / 2) + 45, 25, config.dark_theme? TITLE_COLOUR_DARK : TITLE_COLOUR, "About");
	SDL_DrawTextf(((1280 - (text1_width)) / 2), ((720 - (dialog_height)) / 2) + 70, 20, config.dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "NX Shell v%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO);
	SDL_DrawText(((1280 - (text2_width)) / 2), ((720 - (dialog_height)) / 2) + 100, 20, config.dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "Author: Joel16");
	SDL_DrawText(((1280 - (text3_width)) / 2), ((720 - (dialog_height)) / 2) + 130, 20, config.dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "Graphics: Preetisketch and CyanogenMod/LineageOS contributors");
	SDL_DrawText(((1280 - (text4_width)) / 2), ((720 - (dialog_height)) / 2) + 160, 20, config.dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "Touch screen: StevenMattera");
	SDL_DrawText(((1280 - (text5_width)) / 2), ((720 - (dialog_height)) / 2) + 190, 20, config.dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "E-Book Reader: rock88");

	SDL_DrawRect((1030 - (confirm_width)) - 20, (((720 - (dialog_height)) / 2) + 245) - 20, confirm_width + 40, confirm_height + 40, config.dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
	SDL_DrawText(1030 - (confirm_width), ((720 - (dialog_height)) / 2) + 245, 25, config.dark_theme? TITLE_COLOUR_DARK : TITLE_COLOUR, "OK");
}

void Menu_DisplaySettings(void) {
	int selection = 0, max_items = 2;
	u32 height = 0;
	TouchInfo touchInfo;
	Touch_Init(&touchInfo);

	u32 title_height = 0;
	SDL_GetTextDimensions(30, "Settings", NULL, &title_height);

	const char *main_menu_items[] = {
		"Sorting options",
		"Dark theme",
		"About"
	};

	int toggle_button_width = 0, toggle_button_height = 0; //1180
	SDL_QueryTexture(icon_toggle_on, NULL, NULL, &toggle_button_width, &toggle_button_height);

	displayAbout = false;

	while(appletMainLoop()) {
		SDL_ClearScreen(config.dark_theme? BLACK_BG : WHITE);
		SDL_DrawRect(0, 0, 1280, 40, config.dark_theme? STATUS_BAR_DARK : STATUS_BAR_LIGHT);	// Status bar
		SDL_DrawRect(0, 40, 1280, 100, config.dark_theme? MENU_BAR_DARK : MENU_BAR_LIGHT);	// Menu bar

		StatusBar_DisplayTime();

		SDL_DrawImage(icon_back, 40, 66);
		SDL_DrawText(128, 40 + ((100 - title_height)/2), 30, WHITE, "Settings");

		int printed = 0; // Print counter

		for (int i = 0; i < max_items + 1; i++) {
			if (printed == FILES_PER_PAGE)
				break;

			if (selection < FILES_PER_PAGE || i > (selection - FILES_PER_PAGE)) {
				if (i == selection)
					SDL_DrawRect(0, 140 + (73 * printed), 1280, 73, config.dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);

				if (config.dark_theme)
					SDL_DrawImage(config.dark_theme? icon_toggle_dark_on : icon_toggle_off, 1180 - toggle_button_width, 213 + ((73 - toggle_button_height) / 2));
				else
					SDL_DrawImage(config.dark_theme? icon_toggle_on : icon_toggle_off, 1180 - toggle_button_width, 213 + ((73 - toggle_button_height) / 2));
				
				SDL_GetTextDimensions(25, main_menu_items[i], NULL, &height);
				SDL_DrawText(40, 140 + ((73 - height)/2) + (73 * printed), 25, config.dark_theme? WHITE : BLACK, main_menu_items[i]);

				printed++;
			}
		}

		if (displayAbout)
			Menu_DisplayAboutDialog();

		SDL_Renderdisplay();

		hidScanInput();
		Touch_Process(&touchInfo);
		u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

		if (displayAbout) {
			Menu_ControlAboutDialog(kDown);
			Menu_TouchAboutDialog(touchInfo);
		}
		else {
			if (kDown & KEY_B)
				break;
			
			if ((kDown & KEY_DDOWN) || (kDown & KEY_LSTICK_DOWN) || (kDown & KEY_RSTICK_DOWN))
				selection++;
			else if ((kDown & KEY_DUP) || (kDown & KEY_LSTICK_UP) || (kDown & KEY_RSTICK_UP))
				selection--;

			Utils_SetMax(&selection, 0, max_items);
			Utils_SetMin(&selection, max_items, 0);

			if (kDown & KEY_A) {
				switch (selection) {
					case 0:
						Menu_DisplaySortSettings();
						break;
					case 1:
						config.dark_theme = !config.dark_theme;
						Config_Save(config);
						break;
					case 2:
						displayAbout = true;
						break;
				}
			}

			if (touchInfo.state == TouchStart) {
				int touch_selection = floor(((double) touchInfo.firstTouch.py - 140) / 73);

				if (touch_selection > 0 && touch_selection <= max_items)
					selection = touch_selection;
			}
			else if (touchInfo.state == TouchEnded && touchInfo.tapType != TapNone) {
				if (tapped_inside(touchInfo, 40, 66, 108, 114))
					break;
				else if (touchInfo.firstTouch.py >= 140) {
					int tapped_selection = floor(((double) touchInfo.firstTouch.py - 140) / 73);

					switch (tapped_selection) {
						case 0:
							Menu_DisplaySortSettings();
							break;
						case 1:
							config.dark_theme = !config.dark_theme;
							Config_Save(config);
							break;
						case 2:
							displayAbout = true;
							break;
					}
				}
			}
		}
	}

	MENU_DEFAULT_STATE = MENU_STATE_HOME;
}
