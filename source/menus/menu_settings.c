#include "common.h"
#include "config.h"
#include "menu_settings.h"
#include "SDL_helper.h"
#include "status_bar.h"
#include "textures.h"

void Menu_DisplaySettings(void)
{
	int selection = 0, max_items = 2, height = 0;

	const char *main_menu_items[] =
	{
		"General settings",
		"Dark theme",
		"About"
	};

	int radio_button_width = 0, radio_button_height = 0; //1180
	SDL_QueryTexture(icon_radio_on, NULL, NULL, &radio_button_width, &radio_button_height);

	while(appletMainLoop())
	{
		SDL_ClearScreen(RENDERER, dark_theme? BLACK_BG : WHITE);
		SDL_RenderClear(RENDERER);
		SDL_DrawRect(RENDERER, 0, 0, 1280, 40, dark_theme? STATUS_BAR_DARK : STATUS_BAR_LIGHT);	// Status bar
		SDL_DrawRect(RENDERER, 0, 40, 1280, 100, dark_theme? MENU_BAR_DARK : MENU_BAR_LIGHT);	// Menu bar

		StatusBar_DisplayTime();

		int printed = 0; // Print counter

		for (int i = 0; i < max_items + 1; i++)
		{
			if (printed == FILES_PER_PAGE)
				break;

			if (selection < FILES_PER_PAGE || i > (selection - FILES_PER_PAGE))
			{
				if (i == selection)
					SDL_DrawRect(RENDERER, 0, 140 + (73 * printed), 1280, 73, dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);

				if (dark_theme)
					SDL_DrawImage(RENDERER, dark_theme? icon_radio_dark_on : icon_radio_dark_off, 1180 - radio_button_width, 213 + ((73 - radio_button_height) / 2), 50, 50);
				else
					SDL_DrawImage(RENDERER, dark_theme? icon_radio_on : icon_radio_off, 1180 - radio_button_width, 213 + ((73 - radio_button_height) / 2), 50, 50);
				
				TTF_SizeText(Roboto, main_menu_items[i], NULL, &height);
				SDL_DrawText(Roboto, 40, 140 + ((73 - height)/2) + (73 * printed), dark_theme? WHITE : BLACK, main_menu_items[i]);

				printed++;
			}
		}

		SDL_RenderPresent(RENDERER);

		hidScanInput();
		u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

		if (kDown & KEY_B)
			break;

		if (kDown & KEY_DDOWN)
		{
			if (selection < (max_items))
				selection++;
			else 
				selection = 0;
		}
		else if (kDown & KEY_DUP)
		{
			if (selection > 0)
				selection--;
			else 
				selection = (max_items);
		}

		if (kDown & KEY_A)
		{
			switch (selection)
			{
				case 1:
					dark_theme = !dark_theme;
					Config_Save(dark_theme);
			}
		}
	}

	MENU_DEFAULT_STATE = MENU_STATE_MENUBAR;
}