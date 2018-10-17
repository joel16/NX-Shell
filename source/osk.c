#include <switch.h>

#include "config.h"
#include "osk.h"
#include "SDL_helper.h"
#include "status_bar.h"
#include "touch_helper.h"
#include "textures.h"
#include "utils.h"

#define OSK_BG_COLOUR_LIGHT FC_MakeColor(236, 239, 241, 255)
#define OSK_BG_COLOUR_DARK  FC_MakeColor(39, 50, 56, 255)
#define OSK_SELECTED_COLOUR FC_MakeColor(77, 182, 172, 255)

static int osk_index = 0;

static const char *osk_textdisp[] = {
	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
	"q", "w", "e", "r", "t", "y", "u", "i", "o", "p",
	"a", "s", "d", "f", "g", "h", "j", "k", "l", "z",
	"x", "c", "v", "b", "n", "m", ".", "-", "_", "[_]",
	"!", "@", "#", "$", "%", "^", "&", "*", "(", ")"
};

static const char *osk_textdisp_shift[] = {
	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
	"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P",
	"A", "S", "D", "F", "G", "H", "J", "K", "L", "Z",
	"X", "C", "V", "B", "N", "M", ",", "\"", "'", "[_]",
	"{", "}", "|", "\\", "/", "?", "<", ">", "[", "]"
};

static void OSK_DeleteChar(char *str, int i) {
	int len = strlen(str);

	for (; i < len - 1 ; i++)
		str[i] = str[i+1];

	str[i] = '\0';
}

static void OSK_ResetIndex(void) {
	if (strlen(osk_buffer) != 0)
		osk_index -= 1;
	else 
		osk_index = 0;
}

static void OSK_BlinkText(int fade, int x, int y) {
	SDL_Color FADE_WHITE, FADE_BLACK;

	FADE_WHITE = FC_MakeColor(255, 255, 255, fade);
	FADE_BLACK = FC_MakeColor(0, 0, 0, fade);
	SDL_DrawText(x, y, 30, config.dark_theme? FADE_WHITE : FADE_BLACK, "|");
}

static void OSK_HandleDelete(void) {
	OSK_DeleteChar(osk_buffer, osk_index == 0? osk_index : osk_index - 1);

	if (strlen(osk_buffer) != 0)
		osk_index -= 1;
	else 
		osk_index = 0;
}

static void OSK_HandleAppend(bool shift, bool caps, int x, int y) {
	if (strcasecmp((shift || caps)? osk_textdisp_shift[x + y * 10] : osk_textdisp[x + y * 10], "[_]") == 0)
		Utils_AppendArr(osk_buffer, " ", osk_index);
	else if (strcasecmp((shift || caps)? osk_textdisp_shift[x + y * 10] : osk_textdisp[x + y * 10], "[x]") == 0)
		OSK_HandleDelete();
	else
		Utils_AppendArr(osk_buffer, (shift || caps)? osk_textdisp_shift[x + y * 10] : osk_textdisp[x + y * 10], osk_index);
	
	osk_index += 1;
	shift = false;
}

