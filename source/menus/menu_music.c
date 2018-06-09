#include <switch.h>

#include "common.h"
#include "dirbrowse.h"
#include "menu_music.h"
#include "SDL_helper.h"
#include "status_bar.h"
#include "textures.h"
#include "utils.h"

#define MUSIC_STATUS_BAR_COLOUR SDL_MakeColour(97, 97, 97)
#define MUSIC_STATUS_BG_COLOUR  SDL_MakeColour(43, 53, 61)
#define MUSIC_SEPARATOR_COLOUR  SDL_MakeColour(34, 41, 48)

void Menu_PlayMusic(char *path)
{
	Mix_OpenAudio(22050, AUDIO_S16SYS, 2, 640);
	Mix_Music *audio = Mix_LoadMUS(path);
	Mix_PlayMusic(audio, 1);

	int title_height = 0;
	TTF_SizeText(Roboto_large, Utils_Basename(path), NULL, &title_height);

	bool isPlaying = true;

	int btn_width = 0, btn_height = 0;
	SDL_QueryTexture(btn_pause, NULL, NULL, &btn_width, &btn_height);

	while(appletMainLoop())
	{
		SDL_ClearScreen(RENDERER, MUSIC_STATUS_BG_COLOUR);
		SDL_RenderClear(RENDERER);

		SDL_DrawRect(RENDERER, 0, 0, 1280, 40, MUSIC_STATUS_BAR_COLOUR); // Status bar
		SDL_DrawRect(RENDERER, 0, 40, 1280, 100, MUSIC_STATUS_BG_COLOUR); // Menu bar
		SDL_DrawRect(RENDERER, 0, 140, 1280, 1, MUSIC_SEPARATOR_COLOUR); // Separator

		SDL_DrawText(Roboto_large, 40, 40 + ((100 - title_height)/2), WHITE, Utils_Basename(path)); // Audio filename

		SDL_DrawRect(RENDERER, 0, 141, 560, 560, SDL_MakeColour(158, 158, 158)); // Draw album art background
		SDL_DrawImage(RENDERER, default_artwork, 0, 141, 560, 560); // Default album art

		SDL_DrawRect(RENDERER, 570, 141, 710, 559, SDL_MakeColour(45, 48, 50)); // Draw info box (outer)
		SDL_DrawRect(RENDERER, 575, 146, 700, 549, SDL_MakeColour(46, 49, 51)); // Draw info box (inner)

		SDL_DrawCircle(RENDERER, 615 + ((710 - btn_width) / 2), 186 + ((559 - btn_height) / 2), 80, SDL_MakeColour(98, 100, 101)); // Render outer circle
		SDL_DrawCircle(RENDERER, (615 + ((710 - btn_width) / 2)), (186 + ((559 - btn_height) / 2)), 60, SDL_MakeColour(46, 49, 51)); // Render inner circle

		if (isPlaying)
			SDL_DrawImage(RENDERER, btn_pause, 570 + ((710 - btn_width) / 2), 141 + ((559 - btn_height) / 2), btn_width, btn_height); // Playing
		else
			SDL_DrawImage(RENDERER, btn_play, 570 + ((710 - btn_width) / 2), 141 + ((559 - btn_height) / 2), btn_width, btn_height); // Paused

		StatusBar_DisplayTime();

		SDL_RenderPresent(RENDERER);

		hidScanInput();
		u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

		if ((kDown & KEY_B) || (!Mix_PlayingMusic()))
			break;

		if (kDown & KEY_A)
		{
			if (isPlaying)
			{
				Mix_PauseMusic();
				isPlaying = false;
			}
			else
			{
				Mix_ResumeMusic();
				isPlaying = true;
			}
		}
	}

	Mix_FreeMusic(audio);
	Mix_CloseAudio();
	MENU_DEFAULT_STATE = MENU_STATE_HOME;
}