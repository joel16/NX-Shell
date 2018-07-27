#include <dirent.h>

#include <switch.h>

#include "common.h"
#include "fs.h"
#include "menu_gallery.h"
#include "SDL_helper.h"
#include "touch_helper.h"
#include "utils.h"

static char album[512][512];
static int count = 0, selection = 0;
static SDL_Texture *image = NULL;
static int width = 0, height = 0;
static float scale_factor = 0.0f;

static void Gallery_GetImageList(void)
{
	DIR *dir;
	struct dirent *entries;
	dir = opendir(cwd);

	if (dir != NULL)
	{
		while ((entries = readdir (dir)) != NULL) 
		{
			if ((strncasecmp(FS_GetFileExt(entries->d_name), "png", 3) == 0) || (strncasecmp(FS_GetFileExt(entries->d_name), "jpg", 3) == 0) 
				|| (strncasecmp(FS_GetFileExt(entries->d_name), "bmp", 3) == 0) || (strncasecmp(FS_GetFileExt(entries->d_name), "gif", 3) == 0))
			{
				strcpy(album[count], cwd);
				strcpy(album[count] + strlen(album[count]), entries->d_name);
				count++;
			}
		}

		closedir(dir);
	}
}

static int Gallery_GetCurrentIndex(char *path)
{
	for(int i = 0; i < count; ++i)
	{
		if (!strcmp(album[i], path))
			return i;
	}
}

static void Gallery_HandleNext(bool forward)
{
	if (forward)
		selection++;
	else
		selection--;

	Utils_SetMax(&selection, 0, (count - 1));
	Utils_SetMin(&selection, (count - 1), 0);

	SDL_DestroyTexture(image);
	selection = Gallery_GetCurrentIndex(album[selection]);

	SDL_LoadImage(RENDERER, &image, album[selection]);
	SDL_QueryTexture(image, NULL, NULL, &width, &height);
}

void Gallery_DisplayImage(char *path)
{
	Gallery_GetImageList();
	selection = Gallery_GetCurrentIndex(path);
	SDL_LoadImage(RENDERER, &image, path);
	SDL_QueryTexture(image, NULL, NULL, &width, &height);
	
	TouchInfo touchInfo;
	Touch_Init(&touchInfo);

	while(appletMainLoop())
	{
		SDL_ClearScreen(RENDERER, SDL_MakeColour(33, 39, 43, 255));
		SDL_RenderClear(RENDERER);

		if (height <= 720)
			SDL_DrawImageScale(RENDERER, image, (1280 - width) / 2, (720 - height) / 2, width, height);
		else if (height > 720)
		{
			scale_factor = (720.0f / (float)height);
			width = width * scale_factor;
			height = height * scale_factor;
			SDL_DrawImageScale(RENDERER, image, (float)((1280.0f - width) / 2.0f), (float)((720.0f - height) / 2.0f), 
				(float)width, (float)height);
		}

		hidScanInput();
		Touch_Process(&touchInfo);
		u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

		if ((kDown & KEY_LEFT) || (kDown & KEY_L))
		{
			wait(1);
			Gallery_HandleNext(false);
		}
		else if ((kDown & KEY_RIGHT) || (kDown & KEY_R))
		{
			wait(1);
			Gallery_HandleNext(true);
		}
		
		if (touchInfo.state == TouchEnded && touchInfo.tapType != TapNone)
		{
			if (tapped_inside(touchInfo, 0, 0, 120, 720))
			{
				wait(1);
				Gallery_HandleNext(false);
			}
			else if (tapped_inside(touchInfo, 1160, 0, 1280, 720))
			{
				wait(1);
				Gallery_HandleNext(true);
			}
		}

		SDL_RenderPresent(RENDERER);
		
		if (kDown & KEY_B)
			break;
	}

	SDL_DestroyTexture(image);
	memset(album, 0, sizeof(album[0][0]) * 512 * 512);
	count = 0;
	MENU_DEFAULT_STATE = MENU_STATE_HOME;
}