#include <stdio.h>
#include <switch.h>

#ifdef DEBUG
#include <sys/socket.h>
#endif

#include "common.h"
#include "config.h"
#include "fs.h"
#include "menu_main.h"
#include "textures.h"

static void Term_Services(void)
{
	Textures_Free();
	
	TTF_CloseFont(Roboto_OSK);
	TTF_CloseFont(Roboto_small);
	TTF_CloseFont(Roboto);
	TTF_CloseFont(Roboto_large);
	TTF_Quit();

	Mix_CloseAudio();
	Mix_Quit();

	IMG_Quit();

	SDL_DestroyRenderer(RENDERER);
	SDL_FreeSurface(WINDOW_SURFACE);
	SDL_DestroyWindow(WINDOW);

	#ifdef DEBUG
	socketExit();
	#endif

	timeExit();
	SDL_Quit();
	romfsExit();
}

static void Init_Services(void)
{
	romfsInit();
	SDL_Init(SDL_INIT_EVERYTHING);
	timeInitialize();

	#ifdef DEBUG
	socketInitializeDefault();
	nxlinkStdio();
	#endif

	SDL_CreateWindowAndRenderer(1280, 720, 0, &WINDOW, &RENDERER);

	WINDOW_SURFACE = SDL_GetWindowSurface(WINDOW);

	SDL_SetRenderDrawBlendMode(RENDERER, SDL_BLENDMODE_BLEND);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

	IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);

	Mix_Init(MIX_INIT_FLAC | MIX_INIT_MOD | MIX_INIT_MP3 | MIX_INIT_OGG | MIX_INIT_MID);
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 4096);

	TTF_Init();
	Roboto_large = TTF_OpenFont("romfs:/res/Roboto-Regular.ttf", 30);
	Roboto = TTF_OpenFont("romfs:/res/Roboto-Regular.ttf", 25);
	Roboto_small = TTF_OpenFont("romfs:/res/Roboto-Regular.ttf", 20);
	Roboto_OSK = TTF_OpenFont("romfs:/res/Roboto-Regular.ttf", 50);
	if (!Roboto_large || !Roboto || !Roboto_small || !Roboto_OSK)
		Term_Services();

	Textures_Load();

	FS_RecursiveMakeDir("/switch/NX-Shell/");

	if (FS_FileExists("/switch/NX-Shell/lastdir.txt"))
	{
		char *buf = (char *)malloc(256);
		
		FILE *read = fopen("/switch/NX-Shell/lastdir.txt", "r");
		fscanf(read, "%s", buf);
		fclose(read);
		
		if (FS_DirExists(buf)) // Incase a directory previously visited had been deleted, set start path to sdmc:/ to avoid errors.
			strcpy(cwd, buf);
		else 
			strcpy(cwd, START_PATH);

		free(buf);
	}
	else
	{
		char *buf = (char *)malloc(256);
		strcpy(buf, START_PATH);
			
		FILE *write = fopen("/switch/NX-Shell/lastdir.txt", "w");
		fprintf(write, "%s", buf);
		fclose(write);
		
		strcpy(cwd, buf); // Set Start Path to "sdmc:/" if lastDir.txt hasn't been created.

		free(buf);
	}

	Config_Load();
}

int main(int argc, char **argv)
{
	Init_Services();

	if (setjmp(exitJmp)) 
	{
		Term_Services();
		return 0;
	}

	MENU_DEFAULT_STATE = MENU_STATE_HOME;
	Menu_Main();
	Term_Services();
}
