#include <switch.h>
#include <unistd.h>

#include "common.h"
#include "config.h"
#include "dirbrowse.h"
#include "ftp.h"
#include "SDL_helper.h"
#include "status_bar.h"
#include "textures.h"

void Menu_FTP(void) {
	nifmInitialize();
	ftp_init();

	char hostname[128];
	int pBar = 400, xlim = 950;

	Result ret = gethostname(hostname, sizeof(hostname));

	char *hostname_disp = malloc(138);
	snprintf(hostname_disp, 138, "%s:5000", hostname);

	u32 ip_width = 0, instruc_width = 0, transfer_width = 0;
	SDL_GetTextDimensions(25, hostname_disp, &ip_width, NULL);
	SDL_GetTextDimensions(25, "Press B key to exit FTP mode.", &instruc_width, NULL);

	int dialog_width = 0, dialog_height = 0;
	SDL_QueryTexture(dialog, NULL, NULL, &dialog_width, &dialog_height);

	while(appletMainLoop()) {
		ftp_loop();

		SDL_ClearScreen(config.dark_theme? BLACK_BG : WHITE);
		SDL_DrawRect(0, 0, 1280, 40, config.dark_theme? STATUS_BAR_DARK : STATUS_BAR_LIGHT);	// Status bar
		SDL_DrawRect(0, 40, 1280, 100, config.dark_theme? MENU_BAR_DARK : MENU_BAR_LIGHT);	// Menu bar
		
		StatusBar_DisplayTime();
		Dirbrowse_DisplayFiles();

		SDL_DrawImage(config.dark_theme? dialog_dark : dialog, ((1280 - (dialog_width)) / 2), ((720 - (dialog_height)) / 2));
		SDL_DrawText(((1280 - (dialog_width)) / 2) + 30, ((720 - (dialog_height)) / 2) + 30, 25, config.dark_theme? TITLE_COLOUR_DARK : TITLE_COLOUR, "FTP");

		SDL_DrawText(((1280 - (ip_width)) / 2), ((720 - (dialog_height)) / 2) + 70, 25, config.dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, 
			R_SUCCEEDED(ret)? hostname_disp : NULL);
		SDL_DrawText(((1280 - (instruc_width)) / 2), ((720 - (dialog_height)) / 2) + 120, 25, config.dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "Press B key to exit FTP mode.");

		if (strlen(ftp_file_transfer) != 0) {
			SDL_GetTextDimensions(25, ftp_file_transfer, &transfer_width, NULL);
			SDL_DrawText(((1280 - (transfer_width)) / 2), ((720 - (dialog_height)) / 2) + 170, 25, config.dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, ftp_file_transfer);
		}

		if (isTransfering) {
			SDL_DrawRect(330, ((720 - (dialog_height)) / 2) + 220, 620, 10, config.dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
			SDL_DrawRect(pBar, ((720 - (dialog_height)) / 2) + 220, 120, 10, config.dark_theme? TITLE_COLOUR_DARK : TITLE_COLOUR);

			// Boundary stuff
			SDL_DrawRect(210, ((720 - (dialog_height)) / 2) + 220, 120, 10, config.dark_theme? FC_MakeColor(48, 48, 48, 255) : WHITE);
			SDL_DrawRect(950, ((720 - (dialog_height)) / 2) + 220, 120, 10, config.dark_theme? FC_MakeColor(48, 48, 48, 255) : WHITE); 
			pBar += 3;
			
			if (pBar >= xlim)
				pBar = 400;
		}

		SDL_Renderdisplay();

		hidScanInput();
		u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

		if (kDown & KEY_B)
			break;
	}

	free(hostname_disp);
	ftp_exit();
	nifmExit();
	Dirbrowse_PopulateFiles(true);
	MENU_DEFAULT_STATE = MENU_STATE_HOME;
}