void OSK_Display(char *title, char *msg) {
	u32 text_width = 0, text_height = 0;
	SDL_GetTextDimensions(50, " Q W E R T Y U I O P ", &text_width, &text_height);

	u32 title_height = 0;
	SDL_GetTextDimensions(30, title, NULL, &title_height);

	u32 cursor_width = 0;
	SDL_GetTextDimensions(30, "|", &cursor_width, NULL);

	char buf2[256];
	u32 buf_width = 0, buf_height = 0, buf_width_curr;

	OSK_ResetIndex();

	if (strlen(msg) != 0) {
		osk_index = strlen(msg);
		strcpy(osk_buffer, msg);
	}

	TouchInfo touchInfo;
	Touch_Init(&touchInfo);

	int osk_pos_x = 0, osk_pos_y = 0, transp = 255;
	bool osk_text_shift = false, osk_text_caps = false;

	while(appletMainLoop()) {	
		SDL_ClearScreen(config.dark_theme? BLACK_BG : WHITE);
		SDL_DrawRect(0, 0, 1280, 40, config.dark_theme? STATUS_BAR_DARK : STATUS_BAR_LIGHT);	// Status bar
		SDL_DrawRect(0, 40, 1280, 100, config.dark_theme? MENU_BAR_DARK : MENU_BAR_LIGHT);	// Menu bar

		SDL_DrawImage(icon_back, 40, 66);

		SDL_DrawText(128, 40 + ((100 - title_height)/2), 30, WHITE, title);

		SDL_DrawRect(0, (660 - (text_height * 5)) - 30, 1280, 720 - ((660 - (text_height * 5)) - 30), config.dark_theme? OSK_BG_COLOUR_DARK : OSK_BG_COLOUR_LIGHT);

		if (strlen(osk_buffer) != 0) {
			SDL_GetTextDimensions(30, osk_buffer, &buf_width, &buf_height);
			snprintf(buf2, osk_index + 1, osk_buffer);

			SDL_GetTextDimensions(30, buf2, &buf_width_curr, NULL);
			SDL_DrawTextf((1280 - buf_width) / 2, 210, 30, config.dark_theme? WHITE : BLACK, "%s", osk_buffer);
		}

		OSK_BlinkText(transp, (((1280 - buf_width) / 2) + (buf_width_curr + cursor_width)) - 11, 210);
		transp -= 14;

		for (int x = 0; x <= MAX_X; x++) {
			for (int y = 0; y <= MAX_Y; y++) {
				if (osk_text_shift || osk_text_caps) {
					if (config.dark_theme)
						SDL_DrawText(((1280 - (text_width * 2)) / 2) + (100 * x), (660 - (text_height * 5)) + (60 * y), 50, (x == osk_pos_x && y == osk_pos_y)? 
							TITLE_COLOUR_DARK : WHITE, osk_textdisp_shift[x + y * 10]);
					else
						SDL_DrawText(((1280 - (text_width * 2)) / 2) + (100 * x), (660 - (text_height * 5)) + (60 * y), 50, (x == osk_pos_x && y == osk_pos_y)? 
							TITLE_COLOUR : BLACK, osk_textdisp_shift[x + y * 10]);
				}
				else {
					if (config.dark_theme)
						SDL_DrawText(((1280 - (text_width * 2)) / 2) + (100 * x), (660 - (text_height * 5)) + (60 * y), 50, (x == osk_pos_x && y == osk_pos_y)?
							TITLE_COLOUR_DARK : WHITE, osk_textdisp[x + y * 10]);
					else
						SDL_DrawText(((1280 - (text_width * 2)) / 2) + (100 * x), (660 - (text_height * 5)) + (60 * y), 50, (x == osk_pos_x && y == osk_pos_y)? 
							TITLE_COLOUR : BLACK, osk_textdisp[x + y * 10]);
				}
			}
		}

		SDL_DrawImage(config.dark_theme? icon_remove_dark : icon_remove, 1190, 480);
		SDL_DrawImage(config.dark_theme? icon_accept_dark : icon_accept, 1190, 600);

		StatusBar_DisplayTime();

		SDL_Renderdisplay();

		hidScanInput();
		Touch_Process(&touchInfo);
		u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

		if (kDown & KEY_LEFT)
			osk_pos_x--;
		else if (kDown & KEY_RIGHT)
			osk_pos_x++;

		if (kDown & KEY_UP)
			osk_pos_y--;
		else if (kDown & KEY_DOWN)
			osk_pos_y++;

		Utils_SetMin(&osk_pos_x, MAX_X, 0);
		Utils_SetMax(&osk_pos_x, 0, MAX_X);

		Utils_SetMin(&osk_pos_y, MAX_Y, 0);
		Utils_SetMax(&osk_pos_y, 0, MAX_Y);

		if (kDown & KEY_L) {
			if (strlen(osk_buffer) != 0) {
				if (osk_index > 0)
					osk_index--;
				else 
					osk_index = 0;
			}
		}
		else if (kDown & KEY_R) {
			if (strlen(osk_buffer) != 0) {
				if (osk_index < strlen(osk_buffer))
					osk_index++;
				else
					osk_index = strlen(osk_buffer);
			}
		}

		if (kDown & KEY_ZL) {
			if (osk_text_caps) {
				osk_text_shift = false;
				osk_text_caps = false;
			}
			else
				osk_text_shift = !osk_text_shift;
		}
		else if (kDown & KEY_ZR) {
			if (osk_text_shift) {
				osk_text_shift = false;
				osk_text_caps = false;
			}
			else
				osk_text_caps = !osk_text_caps;
		}

		Utils_SetMax(&transp, 0, 255);
		Utils_SetMin(&transp, 255, 0);

		if (kDown & KEY_A)
			OSK_HandleAppend(osk_text_shift, osk_text_caps, osk_pos_x, osk_pos_y);
		else if (kDown & KEY_B)
			OSK_HandleDelete();

		if (kDown & KEY_PLUS)
			break;
		else if (kDown & KEY_MINUS) {
			osk_buffer[0] = '\0';
			break;
		}

		if (touchInfo.state == TouchStart) {
			int touch_selection_y = floor(((double) touchInfo.firstTouch.py - (660 - (text_height * 5))) / 60);
			int touch_selection_x = floor(((double) touchInfo.firstTouch.px - ((1280 - (text_width * 2)) / 2)) / 100);

			if (touch_selection_y >= (660 - (text_height * 5)) && touch_selection_y <= ((660 - (text_height * 5)) + (60 * MAX_Y)))
				osk_pos_y = touch_selection_y;
			if (touch_selection_x >= ((1280 - (text_width * 2)) / 2) && touch_selection_x <= (((1280 - (text_width * 2)) / 2) + (100 * MAX_X)))
				osk_pos_x = touch_selection_x;
		}
		if (touchInfo.state == TouchEnded && touchInfo.tapType != TapNone) {
			if (tapped_inside(touchInfo, 40, 66, 108, 114)) {
				osk_buffer[0] = '\0';
				break;
			}
			else if (tapped_inside(touchInfo, 1190, 480, 1250, 540))
				OSK_HandleDelete();
			else if (tapped_inside(touchInfo, 1190, 600, 1250, 660))
				break;
			else if (((touchInfo.firstTouch.px >= ((1280 - (text_width * 2)) / 2)) && (touchInfo.firstTouch.px <= (((1280 - (text_width * 2)) / 2) + (100 * (MAX_X + 1))))) 
				&& ((touchInfo.firstTouch.py >= (660 - (text_height * 5))) && (touchInfo.firstTouch.py <= ((660 - (text_height * 5)) + (60 * (MAX_Y + 1)))))) {
				int touch_selection_y = floor(((double) touchInfo.firstTouch.py - (660 - (text_height * 5))) / 60);
				int touch_selection_x = floor(((double) touchInfo.firstTouch.px - ((1280 - (text_width * 2)) / 2)) / 100);

				if (touch_selection_y >= 0 && touch_selection_y <= MAX_Y)
					osk_pos_y = touch_selection_y;
				if (touch_selection_x >= 0 && touch_selection_x <= MAX_X)
					osk_pos_x = touch_selection_x;

				OSK_HandleAppend(osk_text_shift, osk_text_caps, osk_pos_x, osk_pos_y);
			}
		}
	}
}
