#include <switch.h>

#include "common.h"
#include "menu_gallery.h"
#include "SDL_helper.h"

void Gallery_DisplayImage(char *path)
{
	SDL_Texture *image = NULL;
	SDL_LoadImage(RENDERER, &image, path);

	int width = 0, height = 0;
	SDL_QueryTexture(image, NULL, NULL, &width, &height);

	while(appletMainLoop())
	{
		SDL_ClearScreen(RENDERER, SDL_MakeColour(33, 39, 43, 255));
		SDL_RenderClear(RENDERER);
		SDL_DrawImage(RENDERER, image, (1280 - width) / 2, (720 - height) / 2, width, height);
		SDL_RenderPresent(RENDERER);

		hidScanInput();
		u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

		if (kDown & KEY_B)
			break;
	}

	SDL_DestroyTexture(image);
	MENU_DEFAULT_STATE = MENU_STATE_HOME;
}