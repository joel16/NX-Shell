#include <dirent.h>
#include <switch.h>

#include "common.h"
#include "dirbrowse.h"
#include "fs.h"
#include "menu_music.h"
#include "SDL_helper.h"
#include "status_bar.h"
#include "textures.h"
#include "touch_helper.h"
#include "utils.h"

#include "mp3.h"

#define MUSIC_STATUS_BAR_COLOUR SDL_MakeColour(97, 97, 97, 255)
#define MUSIC_STATUS_BG_COLOUR  SDL_MakeColour(43, 53, 61, 255)
#define MUSIC_SEPARATOR_COLOUR  SDL_MakeColour(34, 41, 48, 255)

static char playlist[255][500], title[128];
static int count = 0, selection = 0;
static Mix_Music *audio;

static Result Menu_GetMusicList(void)
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
			u8 name[255] = {'\0'};
			for (u32 i = 0; i < entryCount; i++) 
			{
				int length = strlen(entries[i].name);
				if (strncasecmp(FS_GetFileExt(entries[i].name), "mp3", 3) == 0)
				{
					strcpy(playlist[count], cwd);
					strcpy(playlist[count] + strlen(playlist[count]), entries[i].name);
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

static int Music_GetCurrentIndex(char *path)
{
	for(int i = 0; i < count; ++i)
	{
		if (!strcmp(playlist[i], path))
			return i;
	}
}

static void Music_Play(char *path)
{
	audio = Mix_LoadMUS(path);

	if (audio == NULL)
		return;

	Menu_GetMusicList();

	switch(Mix_GetMusicType(audio))
	{
		case MUS_CMD:
			break;
		case MUS_WAV:
			break;
		case MUS_MID:
			break;
		case MUS_MOD:
		case MUS_OGG:
		case MUS_MP3:
			MP3_Init(path);
			break;
		case MUS_NONE:
			break;
	}

	Result ret = 0;
	if (R_FAILED(ret = Mix_PlayMusic(audio, 1)))
		return;

	selection = Music_GetCurrentIndex(path);

	strncpy(title, strlen(ID3.title) == 0? strupr(Utils_Basename(path)) : strupr(ID3.title), strlen(ID3.title) == 0? strlen(Utils_Basename(path)) + 1 : strlen(ID3.title) + 1);
}

static void Music_HandleNext(bool forward)
{
	if (forward)
		selection++;
	else
		selection--;

	Utils_SetMax(&selection, 0, (count - 1));
	Utils_SetMin(&selection, (count - 1), 0);

	switch(Mix_GetMusicType(audio))
	{
		case MUS_MP3:
			MP3_Exit();
			break;
	}

	Mix_HaltMusic();
	Mix_FreeMusic(audio);

	Music_Play(playlist[selection]);
}

static void Music_HandlePause(bool *status)
{
	if (*status)
	{
		Mix_PauseMusic();
		*status = false;
	}
	else
	{
		Mix_ResumeMusic();
		*status = true;
	}
}

void Menu_PlayMusic(char *path)
{
	Result ret = 0;

	Music_Play(path);

	int title_height = 0;
	TTF_SizeText(Roboto_large, title, NULL, &title_height);

	bool isPlaying = true;

	int btn_width = 0, btn_height = 0;
	SDL_QueryTexture(btn_pause, NULL, NULL, &btn_width, &btn_height);

	TouchInfo touchInfo;
	Touch_Init(&touchInfo);

	while(appletMainLoop())
	{
		SDL_ClearScreen(RENDERER, MUSIC_STATUS_BG_COLOUR);
		SDL_RenderClear(RENDERER);

		SDL_DrawRect(RENDERER, 0, 0, 1280, 40, MUSIC_STATUS_BAR_COLOUR); // Status bar
		SDL_DrawRect(RENDERER, 0, 40, 1280, 100, MUSIC_STATUS_BG_COLOUR); // Menu bar
		SDL_DrawRect(RENDERER, 0, 140, 1280, 1, MUSIC_SEPARATOR_COLOUR); // Separator

		SDL_DrawImage(RENDERER, icon_back, 40, 66, 48, 48);

		SDL_DrawText(Roboto_large, 128, 40 + ((100 - title_height)/2), WHITE, title); // Audio filename

		SDL_DrawRect(RENDERER, 0, 141, 560, 560, SDL_MakeColour(158, 158, 158, 255)); // Draw album art background
		SDL_DrawImage(RENDERER, default_artwork, 0, 141, 560, 560); // Default album art

		SDL_DrawRect(RENDERER, 570, 141, 710, 559, SDL_MakeColour(45, 48, 50, 255)); // Draw info box (outer)
		SDL_DrawRect(RENDERER, 575, 146, 700, 549, SDL_MakeColour(46, 49, 51, 255)); // Draw info box (inner)

		if (strlen(ID3.artist) != 0)
			SDL_DrawText(Roboto_large, 590, 161, WHITE, ID3.artist);
		if (strlen(ID3.album) != 0)
			SDL_DrawText(Roboto_large, 590, 201, WHITE, ID3.album);
		if (strlen(ID3.year) != 0)
			SDL_DrawText(Roboto_large, 590, 241, WHITE, ID3.year);
		if (strlen(ID3.genre) != 0)
			SDL_DrawText(Roboto_large, 590, 281, WHITE, ID3.genre);

		SDL_DrawCircle(RENDERER, 615 + ((710 - btn_width) / 2), 186 + ((559 - btn_height) / 2), 80, SDL_MakeColour(98, 100, 101, 255)); // Render outer circle
		SDL_DrawCircle(RENDERER, (615 + ((710 - btn_width) / 2)), (186 + ((559 - btn_height) / 2)), 60, SDL_MakeColour(46, 49, 51, 255)); // Render inner circle

		if (isPlaying)
			SDL_DrawImage(RENDERER, btn_pause, 570 + ((710 - btn_width) / 2), 141 + ((559 - btn_height) / 2), btn_width, btn_height); // Playing
		else
			SDL_DrawImage(RENDERER, btn_play, 570 + ((710 - btn_width) / 2), 141 + ((559 - btn_height) / 2), btn_width, btn_height); // Paused

		SDL_DrawImage(RENDERER, btn_rewind, (570 + ((710 - btn_width) / 2)) - (btn_width * 2), 141 + ((559 - btn_height) / 2), btn_width, btn_height); // Rewind
		SDL_DrawImage(RENDERER, btn_forward, (570 + ((710 - btn_width) / 2)) + (btn_width * 2), 141 + ((559 - btn_height) / 2), btn_width, btn_height); // Forward

		StatusBar_DisplayTime();

		SDL_RenderPresent(RENDERER);

		hidScanInput();
		Touch_Process(&touchInfo);
		u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

		if ((kDown & KEY_B) || (!Mix_PlayingMusic()))
		{
			wait(1);
			Mix_HaltMusic();
			break;
		}

		if (kDown & KEY_A)
			Music_HandlePause(&isPlaying);

		if ((kDown & KEY_LEFT) || (kDown & KEY_L))
		{
			wait(1);
			Music_HandleNext(false);
		}
		else if ((kDown & KEY_RIGHT) || (kDown & KEY_R))
		{
			wait(1);
			Music_HandleNext(true);
		}

		if (touchInfo.state == TouchEnded && touchInfo.tapType != TapNone)
		{
			if (tapped_inside(touchInfo, 40, 66, 108, 114))
			{
				wait(1);
				Mix_HaltMusic();
				break;
			}

			if (tapped_inside(touchInfo, 570 + ((710 - btn_width) / 2), 141 + ((559 - btn_height) / 2), (570 + ((710 - btn_width) / 2)) + btn_width, (141 + ((559 - btn_height) / 2) + btn_height)))
				Music_HandlePause(&isPlaying);
			else if (tapped_inside(touchInfo, (570 + ((710 - btn_width) / 2)) - (btn_width * 2), 141 + ((559 - btn_height) / 2), (570 + ((710 - btn_width) / 2)) - (btn_width * 2) + btn_width, (141 + ((559 - btn_height) / 2) + btn_height)))
			{
				wait(1);
				Music_HandleNext(false);
			}
			else if (tapped_inside(touchInfo, (570 + ((710 - btn_width) / 2)) + (btn_width * 2), 141 + ((559 - btn_height) / 2), (570 + ((710 - btn_width) / 2)) + (btn_width * 2) + btn_width, (141 + ((559 - btn_height) / 2) + btn_height)))
			{
				wait(1);
				Music_HandleNext(true);
			}
		}
	}

	switch(Mix_GetMusicType(audio))
	{
		case MUS_MP3:
			MP3_Exit();
			break;
	}

	Mix_FreeMusic(audio);

	MENU_DEFAULT_STATE = MENU_STATE_HOME;
}