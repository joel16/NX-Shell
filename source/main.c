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

	fsMountSdcard(&fs);

	Config_Load();
	Config_GetLastDirectory();
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
