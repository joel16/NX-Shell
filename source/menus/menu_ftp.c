#include <switch.h>
#include <unistd.h>

#include "common.h"
#include "config.h"
#include "dialog.h"
#include "dirbrowse.h"
#include "ftp.h"
#include "SDL_helper.h"
#include "status_bar.h"
#include "textures.h"
#include "touch_helper.h"

void Menu_FTP(void) {
	nifmInitialize(1);
	ftp_init();

	char hostname[128];
	int pBar = 400, xlim = 950;

	Result ret = gethostname(hostname, sizeof(hostname));

	char *hostname_disp = malloc(138);
	snprintf(hostname_disp, 138, "IP: %s:5000", hostname);

	u32 ip_width = 0, instruc_width = 0, transfer_width = 0;
	SDL_GetTextDimensions(25, hostname_disp, &ip_width, NULL);
	SDL_GetTextDimensions(25, "Press B key to exit FTP mode.", &instruc_width, NULL);
	
	int dialog_width = 0, dialog_height = 0;
	u32 confirm_width = 0, confirm_height = 0;
	SDL_GetTextDimensions(25, "OK", &confirm_width, &confirm_height);
	SDL_QueryTexture(dialog, NULL, NULL, &dialog_width, &dialog_height);
	
	TouchInfo touchInfo;
	Touch_Init(&touchInfo);
	
	appletSetMediaPlaybackState(true);

	while(appletMainLoop()) {
		ftp_loop();

		SDL_ClearScreen(config.dark_theme? BLACK_BG : WHITE);
		SDL_DrawRect(0, 0, 1280, 40, config.dark_theme? STATUS_BAR_DARK : STATUS_BAR_LIGHT);	// Status bar
		SDL_DrawRect(0, 40, 1280, 100, config.dark_theme? MENU_BAR_DARK : MENU_BAR_LIGHT);	// Menu bar
		
		StatusBar_DisplayTime();
		Dirbrowse_DisplayFiles();

		Dialog_DisplayMessage("FTP", R_SUCCEEDED(ret)? hostname_disp : NULL, "Press B key or tap \"OK\" to exit FTP mode.", false);
		
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

		SDL_RenderPresent(SDL_GetRenderer(SDL_GetWindow()));

		hidScanInput();
		Touch_Process(&touchInfo);
		u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

		if (kDown & KEY_B)
			break;
		
		if (touchInfo.state == TouchEnded && touchInfo.tapType != TapNone) {
            // Confirm Button
            if (tapped_inside(touchInfo, 1010 - confirm_width, (720 - dialog_height) / 2 + 225, 1050 + confirm_width, (720 - dialog_height) / 2 + 265 + confirm_height))
                break;
        }
	}

	appletSetMediaPlaybackState(false);

	free(hostname_disp);
	ftp_exit();
	nifmExit();
	Dirbrowse_PopulateFiles(true);
	MENU_DEFAULT_STATE = MENU_STATE_HOME;
}
