#include <stdio.h>
#include <switch.h>

#include "common.h"
#include "config.h"
#include "fs.h"
#include "menu_main.h"
#include "textures.h"

static void Term_Services(void)
{
	Textures_Free();
	
	TTF_CloseFont(Roboto_small);
	TTF_CloseFont(Roboto);
	TTF_CloseFont(Roboto_large);
	TTF_Quit();

	IMG_Quit();

	SDL_DestroyRenderer(RENDERER);
	SDL_FreeSurface(WINDOW_SURFACE);
	SDL_DestroyWindow(WINDOW);

	SDL_Quit();
	romfsExit();
}

static void Init_Services(void)
{
	romfsInit();
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_CreateWindowAndRenderer(1280, 720, 0, &WINDOW, &RENDERER);

	WINDOW_SURFACE = SDL_GetWindowSurface(WINDOW);

	SDL_SetRenderDrawBlendMode(RENDERER, SDL_BLENDMODE_BLEND);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

	IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);

	TTF_Init();
	Roboto_large = TTF_OpenFont("romfs:/res/Roboto-Regular.ttf", 30);
	Roboto = TTF_OpenFont("romfs:/res/Roboto-Regular.ttf", 25);
	Roboto_small = TTF_OpenFont("romfs:/res/Roboto-Regular.ttf", 20);
	if (!Roboto_large || !Roboto || !Roboto_small)
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
}

int main(int argc, char **argv)
{
	Init_Services();
	Config_Load();

	if (setjmp(exitJmp)) 
	{
		Term_Services();
		return 0;
	}

	MENU_DEFAULT_STATE = MENU_STATE_HOME;
	Menu_Main();
	Term_Services();
}
