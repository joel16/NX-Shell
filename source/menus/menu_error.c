#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "dialog.h"
#include "SDL_helper.h"
#include "textures.h"
#include "touch_helper.h"

void Menu_DisplayError(const char *msg, int ret) {
    char *result = malloc(64);
    if (ret != 0)
        snprintf(result, 64, "Ret: %08X\n", ret);
        
    TouchInfo touchInfo;
    Touch_Init(&touchInfo);
    
    int dialog_width = 0, dialog_height = 0;
    u32 confirm_width = 0, confirm_height = 0;
    SDL_GetTextDimensions(25, "OK", &confirm_width, &confirm_height);
    SDL_QueryTexture(dialog, NULL, NULL, &dialog_width, &dialog_height);
    
    while(appletMainLoop()) {
        Dialog_DisplayMessage("Error", msg, result, true);
        
        hidScanInput();
        Touch_Process(&touchInfo);
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if ((kDown & KEY_B) || (kDown & KEY_A))
            break;

        if (touchInfo.state == TouchEnded && touchInfo.tapType != TapNone) {
            // Touched outside
            if (tapped_outside(touchInfo, (1280 - dialog_width) / 2, (720 - dialog_height) / 2, (1280 + dialog_width) / 2, (720 + dialog_height) / 2))
                break;
            // Confirm Button
            if (tapped_inside(touchInfo, 1010 - confirm_width, (720 - dialog_height) / 2 + 225, 1050 + confirm_width, (720 - dialog_height) / 2 + 265 + confirm_height))
                break;
        }
	}

	free(result);
}
