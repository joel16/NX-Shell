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

#define MUSIC_GENRE_COLOUR      FC_MakeColor(97, 97, 97, 255)
#define MUSIC_STATUS_BG_COLOUR  FC_MakeColor(43, 53, 61, 255)
#define MUSIC_SEPARATOR_COLOUR  FC_MakeColor(34, 41, 48, 255)

typedef enum {
	MUSIC_STATE_NONE,   // 0
	MUSIC_STATE_REPEAT, // 1
	MUSIC_STATE_SHUFFLE // 2
} Music_State;

static char playlist[1024][512], title[128];
static int count = 0, selection = 0, state = 0;
static Mix_Music *audio;

static Result Menu_GetMusicList(void) {
	FsDir dir;
	Result ret = 0;
	
	if (R_SUCCEEDED(ret = FS_OpenDirectory(cwd, FS_DIROPEN_DIRECTORY | FS_DIROPEN_FILE, &dir))) {
		u64 entryCount = 0;

		if (R_FAILED(ret = FS_GetDirEntryCount(&dir, &entryCount)))
			return ret;
		
		FsDirectoryEntry *entries = (FsDirectoryEntry*)calloc(entryCount + 1, sizeof(FsDirectoryEntry));
		
		if (R_SUCCEEDED(ret = FS_ReadDir(&dir, 0, NULL, entryCount, entries))) {
			qsort(entries, entryCount, sizeof(FsDirectoryEntry), Utils_Alphasort);

			for (u32 i = 0; i < entryCount; i++) {
				if ((!strncasecmp(FS_GetFileExt(entries[i].name), "mp3", 3)) || (!strncasecmp(FS_GetFileExt(entries[i].name), "ogg", 3)) 
					|| (!strncasecmp(FS_GetFileExt(entries[i].name), "wav", 3)) || (!strncasecmp(FS_GetFileExt(entries[i].name), "mod", 3))
					|| (!strncasecmp(FS_GetFileExt(entries[i].name), "flac", 4)) || (!strncasecmp(FS_GetFileExt(entries[i].name), "midi", 4))
					|| (!strncasecmp(FS_GetFileExt(entries[i].name), "mid", 3))) {
					strcpy(playlist[count], cwd);
					strcpy(playlist[count] + strlen(playlist[count]), entries[i].name);
					count++;
				}
			}
		}
		else {
			free(entries);
			return ret;
		}

		free(entries);
		fsDirClose(&dir); // Close directory
	}
	else
		return ret;

	return 0;
}

static int Music_GetCurrentIndex(char *path) {
	for(int i = 0; i < count; ++i) {
		if (!strcmp(playlist[i], path))
			return i;
	}

	return 0;
}

static void Music_Play(char *path) {
	audio = Mix_LoadMUS(path);

	if (audio == NULL)
		return;

	if (Mix_GetMusicType(audio) == MUS_MP3)
		MP3_Init(path);

	Result ret = 0;
	if (R_FAILED(ret = Mix_PlayMusic(audio, 1))) {
		return;
	}

	selection = Music_GetCurrentIndex(path);

	strncpy(title, strlen(ID3.title) == 0? strupr(Utils_Basename(path)) : strupr(ID3.title), strlen(ID3.title) == 0? strlen(Utils_Basename(path)) + 1 : strlen(ID3.title) + 1);
}

static void Music_HandleNext(bool forward, int state) {
	if (state == MUSIC_STATE_NONE) {
		if (forward)
			selection++;
		else
			selection--;
	}
	else if (state == MUSIC_STATE_SHUFFLE) {
		int old_selection = selection;
		time_t t;
		srand((unsigned) time(&t));
		selection = rand() % (count - 1);

		if (selection == old_selection)
			selection++;
	}

	Utils_SetMax(&selection, 0, (count - 1));
	Utils_SetMin(&selection, (count - 1), 0);

	if (Mix_GetMusicType(audio) == MUS_MP3)
		MP3_Exit();

	Mix_HaltMusic();
	Mix_FreeMusic(audio);

	Music_Play(playlist[selection]);
}

static void Music_HandlePause(bool *status) {
	if (*status) {
		Mix_PauseMusic();
		*status = false;
	}
	else {
		Mix_ResumeMusic();
		*status = true;
	}
}

