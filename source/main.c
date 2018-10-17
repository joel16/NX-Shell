#include <stdio.h>
#include <switch.h>

#ifdef DEBUG
#include <sys/socket.h>
#endif

#include "common.h"
#include "config.h"
#include "fs.h"
#include "menu_main.h"
#include "SDL_helper.h"
#include "textures.h"

static void Term_Services(void) {
	Textures_Free();

	#ifdef DEBUG
		socketExit();
	#endif

	timeExit();
	SDL_HelperTerm();
	romfsExit();
	psmExit();
	plExit();
}

static void Init_Services(void) {
	plInitialize();
	psmInitialize();
	romfsInit();
	SDL_HelperInit();
	timeInitialize();

	#ifdef DEBUG
		socketInitializeDefault();
		nxlinkStdio();
	#endif

	Textures_Load();

	FS_RecursiveMakeDir("/switch/NX-Shell/");

	if (FS_FileExists("/switch/NX-Shell/lastdir.txt")) {
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
	else {
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

int main(int argc, char **argv) {
	Init_Services();

	if (setjmp(exitJmp)) {
		Term_Services();
		return 0;
	}

	MENU_DEFAULT_STATE = MENU_STATE_HOME;
	Menu_Main();
	Term_Services();
}
