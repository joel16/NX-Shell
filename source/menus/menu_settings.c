#include "common.h"
#include "config.h"
#include "menu_settings.h"
#include "SDL_helper.h"
#include "status_bar.h"
#include "textures.h"

void Menu_DisplaySortSettings(void)
{
	int selection = 0, max_items = 1, height = 0;

	int title_height = 0;
	TTF_SizeText(Roboto_large, "Settings", NULL, &title_height);

	const char *main_menu_items[] =
	{
		"Alphabetical (ascending order)",
		"Alphabetical (descending order)"
	};

	int radio_button_width = 0, radio_button_height = 0; //1180
	SDL_QueryTexture(icon_radio_dark_on, NULL, NULL, &radio_button_width, &radio_button_height);

	while(appletMainLoop())
	{
		SDL_ClearScreen(RENDERER, config_dark_theme? BLACK_BG : WHITE);
		SDL_RenderClear(RENDERER);
		SDL_DrawRect(RENDERER, 0, 0, 1280, 40, config_dark_theme? STATUS_BAR_DARK : STATUS_BAR_LIGHT);	// Status bar
		SDL_DrawRect(RENDERER, 0, 40, 1280, 100, config_dark_theme? MENU_BAR_DARK : MENU_BAR_LIGHT);	// Menu bar

		StatusBar_DisplayTime();

		SDL_DrawText(Roboto_large, 40, 40 + ((100 - title_height)/2), WHITE, "Sorting Options");

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
				SDL_DrawText(Roboto, 40, 140 + ((73 - height)/2) + (73 * printed), config_dark_theme? WHITE : BLACK, main_menu_items[i]);

				printed++;
			}
		}

		config_sort_by == 0? SDL_DrawImage(RENDERER, config_dark_theme? icon_radio_dark_on : icon_radio_on, (1170 - radio_button_width), 152, 50, 50) : 
			SDL_DrawImage(RENDERER, config_dark_theme? icon_radio_dark_off : icon_radio_off, (1170 - radio_button_width), 152, 50, 50);
		
		config_sort_by == 1? SDL_DrawImage(RENDERER, config_dark_theme? icon_radio_dark_on : icon_radio_on, (1170 - radio_button_width), 225, 50, 50) : 
			SDL_DrawImage(RENDERER, config_dark_theme? icon_radio_dark_off : icon_radio_off, (1170 - radio_button_width), 225, 50, 50);

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
				case 0:
					config_sort_by = 0;
					Config_Save(config_dark_theme, config_sort_by);
					break;
				case 1:
					config_sort_by = 1;
					Config_Save(config_dark_theme, config_sort_by);
					break;
			}
		}
	}

}

void Menu_DisplaySettings(void)
{
	int selection = 0, max_items = 2, height = 0;

	int title_height = 0;
	TTF_SizeText(Roboto_large, "Settings", NULL, &title_height);

	const char *main_menu_items[] =
	{
		"General settings",
		"Dark theme",
		"Sorting options"
	};

	int toggle_button_width = 0, toggle_button_height = 0; //1180
	SDL_QueryTexture(icon_toggle_on, NULL, NULL, &toggle_button_width, &toggle_button_height);

	while(appletMainLoop())
	{
		SDL_ClearScreen(RENDERER, config_dark_theme? BLACK_BG : WHITE);
		SDL_RenderClear(RENDERER);
		SDL_DrawRect(RENDERER, 0, 0, 1280, 40, config_dark_theme? STATUS_BAR_DARK : STATUS_BAR_LIGHT);	// Status bar
		SDL_DrawRect(RENDERER, 0, 40, 1280, 100, config_dark_theme? MENU_BAR_DARK : MENU_BAR_LIGHT);	// Menu bar

		StatusBar_DisplayTime();

		SDL_DrawText(Roboto_large, 40, 40 + ((100 - title_height)/2), WHITE, "Settings");

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
					SDL_DrawImage(RENDERER, config_dark_theme? icon_toggle_dark_on : icon_toggle_off, 1180 - toggle_button_width, 213 + ((73 - toggle_button_height) / 2), 65, 65);
				else
					SDL_DrawImage(RENDERER, config_dark_theme? icon_toggle_on : icon_toggle_off, 1180 - toggle_button_width, 213 + ((73 - toggle_button_height) / 2), 65, 65);
				
				TTF_SizeText(Roboto, main_menu_items[i], NULL, &height);
				SDL_DrawText(Roboto, 40, 140 + ((73 - height)/2) + (73 * printed), config_dark_theme? WHITE : BLACK, main_menu_items[i]);

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
					config_dark_theme = !config_dark_theme;
					Config_Save(config_dark_theme, config_sort_by);
					break;
				case 2:
					Menu_DisplaySortSettings();
					break;
			}
		}
	}

	MENU_DEFAULT_STATE = MENU_STATE_MENUBAR;
}