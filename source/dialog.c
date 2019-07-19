#include <stdio.h>

#include "common.h"
#include "config.h"
#include "dirbrowse.h"
#include "SDL_helper.h"
#include "status_bar.h"
#include "textures.h"

static char message_resized[41];
static int width = 0, height = 0;

static void Dialog_DisplayBoxAndMsg(const char *title, const char *msg_1, const char *msg_2, u32 msg_1_width, u32 msg_2_width, bool with_bg) {
    if (with_bg) {
        SDL_ClearScreen(config.dark_theme? BLACK_BG : WHITE);
        SDL_DrawRect(0, 0, 1280, 40, config.dark_theme? STATUS_BAR_DARK : STATUS_BAR_LIGHT);	// Status bar
        SDL_DrawRect(0, 40, 1280, 100, config.dark_theme? MENU_BAR_DARK : MENU_BAR_LIGHT);	// Menu bar
        
        StatusBar_DisplayTime();
        Dirbrowse_DisplayFiles();
    }

    SDL_QueryTexture(config.dark_theme? dialog_dark : dialog, NULL, NULL, &width, &height);
    
    SDL_DrawRect(0, 40, 1280, 680, FC_MakeColor(0, 0, 0, config.dark_theme? 55 : 80));
    SDL_DrawImage(config.dark_theme? dialog_dark : dialog, ((1280 - (width)) / 2), ((720 - (height)) / 2));
    SDL_DrawText(((1280 - (width)) / 2) + 30, ((720 - (height)) / 2) + 30, 25, config.dark_theme? TITLE_COLOUR_DARK : TITLE_COLOUR, title);

    if (msg_1 && msg_2) {
        SDL_DrawText(((1280 - (msg_1_width)) / 2), ((720 - (height)) / 2) + 100, 25, config.dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, msg_1);
        SDL_DrawText(((1280 - (msg_2_width)) / 2), ((720 - (height)) / 2) + 135, 25, config.dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, msg_2);
    }
    else if (msg_1 && !msg_2)
        SDL_DrawText(((1280 - (msg_1_width)) / 2), ((720 - (height)) / 2) + 120, 25, config.dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, msg_1);
}

void Dialog_DisplayMessage(const char *title, const char *msg_1, const char *msg_2, bool with_bg) {
    u32 text_width1 = 0, text_width2 = 0, confirm_width = 0, confirm_height = 0;
    
    if (msg_1)
        SDL_GetTextDimensions(25, msg_1, &text_width1, NULL);
        
    if (msg_2)
        SDL_GetTextDimensions(25, msg_2, &text_width2, NULL);
    
    Dialog_DisplayBoxAndMsg(title, msg_1, msg_2, text_width1, text_width2, with_bg);

    SDL_GetTextDimensions(25, "OK", &confirm_width, &confirm_height);

    SDL_DrawRect((1030 - (confirm_width)) - 20, (((720 - (height)) / 2) + 245) - 20, confirm_width + 40, confirm_height + 40, 
        config.dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
    SDL_DrawText(1030 - (confirm_width), ((720 - (height)) / 2) + 245, 25, config.dark_theme? TITLE_COLOUR_DARK : TITLE_COLOUR, "OK");

    if (with_bg)
        SDL_RenderPresent(SDL_GetRenderer(SDL_GetWindow()));
}

void Dialog_DisplayPrompt(const char *title, const char *msg_1, const char *msg_2, int *selection, bool with_bg) {
    u32 text_width1 = 0, text_width2 = 0, confirm_width = 0, confirm_height = 0, cancel_width = 0, cancel_height = 0;
    
    if (msg_1)
        SDL_GetTextDimensions(25, msg_1, &text_width1, NULL);
        
    if (msg_2)
        SDL_GetTextDimensions(25, msg_2, &text_width2, NULL);
        
    Dialog_DisplayBoxAndMsg(title, msg_1, msg_2, text_width1, text_width2, with_bg);
    
    SDL_GetTextDimensions(25, "YES", &confirm_width, &confirm_height);
    SDL_GetTextDimensions(25, "NO", &cancel_width, &cancel_height);
    
    if (*selection == 0)
        SDL_DrawRect((1030 - (confirm_width)) - 20, (((720 - (height)) / 2) + 245) - 20, confirm_width + 40, confirm_height + 40, 
            config.dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
    else if (*selection == 1)
        SDL_DrawRect((915 - (confirm_width)) - 20, (((720 - (height)) / 2) + 245) - 20, confirm_width + 40, cancel_height + 40, 
            config.dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);

    SDL_DrawText(1030 - (confirm_width), ((720 - (height)) / 2) + 245, 25, config.dark_theme? TITLE_COLOUR_DARK : TITLE_COLOUR, "YES");
    SDL_DrawText(910 - (cancel_width), ((720 - (height)) / 2) + 245, 25, config.dark_theme? TITLE_COLOUR_DARK : TITLE_COLOUR, "NO");
    
    if (with_bg)
        SDL_RenderPresent(SDL_GetRenderer(SDL_GetWindow()));
}

void Dialog_DisplayProgress(const char *title, const char *message, u32 offset, u32 size) {
    snprintf(message_resized, 41, "%.40s", message);
    u32 text_width = 0;
    SDL_GetTextDimensions(25, message_resized, &text_width, NULL);
    
    Dialog_DisplayBoxAndMsg(title, message_resized, NULL, text_width, 0, true);

    SDL_DrawRect(((1280 - (width)) / 2) + 80, ((720 - (height)) / 2) + 178, 720, 12, config.dark_theme? 
        SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
    SDL_DrawRect(((1280 - (width)) / 2) + 80, ((720 - (height)) / 2) + 178, (double)offset / (double)size * 720.0, 12, 
        config.dark_theme? TITLE_COLOUR_DARK : TITLE_COLOUR);
        
    SDL_RenderPresent(SDL_GetRenderer(SDL_GetWindow()));
}
