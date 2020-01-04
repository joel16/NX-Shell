#include <stdio.h>

#ifdef DEBUG
#include <sys/socket.h>
#endif

#include "common.h"
#include "config.h"
#include "fs.h"
#include "menu_main.h"
#include "SDL_helper.h"
#include "textures.h"
#include "utils.h"

static void *addr;

static void Term_Services(void) {
	if (("DEV_USER"))
		fsdevUnmountDevice("DEV_USER");

	if (fsdevGetDeviceFileSystem("DEV_SYSTEM"))
		fsdevUnmountDevice("DEV_SYSTEM");

	if (fsdevGetDeviceFileSystem("DEV_SAFE"))
		fsdevUnmountDevice("DEV_SAFE");

	if (fsdevGetDeviceFileSystem("DEV_PRODINFOF"))
		fsdevUnmountDevice("DEV_PRODINFOF");
	
	Textures_Free();

	SDL_HelperTerm();
	romfsExit();
	psmExit();
	
	svcSetHeapSize(&addr, ((u8 *)envGetHeapOverrideAddr() + envGetHeapOverrideSize()) - (u8 *)addr);
}

static Result Init_Services(void) {
	Result ret = 0;
	
	if (R_FAILED(ret = svcSetHeapSize(&addr, 0x10000000)))
		return ret;

	extern char *fake_heap_end;
	fake_heap_end = (char *)addr + 0x10000000;

	if (R_FAILED(ret = psmInitialize()))
		return ret;

	if (R_FAILED(ret = romfsInit()))
		return ret;

	if (R_FAILED(ret = SDL_HelperInit()))
		return ret;

	Textures_Load();

	BROWSE_STATE = STATE_SD;
	devices[0] = *fsdevGetDeviceFileSystem("sdmc");
	fs = &devices[0];

	total_storage = Utils_GetTotalStorage(fs);
	used_storage = Utils_GetUsedStorage(fs);

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