void Menu_PlayMusic(char *path) {
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 4096);
	Menu_GetMusicList();
	Music_Play(path);

	u32 title_height = 0;
	SDL_GetTextDimensions(30, title, NULL, &title_height);

	bool isPlaying = true;

	int btn_width = 0, btn_height = 0;
	SDL_QueryTexture(btn_pause, NULL, NULL, &btn_width, &btn_height);

	TouchInfo touchInfo;
	Touch_Init(&touchInfo);

	bool locked = false;

	while(appletMainLoop()) {
		SDL_ClearScreen(MUSIC_STATUS_BG_COLOUR);

		SDL_DrawImage(default_artwork_blur, 0, 0);
		SDL_DrawRect(0, 0, 1280, 40, MUSIC_GENRE_COLOUR); // Status bar
		SDL_DrawRect(0, 140, 1280, 1, MUSIC_SEPARATOR_COLOUR); // Separator

		if (locked)
			SDL_DrawImage(icon_lock, 5, 3);

		SDL_DrawImage(icon_back, 40, 66);

		SDL_DrawText(128, 40 + ((100 - title_height)/2), 30, WHITE, title); // Audio filename

		SDL_DrawRect(0, 141, 560, 560, MUSIC_GENRE_COLOUR); // Draw album art background
		SDL_DrawImage(default_artwork, 0, 141); // Default album art

		SDL_DrawRect(570, 141, 710, 559, FC_MakeColor(45, 48, 50, 255)); // Draw info box (outer)
		SDL_DrawRect(575, 146, 700, 549, FC_MakeColor(46, 49, 51, 255)); // Draw info box (inner)

		if (strlen(ID3.artist) != 0)
			SDL_DrawText(590, 161, 30, WHITE, ID3.artist);
		if (strlen(ID3.album) != 0)
			SDL_DrawText(590, 201, 30, WHITE, ID3.album);
		if (strlen(ID3.year) != 0)
			SDL_DrawText(590, 241, 30, WHITE, ID3.year);
		if (strlen(ID3.genre) != 0)
			SDL_DrawText(590, 281, 30, WHITE, ID3.genre);

		SDL_DrawCircle(615 + ((710 - btn_width) / 2), 186 + ((559 - btn_height) / 2), 80, FC_MakeColor(98, 100, 101, 255)); // Render outer circle
		SDL_DrawCircle((615 + ((710 - btn_width) / 2)), (186 + ((559 - btn_height) / 2)), 60, FC_MakeColor(46, 49, 51, 255)); // Render inner circle

		if (isPlaying)
			SDL_DrawImage(btn_pause, 570 + ((710 - btn_width) / 2), 141 + ((559 - btn_height) / 2)); // Playing
		else
			SDL_DrawImage(btn_play, 570 + ((710 - btn_width) / 2), 141 + ((559 - btn_height) / 2)); // Paused

		SDL_DrawImage(btn_rewind, (560 + ((710 - btn_width) / 2)) - (btn_width * 2), 141 + ((559 - btn_height) / 2));  // Rewind
		SDL_DrawImage(btn_forward, (580 + ((710 - btn_width) / 2)) + (btn_width * 2), 141 + ((559 - btn_height) / 2)); // Forward
		SDL_DrawImageScale(state == MUSIC_STATE_SHUFFLE? btn_shuffle_overlay : btn_shuffle, (590 + ((710 - (btn_width - 10)) / 2)) - ((btn_width - 10) * 2), 141 + ((559 - (btn_height - 10)) / 2) + 90, (btn_width - 10), (btn_height - 10));  // Shuffle
		SDL_DrawImageScale(state == MUSIC_STATE_REPEAT? btn_repeat_overlay : btn_repeat, (550 + ((710 - (btn_width - 10)) / 2)) + ((btn_width - 10) * 2), 141 + ((559 - (btn_height - 10)) / 2) + 90, (btn_width - 10), (btn_height - 10)); // Repeat
		StatusBar_DisplayTime();

		SDL_Renderdisplay();

		hidScanInput();
		Touch_Process(&touchInfo);
		u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

		if (!Mix_PlayingMusic()) {
			wait(1);

			if (state == MUSIC_STATE_NONE) {
				if (count != 0)
					Music_HandleNext(true, MUSIC_STATE_NONE);
				break;
			}
			else if (state == MUSIC_STATE_REPEAT)
				Music_HandleNext(false, MUSIC_STATE_REPEAT);
			else if (state == MUSIC_STATE_SHUFFLE) {
				if (count != 0)
					Music_HandleNext(false, MUSIC_STATE_SHUFFLE);
			}
		}

		if (kDown & KEY_PLUS)
			locked = !locked;

		if (kDown & KEY_B) {
			wait(1);
			Mix_HaltMusic();
			break;
		}
		
		if (kDown & KEY_A)
			Music_HandlePause(&isPlaying);

		if (kDown & KEY_Y) {
			if (state == MUSIC_STATE_REPEAT)
				state = MUSIC_STATE_NONE;
			else
				state = MUSIC_STATE_REPEAT;
		}
		else if (kDown & KEY_X) {
			if (state == MUSIC_STATE_SHUFFLE)
				state = MUSIC_STATE_NONE;
			else
				state = MUSIC_STATE_SHUFFLE;
		}

		if (!locked) {
			if ((kDown & KEY_LEFT) || (kDown & KEY_L)) {
				wait(1);

				if (count != 0)
					Music_HandleNext(false, MUSIC_STATE_NONE);
			}
			else if ((kDown & KEY_RIGHT) || (kDown & KEY_R)) {
				wait(1);

				if (count != 0)
					Music_HandleNext(true, MUSIC_STATE_NONE);
			}
		}

		if (touchInfo.state == TouchEnded && touchInfo.tapType != TapNone) {
			if (tapped_inside(touchInfo, 40, 66, 108, 114)) {
				wait(1);
				Mix_HaltMusic();
				break;
			}

			if (tapped_inside(touchInfo, 570 + ((710 - btn_width) / 2), 141 + ((559 - btn_height) / 2), (570 + ((710 - btn_width) / 2)) + btn_width, (141 + ((559 - btn_height) / 2) + btn_height)))
				Music_HandlePause(&isPlaying);
			else if (tapped_inside(touchInfo, (590 + ((710 - (btn_width - 10)) / 2)) - ((btn_width - 10) * 2), 141 + ((559 - (btn_height - 10)) / 2) + 90, ((590 + ((710 - (btn_width - 10)) / 2)) - ((btn_width - 10) * 2)) + (btn_width - 10), (141 + ((559 - (btn_height - 10)) / 2) + 90) + (btn_height - 10))) {
				if (state == MUSIC_STATE_SHUFFLE)
					state = MUSIC_STATE_NONE;
				else
					state = MUSIC_STATE_SHUFFLE;
			}
			else if (tapped_inside(touchInfo, (550 + ((710 - (btn_width - 10)) / 2)) + ((btn_width - 10) * 2), 141 + ((559 - (btn_height - 10)) / 2) + 90, ((550 + ((710 - (btn_width - 10)) / 2)) + ((btn_width - 10) * 2)) + (btn_width - 10), (141 + ((559 - (btn_height - 10)) / 2) + 90) + (btn_height - 10))) {
				if (state == MUSIC_STATE_REPEAT)
					state = MUSIC_STATE_NONE;
				else
					state = MUSIC_STATE_REPEAT;
			}
			
			if (!locked) {
				if (tapped_inside(touchInfo, (570 + ((710 - btn_width) / 2)) - (btn_width * 2), 141 + ((559 - btn_height) / 2), (570 + ((710 - btn_width) / 2)) - (btn_width * 2) + btn_width, (141 + ((559 - btn_height) / 2) + btn_height))) {
					wait(1);

					if (count != 0)
						Music_HandleNext(false, MUSIC_STATE_NONE);
				}
				else if (tapped_inside(touchInfo, (570 + ((710 - btn_width) / 2)) + (btn_width * 2), 141 + ((559 - btn_height) / 2), (570 + ((710 - btn_width) / 2)) + (btn_width * 2) + btn_width, (141 + ((559 - btn_height) / 2) + btn_height))) {
					wait(1);

					if (count != 0)
						Music_HandleNext(true, MUSIC_STATE_NONE);
				}
			}
		}
	}

	if (Mix_GetMusicType(audio) == MUS_MP3)
		MP3_Exit();

	Mix_FreeMusic(audio);
	memset(playlist, 0, sizeof(playlist[0][0]) * 512 * 512);
	count = 0;
	Mix_CloseAudio();
	MENU_DEFAULT_STATE = MENU_STATE_HOME;
}
