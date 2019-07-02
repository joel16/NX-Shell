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
	fsdevUnmountDevice("NX-Shell_USER");
	fsdevUnmountDevice("NX-Shell_SYSTEM");
	fsdevUnmountDevice("NX-Shell_SAFE");
	fsdevUnmountDevice("NX-Shell_PRODINFOF");
	
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
	fake_heap_end = (char *) addr + 0x10000000;

	if (R_FAILED(ret = psmInitialize()))
		return ret;

	if (R_FAILED(ret = romfsInit()))
		return ret;

	if (R_FAILED(ret = SDL_HelperInit()))
		return ret;

	Textures_Load();

	BROWSE_STATE = STATE_SD;
	sdmc_fs = *fsdevGetDefaultFileSystem();

	FS_OpenBisFileSystem(&prodinfo_fs, 28, "");
	fsdevMountDevice("NX-Shell_PRODINFOF", prodinfo_fs);

	FS_OpenBisFileSystem(&safe_fs, 29, "");
	fsdevMountDevice("NX-Shell_SAFE", safe_fs);

	FS_OpenBisFileSystem(&system_fs, 31, "");
	fsdevMountDevice("NX-Shell_SYSTEM", system_fs);

	FS_OpenBisFileSystem(&user_fs, 30, "");
	fsdevMountDevice("NX-Shell_USER", user_fs);

	fs = &sdmc_fs;

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
