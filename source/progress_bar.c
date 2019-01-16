#include <switch.h>

#include "config.h"
#include "dirbrowse.h"
#include "progress_bar.h"
#include "SDL_helper.h"
#include "status_bar.h"
#include "textures.h"

void ProgressBar_DisplayProgress(char *msg, char *src, u64 offset, u64 size) {
	SDL_ClearScreen(config.dark_theme? BLACK_BG : WHITE);
	SDL_DrawRect(0, 0, 1280, 40, config.dark_theme? STATUS_BAR_DARK : STATUS_BAR_LIGHT);	// Status bar
	SDL_DrawRect(0, 40, 1280, 100, config.dark_theme? MENU_BAR_DARK : MENU_BAR_LIGHT);	// Menu bar
		
	StatusBar_DisplayTime();
	Dirbrowse_DisplayFiles();
	
	u32 text_width = 0;
	SDL_GetTextDimensions(25, src, &text_width, NULL);

	int width = 0, height = 0;
	SDL_QueryTexture(config.dark_theme? dialog_dark : dialog, NULL, NULL, &width, &height);

	SDL_DrawImage(config.dark_theme? dialog_dark : dialog, ((1280 - (width)) / 2), ((720 - (height)) / 2));

	SDL_DrawText(((1280 - (width)) / 2) + 30, ((720 - (height)) / 2) + 30, 25, config.dark_theme? TITLE_COLOUR_DARK : TITLE_COLOUR, msg);

	SDL_DrawText(((1280 - (text_width)) / 2), ((720 - (height)) / 2) + 111, 25, config.dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, src);

	SDL_DrawRect(((1280 - (width)) / 2) + 80, ((720 - (height)) / 2) + 178, 720, 12, config.dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
	SDL_DrawRect(((1280 - (width)) / 2) + 80, ((720 - (height)) / 2) + 178, (double)offset / (double)size * 720.0, 12, config.dark_theme? TITLE_COLOUR_DARK : TITLE_COLOUR);

	SDL_Renderdisplay();
}
