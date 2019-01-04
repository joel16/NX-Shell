#include "CEV_gif.h"
#include "common.h"
#include "fs.h"
#include "menu_gallery.h"
#include "SDL_helper.h"
#include "touch_helper.h"
#include "utils.h"

static char album[1024][512];
static int count = 0, selection = 0;
static SDL_Texture *image = NULL;
static int width = 0, height = 0;
static float scale_factor = 0.0f;
static int pos_x = 0, pos_y = 0;

static Result Gallery_GetImageList(void) {
	FsDir dir;
	Result ret = 0;
	
	if (R_SUCCEEDED(ret = FS_OpenDirectory(fs, cwd, FS_DIROPEN_DIRECTORY | FS_DIROPEN_FILE, &dir))) {
		u64 entryCount = 0;

		if (R_FAILED(ret = FS_GetDirEntryCount(&dir, &entryCount)))
			return ret;
		
		FsDirectoryEntry *entries = (FsDirectoryEntry*)calloc(entryCount + 1, sizeof(FsDirectoryEntry));
		
		if (R_SUCCEEDED(ret = FS_ReadDir(&dir, 0, NULL, entryCount, entries))) {
			qsort(entries, entryCount, sizeof(FsDirectoryEntry), Utils_Alphasort);

			for (u32 i = 0; i < entryCount; i++) {
				if ((!strncasecmp(FS_GetFileExt(entries[i].name), "png", 3)) || (!strncasecmp(FS_GetFileExt(entries[i].name), "jpg", 3)) || 
					(!strncasecmp(FS_GetFileExt(entries[i].name), "bmp", 3)) || (!strncasecmp(FS_GetFileExt(entries[i].name), "gif", 3))) {
					strcpy(album[count], cwd);
					strcpy(album[count] + strlen(album[count]), entries[i].name);
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

static int Gallery_GetCurrentIndex(char *path) {
	for(int i = 0; i < count; ++i) {
		if (!strcmp(album[i], path))
			return i;
	}

	return 0;
}

static void Gallery_HandleNext(bool forward) {
	pos_x = 0;
	pos_y = 0;

	if (forward)
		selection++;
	else
		selection--;

	Utils_SetMax(&selection, 0, (count - 1));
	Utils_SetMin(&selection, (count - 1), 0);

	SDL_DestroyTexture(image);
	selection = Gallery_GetCurrentIndex(album[selection]);

	SDL_LoadImage(&image, album[selection]);
	SDL_QueryTexture(image, NULL, NULL, &width, &height);
}

static void Gallery_DrawImage(int x, int y, int w, int h, float zoom_factor, SDL_Rect *clip, double angle, SDL_Point *center, SDL_RendererFlip flip) {
	SDL_Rect position = { x - (pos_x * zoom_factor - pos_x) / 2, y - (pos_y * zoom_factor - pos_y) / 2, w * zoom_factor, h * zoom_factor};

	if (clip != NULL) {
		position.w = clip->w;
		position.h = clip->h;
	}

	SDL_RenderCopyEx(SDL_GetMainRenderer(), image, clip, &position, angle, center, flip);
}

void Gallery_DisplayGif(char *path) {
	SDL_RenderClear(SDL_GetMainRenderer());
	CEV_GifAnim *animation = CEV_gifAnimLoad(path, SDL_GetMainRenderer());
	SDL_Texture *texture = CEV_gifTexture(animation);
	CEV_gifLoopMode(animation, GIF_REPEAT_FOR);

	while(appletMainLoop()) {
		hidScanInput();
		u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

		if (CEV_gifAnimAuto(animation)) {
			SDL_RenderClear(SDL_GetMainRenderer());
			SDL_RenderCopy(SDL_GetMainRenderer(), texture, NULL, NULL);
			SDL_RenderPresent(SDL_GetMainRenderer());
		}

		if (kDown & KEY_B)
			break;
	}

	CEV_gifAnimFree(animation);
}

void Gallery_DisplayImage(char *path) {
	Gallery_GetImageList();
	selection = Gallery_GetCurrentIndex(path);
	SDL_LoadImage(&image, path);
	SDL_QueryTexture(image, NULL, NULL, &width, &height);
	
	TouchInfo touchInfo;
	Touch_Init(&touchInfo);

	u64 current_time = SDL_GetPerformanceCounter(), last_time = 0;
	float zoom_factor = 1.0f;
	double degrees = 0;
	SDL_RendererFlip flip_type = SDL_FLIP_NONE;

	pos_x = 0;
	pos_y = 0;

	while(appletMainLoop()) {
		SDL_ClearScreen(FC_MakeColor(33, 39, 43, 255));

		last_time = current_time;
    	current_time = SDL_GetPerformanceCounter();
		double delta_time = (double)((current_time - last_time) * 1000 / SDL_GetPerformanceFrequency());

		if (height <= 720)
			Gallery_DrawImage((1280 - (width * zoom_factor)) / 2, (720 - (height * zoom_factor)) / 2, 
			width, height, zoom_factor, NULL, degrees, NULL, flip_type);
		else if (height > 720) {
			scale_factor = (720.0f / (float)height);
			width = width * scale_factor;
			height = height * scale_factor;
			Gallery_DrawImage((float)((1280.0f - (width * zoom_factor)) / 2.0f), 
			(float)((720.0f - (height * zoom_factor)) / 2.0f), (float)width, (float)height, zoom_factor, NULL, degrees, NULL, flip_type);
		}

		//SDL_DrawTextf(0, 0, 30, WHITE, "x: %d, y: %d, zoom: %lf, degrees: %lf", pos_x, pos_y, zoom_factor, degrees);

		hidScanInput();
		Touch_Process(&touchInfo);
		u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
		u64 kHeld = hidKeysHeld(CONTROLLER_P1_AUTO);

		if ((kDown & KEY_DLEFT) || (kDown & KEY_L)) {
			degrees = 0;
			flip_type = SDL_FLIP_NONE;
			Gallery_HandleNext(false);
		}
		else if ((kDown & KEY_DRIGHT) || (kDown & KEY_R)) {
			degrees = 0;
			flip_type = SDL_FLIP_NONE;
			Gallery_HandleNext(true);
		}

		if ((kHeld & KEY_DUP) || (kHeld & KEY_RSTICK_UP)) {
			zoom_factor += 0.5f * (delta_time * 0.001);

			if (zoom_factor > 2.0f)
				zoom_factor = 2.0f;
		}
		else if ((kHeld & KEY_DDOWN) || (kHeld & KEY_RSTICK_DOWN)) {
			zoom_factor -= 0.5f * (delta_time * 0.001);

			if (zoom_factor < 0.5f)
				zoom_factor = 0.5f;

			if (zoom_factor <= 1.0f) {
				pos_x = 0;
				pos_y = 0;
			}
		}

		if ((height * zoom_factor > 720) || (width * zoom_factor > 1280)) {
			double velocity = 2 / zoom_factor;
			if (kHeld & KEY_LSTICK_UP)
				pos_y -= ((velocity * zoom_factor) * delta_time);
			else if (kHeld & KEY_LSTICK_DOWN)
				pos_y += ((velocity * zoom_factor) * delta_time);
			else if (kHeld & KEY_LSTICK_LEFT)
				pos_x -= ((velocity * zoom_factor) * delta_time);
			else if (kHeld & KEY_LSTICK_RIGHT)
				pos_x += ((velocity * zoom_factor) * delta_time);
		}

		if ((degrees == 0) || (degrees == 180)) {
			Utils_SetMax(&pos_x, width, width);
			Utils_SetMin(&pos_x, -width, -width);
			Utils_SetMax(&pos_y, height, height);
			Utils_SetMin(&pos_y, -height, -height);
		}
		else {
			Utils_SetMax(&pos_x, height, height);
			Utils_SetMin(&pos_x, -height, -height);
			Utils_SetMax(&pos_y, width, width);
			Utils_SetMin(&pos_y, -width, -width);
		}

		if (kDown & KEY_Y) {
			if (flip_type == SDL_FLIP_HORIZONTAL)
				flip_type = SDL_FLIP_NONE;
			else
				flip_type = SDL_FLIP_HORIZONTAL;
		}
		else if (kDown & KEY_X) {
			if (flip_type == SDL_FLIP_VERTICAL)
				flip_type = SDL_FLIP_NONE;
			else
				flip_type = SDL_FLIP_VERTICAL;
		}

		if (kDown & KEY_ZL) {
			degrees -= 90;

			if (degrees < 0)
				degrees = 270;
		}
		else if (kDown & KEY_ZR) {
			degrees += 90;

			if (degrees > 270)
				degrees = 0;
		}
		
		if (touchInfo.state == TouchEnded && touchInfo.tapType != TapNone) {
			if (tapped_inside(touchInfo, 0, 0, 120, 720)) {
				degrees = 0;
				flip_type = SDL_FLIP_NONE;
				Gallery_HandleNext(false);
			}
			else if (tapped_inside(touchInfo, 1160, 0, 1280, 720)) {
				degrees = 0;
				flip_type = SDL_FLIP_NONE;
				Gallery_HandleNext(true);
			}
		}

		SDL_Renderdisplay();
		
		if (kDown & KEY_B)
			break;

		if (kDown & KEY_PLUS)
			longjmp(exitJmp, 1);
	}

	SDL_DestroyTexture(image);
	memset(album, 0, sizeof(album[0][0]) * 512 * 512);
	count = 0;
	MENU_DEFAULT_STATE = MENU_STATE_HOME;
}
