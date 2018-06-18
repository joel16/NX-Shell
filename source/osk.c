#include <switch.h>

#include "common.h"
#include "config.h"
#include "osk.h"
#include "SDL_helper.h"
#include "status_bar.h"
#include "touch_helper.h"

#define OSK_BG_COLOUR_LIGHT SDL_MakeColour(236, 239, 241, 255)
#define OSK_BG_COLOUR_DARK  SDL_MakeColour(39, 50, 56, 255)
#define OSK_SELECTED_COLOUR SDL_MakeColour(77, 182, 172, 255)

static int osk_pos_x = 0, osk_pos_y = 0, osk_index = 0;
static bool osk_text_shift;

static const char *osk_textdisp[] = 
{
	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
	"q", "w", "e", "r", "t", "y", "u", "i", "o", "p",
	"a", "s", "d", "f", "g", "h", "j", "k", "l", "z",
	"x", "c", "v", "b", "n", "m", " ", ".", "-", "_",
	"!", "@", "#", "$", "%", "^", "&", "*", "(", ")"
};

static const char *osk_textdisp_shift[] = 
{
	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
	"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P",
	"A", "S", "D", "F", "G", "H", "J", "K", "L", "Z",
	"X", "C", "V", "B", "N", "M", " ", ".", "-", "_",
	"!", "@", "#", "$", "%", "^", "&", "*", "(", ")"
};

static void OSK_DeleteChar(char *str, int i) 
{
	int len = strlen(str);

	for (; i < len - 1 ; i++)
		str[i] = str[i+1];

	str[i] = '\0';
}

void OSK_Display(char *msg)
{
	int text_width = 0, text_height = 0;
	TTF_SizeText(Roboto_OSK, " Q W E R T Y U I O P ", &text_width, &text_height);

	/*if (strlen(msg) != 0)
	{
		osk_index = strlen(msg);
		strcpy(osk_buffer, msg);
	}*/

	TouchInfo touchInfo;
	Touch_Init(&touchInfo);

	while(appletMainLoop())
	{	
		SDL_ClearScreen(RENDERER, config_dark_theme? BLACK_BG : WHITE);
		SDL_DrawRect(RENDERER, 0, 0, 1280, 40, config_dark_theme? STATUS_BAR_DARK : STATUS_BAR_LIGHT);	// Status bar
		SDL_DrawRect(RENDERER, 0, (660 - (text_height * 5)) - 30, 1280, 720 - ((660 - (text_height * 5)) - 30), config_dark_theme? OSK_BG_COLOUR_DARK : OSK_BG_COLOUR_LIGHT);

		if (strlen(osk_buffer) != 0)
		{
			int buf_width = 0, buf_height = 0;
			TTF_SizeText(Roboto_OSK, osk_buffer, &buf_width, &buf_height);

			SDL_DrawText(Roboto_OSK, (1280 - buf_width) / 2, (660 - (text_height * 5)) - 20, config_dark_theme? WHITE : BLACK, osk_buffer);
		}

		for (int x = 0; x <= MAX_X; x++)
		{
			for (int y = 0; y <= MAX_Y; y++)
			{
				if (osk_text_shift)
				{
					if (config_dark_theme)
						SDL_DrawText(Roboto_OSK, ((1280 - (text_width * 2)) / 2) + (100 * x), (660 - (text_height * 4)) + (50 * y), (x == osk_pos_x && y == osk_pos_y)? 
							OSK_SELECTED_COLOUR : WHITE, osk_textdisp_shift[x + y * 10]);
					else
						SDL_DrawText(Roboto_OSK, ((1280 - (text_width * 2)) / 2) + (100 * x), (660 - (text_height * 4)) + (50 * y), (x == osk_pos_x && y == osk_pos_y)? 
							OSK_SELECTED_COLOUR : BLACK, osk_textdisp_shift[x + y * 10]);
				}
				else
				{
					if (config_dark_theme)
						SDL_DrawText(Roboto_OSK, ((1280 - (text_width * 2)) / 2) + (100 * x), (660 - (text_height * 4)) + (50 * y),  (x == osk_pos_x && y == osk_pos_y)?
							OSK_SELECTED_COLOUR : WHITE, osk_textdisp[x + y * 10]);
					else
						SDL_DrawText(Roboto_OSK, ((1280 - (text_width * 2)) / 2) + (100 * x), (660 - (text_height * 4)) + (50 * y),  (x == osk_pos_x && y == osk_pos_y)? 
							OSK_SELECTED_COLOUR : BLACK, osk_textdisp[x + y * 10]);
				}
			}
		}

		StatusBar_DisplayTime();

		SDL_RenderPresent(RENDERER);

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

		if (kDown & KEY_L)
			osk_text_shift = !osk_text_shift;

		if (osk_pos_y > MAX_Y)
			osk_pos_y = 0;
		else if (osk_pos_y < 0)
			osk_pos_y = MAX_Y;

		if (osk_pos_x > MAX_X)
			osk_pos_x = 0;
		else if (osk_pos_x < 0)
			osk_pos_x = MAX_X;

		if (kDown & KEY_A)
		{
			strcat(osk_buffer, osk_text_shift? osk_textdisp_shift[osk_pos_x + osk_pos_y * 10] : osk_textdisp[osk_pos_x + osk_pos_y * 10]);
			osk_index += 1;
		}
		else if (kDown & KEY_B)
		{
			OSK_DeleteChar(osk_buffer, osk_index == 0? osk_index : osk_index - 1);

			if (strlen(osk_buffer) != 0)
				osk_index -= 1;
			else 
				osk_index = 0;
		}

		if (kDown & KEY_PLUS)
			break;
		else if (kDown & KEY_MINUS)
		{
			osk_buffer[0] = '\0';
			break;
		}
	}
}
