#include "audio.h"
#include "common.h"
#include "dirbrowse.h"
#include "fs.h"
#include "menu_music.h"
#include "SDL_helper.h"
#include "status_bar.h"
#include "textures.h"
#include "touch_helper.h"
#include "utils.h"

#define MUSIC_GENRE_COLOUR      FC_MakeColor(97, 97, 97, 255)
#define MUSIC_STATUS_BG_COLOUR  FC_MakeColor(43, 53, 61, 255)

typedef enum {
	MUSIC_STATE_NONE,   // 0
	MUSIC_STATE_REPEAT, // 1
	MUSIC_STATE_SHUFFLE // 2
} Music_State;

static char playlist[1024][512];
static int count = 0, selection = 0, state = 0;
static u32 title_height = 0, length_time_width = 0;
char *position_time = NULL, *length_time = NULL, *filename = NULL;

static Result Menu_GetMusicList(void) {
	FsDir dir;
	Result ret = 0;
	
	if (R_SUCCEEDED(ret = FS_OpenDirectory(fs, cwd, FsDirOpenMode_ReadDirs | FsDirOpenMode_ReadFiles, &dir))) {
		u64 entryCount = 0;

		if (R_FAILED(ret = FS_GetDirEntryCount(&dir, &entryCount)))
			return ret;
		
		FsDirectoryEntry *entries = (FsDirectoryEntry *)calloc(entryCount + 1, sizeof(FsDirectoryEntry));
		
		if (R_SUCCEEDED(ret = FS_ReadDir(&dir, 0, NULL, entryCount, entries))) {
			qsort(entries, entryCount, sizeof(FsDirectoryEntry), Utils_Alphasort);

			for (u32 i = 0; i < entryCount; i++) {
				const char *ext = FS_GetFileExt(entries[i].name);
				
				if ((!strncasecmp(ext, "flac", 4)) || (!strncasecmp(ext, "it", 2)) || (!strncasecmp(ext, "mod", 3))
					|| (!strncasecmp(ext, "mp3", 3)) || (!strncasecmp(ext, "ogg", 3)) || (!strncasecmp(ext, "opus", 4))
					|| (!strncasecmp(ext, "s3m", 3)) || (!strncasecmp(ext, "wav", 3)) || (!strncasecmp(ext, "xm", 2))) {
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

static void Menu_ConvertSecondsToString(char *string, u64 seconds) {
	int h = 0, m = 0, s = 0;
	h = (seconds / 3600);
	m = (seconds - (3600 * h)) / 60;
	s = (seconds - (3600 * h) - (m * 60));

	if (h > 0)
		snprintf(string, 35, "%02d:%02d:%02d", h, m, s);
	else
		snprintf(string, 35, "%02d:%02d", m, s);
}

static void Music_Play(char *path) {
	Audio_Init(path);

	filename = malloc(128);
	snprintf(filename, 128, Utils_Basename(path));
	position_time = malloc(35);
	length_time = malloc(35);
	length_time_width = 0;

	Menu_ConvertSecondsToString(length_time, Audio_GetLengthSeconds());
	SDL_GetTextDimensions(30, length_time, &length_time_width, NULL);
	SDL_GetTextDimensions(30, strupr(filename), NULL, &title_height);
	selection = Music_GetCurrentIndex(path);
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

	Audio_Stop();

	free(filename);
	free(length_time);
	free(position_time);

	Audio_Term();
	Music_Play(playlist[selection]);
}

void Menu_PlayMusic(char *path) {
	Menu_GetMusicList();
	Music_Play(path);

	int btn_width = 0, btn_height = 0;
	SDL_QueryTexture(btn_pause, NULL, NULL, &btn_width, &btn_height);

	TouchInfo touchInfo;
	Touch_Init(&touchInfo);

	bool locked = false;

	appletSetMediaPlaybackState(true);

	while(appletMainLoop()) {
		SDL_ClearScreen(MUSIC_STATUS_BG_COLOUR);

		SDL_DrawImage(default_artwork_blur, 0, 0);
		SDL_DrawRect(0, 0, 1280, 40, MUSIC_GENRE_COLOUR); // Status bar

		if (locked)
			SDL_DrawImage(icon_lock, 5, 3);

		SDL_DrawImage(icon_back, 40, 66);

		if ((metadata.has_meta) && (metadata.title[0] != '\0') && (metadata.artist[0] != '\0')) {
			SDL_DrawText(128, 20 + ((100 - title_height) / 2), 30, WHITE, strupr(metadata.title));
			SDL_DrawText(128, 60 + ((100 - title_height) / 2), 30, WHITE, strupr(metadata.artist));
		}
		else if ((metadata.has_meta) && (metadata.title[0] != '\0'))
			SDL_DrawText(128, 40 + ((100 - title_height) / 2), 30, WHITE, strupr(metadata.title));
		else
			SDL_DrawText(128, 40 + ((100 - title_height) / 2), 30, WHITE, strupr(filename));

		SDL_DrawRect(0, 141, 560, 560, MUSIC_GENRE_COLOUR); // Draw album art background

		SDL_DrawRect(570, 141, 710, 559, FC_MakeColor(45, 48, 50, 255)); // Draw info box (outer)
		SDL_DrawRect(575, 146, 700, 549, FC_MakeColor(46, 49, 51, 255)); // Draw info box (inner)

		if (metadata.has_meta) {
			if (metadata.album[0] != '\0')
				SDL_DrawTextf(590, 156, 30, WHITE, "Album: %.50s", metadata.album);

			if (metadata.year[0] != '\0')
				SDL_DrawTextf(590, 196, 30, WHITE, "Year: %.50s", metadata.year);
			
			if (metadata.genre[0] != '\0')
				SDL_DrawTextf(590, 236, 30, WHITE, "Genre: %.50s", metadata.genre);
		}

		if ((metadata.has_meta) && (metadata.cover_image))
			SDL_DrawImageScale(metadata.cover_image, 0, 141, 560, 560); // Cover art
		else
			SDL_DrawImage(default_artwork, 0, 141); // Default album art

		Menu_ConvertSecondsToString(position_time, Audio_GetPositionSeconds());
		SDL_DrawText(605, 615, 30, WHITE, position_time);
		SDL_DrawText(1245 - length_time_width, 615, 30, WHITE, length_time);
		SDL_DrawRect(605, 655, 640, 5, FC_MakeColor(97, 97, 97, 150));
		SDL_DrawRect(605, 655, (((double)Audio_GetPosition()/(double)Audio_GetLength()) * 640.0), 5, WHITE);

		SDL_DrawCircle(615 + ((710 - btn_width) / 2), 186 + ((559 - btn_height) / 2), 80, FC_MakeColor(98, 100, 101, 255)); // Render outer circle
		SDL_DrawCircle((615 + ((710 - btn_width) / 2)), (186 + ((559 - btn_height) / 2)), 60, FC_MakeColor(46, 49, 51, 255)); // Render inner circle

		if (!paused)
			SDL_DrawImage(btn_pause, 570 + ((710 - btn_width) / 2), 141 + ((559 - btn_height) / 2)); // Playing
		else
			SDL_DrawImage(btn_play, 570 + ((710 - btn_width) / 2), 141 + ((559 - btn_height) / 2)); // Paused

		SDL_DrawImage(btn_rewind, (560 + ((710 - btn_width) / 2)) - (btn_width * 2), 141 + ((559 - btn_height) / 2));  // Rewind
		SDL_DrawImage(btn_forward, (580 + ((710 - btn_width) / 2)) + (btn_width * 2), 141 + ((559 - btn_height) / 2)); // Forward
		SDL_DrawImage(state == MUSIC_STATE_SHUFFLE? btn_shuffle_overlay : btn_shuffle,
			(590 + ((710 - (btn_width - 10)) / 2)) - ((btn_width - 10) * 2), 141 + ((559 - (btn_height - 10)) / 2) + 90);  // Shuffle
		SDL_DrawImage(state == MUSIC_STATE_REPEAT? btn_repeat_overlay : btn_repeat,
			(550 + ((710 - (btn_width - 10)) / 2)) + ((btn_width - 10) * 2), 141 + ((559 - (btn_height - 10)) / 2) + 90); // Repeat
		StatusBar_DisplayTime();

		SDL_RenderPresent(SDL_GetRenderer(SDL_GetWindow()));

		hidScanInput();
		Touch_Process(&touchInfo);
		u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

		if (!playing) {
			if (state == MUSIC_STATE_NONE) {
				Audio_Stop();
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
			Audio_Stop();
			break;
		}
		
		if (kDown & KEY_A)
			Audio_Pause();

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
				if (count != 0)
					Music_HandleNext(false, MUSIC_STATE_NONE);
			}
			else if ((kDown & KEY_RIGHT) || (kDown & KEY_R)) {
				if (count != 0)
					Music_HandleNext(true, MUSIC_STATE_NONE);
			}
		}

		if (touchInfo.state == TouchMoving && touchInfo.tapType == TapNone && tapped_inside(touchInfo, 605, 635, 1245, 680)) {
			if (!paused)
				Audio_Pause();

			Audio_Seek(touchInfo.prevTouch.px - 605);

			if (paused)
				Audio_Pause();
		}
		else if (touchInfo.state == TouchEnded && touchInfo.tapType != TapNone) {
			if (tapped_inside(touchInfo, 40, 66, 108, 114)) {
				Audio_Stop();
				break;
			}

			if (tapped_inside(touchInfo, 570 + ((710 - btn_width) / 2), 141 + ((559 - btn_height) / 2), (570 + ((710 - btn_width) / 2))
				+ btn_width, (141 + ((559 - btn_height) / 2) + btn_height)))
				Audio_Pause();
			else if (tapped_inside(touchInfo, (590 + ((710 - (btn_width - 10)) / 2)) - ((btn_width - 10) * 2),
				141 + ((559 - (btn_height - 10)) / 2) + 90, ((590 + ((710 - (btn_width - 10)) / 2)) - ((btn_width - 10) * 2))
				+ (btn_width - 10), (141 + ((559 - (btn_height - 10)) / 2) + 90) + (btn_height - 10))) {
				if (state == MUSIC_STATE_SHUFFLE)
					state = MUSIC_STATE_NONE;
				else
					state = MUSIC_STATE_SHUFFLE;
			}
			else if (tapped_inside(touchInfo, (550 + ((710 - (btn_width - 10)) / 2)) + ((btn_width - 10) * 2),
				141 + ((559 - (btn_height - 10)) / 2) + 90, ((550 + ((710 - (btn_width - 10)) / 2)) + ((btn_width - 10) * 2))
				+ (btn_width - 10), (141 + ((559 - (btn_height - 10)) / 2) + 90) + (btn_height - 10))) {
				if (state == MUSIC_STATE_REPEAT)
					state = MUSIC_STATE_NONE;
				else
					state = MUSIC_STATE_REPEAT;
			}
			
			if (!locked) {
				if (tapped_inside(touchInfo, (570 + ((710 - btn_width) / 2)) - (btn_width * 2), 141 + ((559 - btn_height) / 2),
					(570 + ((710 - btn_width) / 2)) - (btn_width * 2) + btn_width, (141 + ((559 - btn_height) / 2) + btn_height))) {
					if (count != 0)
						Music_HandleNext(false, MUSIC_STATE_NONE);
				}
				else if (tapped_inside(touchInfo, (570 + ((710 - btn_width) / 2)) + (btn_width * 2), 141 + ((559 - btn_height) / 2),
					(570 + ((710 - btn_width) / 2)) + (btn_width * 2) + btn_width, (141 + ((559 - btn_height) / 2) + btn_height))) {
					if (count != 0)
						Music_HandleNext(true, MUSIC_STATE_NONE);
				}
			}
		}
	}

	appletSetMediaPlaybackState(false);

	free(filename);
	free(length_time);
	free(position_time);

	Audio_Term();
	memset(playlist, 0, sizeof(playlist));
	count = 0;
	MENU_DEFAULT_STATE = MENU_STATE_HOME;
}
