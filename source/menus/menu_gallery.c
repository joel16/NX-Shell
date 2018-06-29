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

static Result Gallery_GetImageList(void)
{
	FsDir dir;
	Result ret = 0;
	
	if (R_SUCCEEDED(ret = fsFsOpenDirectory(&fs, cwd, FS_DIROPEN_DIRECTORY | FS_DIROPEN_FILE, &dir)))
	{
		u64 entryCount = 0;
		if (R_FAILED(ret = fsDirGetEntryCount(&dir, &entryCount)))
			return ret;
		
		FsDirectoryEntry *entries = (FsDirectoryEntry*)calloc(entryCount + 1, sizeof(FsDirectoryEntry));
		
		if (R_SUCCEEDED(ret = fsDirRead(&dir, 0, NULL, entryCount, entries)))
		{
			qsort(entries, entryCount, sizeof(FsDirectoryEntry), Utils_Alphasort);

			for (u32 i = 0; i < entryCount; i++) 
			{
				int length = strlen(entries[i].name);
				if ((strncasecmp(FS_GetFileExt(entries[i].name), "png", 3) == 0) || (strncasecmp(FS_GetFileExt(entries[i].name), "jpg", 3) == 0) || 
					(strncasecmp(FS_GetFileExt(entries[i].name), "bmp", 3) == 0) || (strncasecmp(FS_GetFileExt(entries[i].name), "gif", 3) == 0))
				{
					strcpy(album[count], cwd);
					strcpy(album[count] + strlen(album[count]), entries[i].name);
					count++;
				}
			}
		}
		else
		{
			free(entries);
			return ret;
		}
		
		free(entries);
		fsDirClose(&dir); // Close directory
	}
	else
		return ret;
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

	Gallery_GetImageList();
	selection = Gallery_GetCurrentIndex(album[selection]);

	SDL_LoadImage(RENDERER, &image, album[selection]);
	SDL_QueryTexture(image, NULL, NULL, &width, &height);
}

void Gallery_DisplayImage(char *path)
{
	SDL_LoadImage(RENDERER, &image, path);

	SDL_QueryTexture(image, NULL, NULL, &width, &height);

	Gallery_GetImageList();
	selection = Gallery_GetCurrentIndex(path);
	
	TouchInfo touchInfo;
	Touch_Init(&touchInfo);

	while(appletMainLoop())
	{
		SDL_ClearScreen(RENDERER, SDL_MakeColour(33, 39, 43, 255));
		SDL_RenderClear(RENDERER);
		SDL_DrawImage(RENDERER, image, (1280 - width) / 2, (720 - height) / 2, width, height);

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