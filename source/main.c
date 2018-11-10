#include <stdio.h>

#ifdef DEBUG
#include <sys/socket.h>
#endif

#include "common.h"
#include "config.h"
#include "menu_main.h"
#include "SDL_helper.h"
#include "textures.h"
#include "utils.h"

static void Term_Services(void) {
	Textures_Free();

	#ifdef DEBUG
		socketExit();
	#endif

	nsExit();
	usbCommsExit();
	timeExit();
	SDL_HelperTerm();
	romfsExit();
	psmExit();
	plExit();
}

static Result Init_Services(void) {
	Result ret = 0;

	if (R_FAILED(ret = plInitialize()))
		return ret;

	if (R_FAILED(ret = psmInitialize()))
		return ret;

	if (R_FAILED(ret = romfsInit()))
		return ret;

	if (R_FAILED(ret = SDL_HelperInit()))
		return ret;

	if (R_FAILED(ret = timeInitialize()))
		return ret;

	if (R_FAILED(ret = usbCommsInitialize()))
		return ret;

	if (R_FAILED(ret = nsInitialize()))
		return ret;

	#ifdef DEBUG
		socketInitializeDefault();
		nxlinkStdio();
	#endif

	Textures_Load();

	BROWSE_STATE = STATE_SD;
	fs = fsdevGetDefaultFileSystem();
	total_storage = Utils_GetTotalStorage(FsStorageId_SdCard);
	used_storage = Utils_GetUsedStorage(FsStorageId_SdCard);

	Config_Load();
	Config_GetLastDirectory();

	return 0;
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
