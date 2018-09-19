#include <time.h>
#include <switch.h>

#include "common.h"
#include "SDL_helper.h"
#include "status_bar.h"
#include "textures.h"

static char *Clock_GetCurrentTime(void)
{
	static char buffer[10];

    time_t unixTime = time(NULL);
	struct tm* timeStruct = gmtime((const time_t *)&unixTime);
	int hours = (timeStruct->tm_hour);
	int minutes = timeStruct->tm_min;
	
	bool amOrPm = false;
	
	if (hours < 12)
		amOrPm = true;
	if (hours == 0)
		hours = 12;
	else if (hours > 12)
		hours = hours - 12;

	if ((hours >= 1) && (hours < 10))
		snprintf(buffer, 10, "%2i:%02i %s", hours, minutes, amOrPm ? "AM" : "PM");
	else
		snprintf(buffer, 10, "%2i:%02i %s", hours, minutes, amOrPm ? "AM" : "PM");

	return buffer;
}

static void StatusBar_GetBatteryStatus(int x, int y)
{
	u32 percent = 0;
	ChargerType state;
	int width = 0;
	char buf[5];

	if (R_FAILED(psmGetChargerType(&state)))
		state = 0;
	
	if (R_SUCCEEDED(psmGetBatteryChargePercentage(&percent)))
	{
		if (percent < 20)
			SDL_DrawImage(RENDERER, battery_low, x, 3);
		else if ((percent >= 20) && (percent < 30))
		{
			if (state != 0)
				SDL_DrawImage(RENDERER, battery_20_charging, x, 3);
			else
				SDL_DrawImage(RENDERER, battery_20, x, 3);
		}
		else if ((percent >= 30) && (percent < 50))
		{
			if (state != 0)
				SDL_DrawImage(RENDERER, battery_50_charging, x, 3);
			else
				SDL_DrawImage(RENDERER, battery_50, x, 3);
		}
		else if ((percent >= 50) && (percent < 60))
		{
			if (state != 0)
				SDL_DrawImage(RENDERER, battery_50_charging, x, 3);
			else
				SDL_DrawImage(RENDERER, battery_50, x, 3);
		}
		else if ((percent >= 60) && (percent < 80))
		{
			if (state != 0)
				SDL_DrawImage(RENDERER, battery_60_charging, x, 3);
			else
				SDL_DrawImage(RENDERER, battery_60, x, 3);
		}
		else if ((percent >= 80) && (percent < 90))
		{
			if (state != 0)
				SDL_DrawImage(RENDERER, battery_80_charging, x, 3);
			else
				SDL_DrawImage(RENDERER, battery_80, x, 3);
		}
		else if ((percent >= 90) && (percent < 100))
		{
			if (state != 0)
				SDL_DrawImage(RENDERER, battery_90_charging, x, 3);
			else
				SDL_DrawImage(RENDERER, battery_90, x, 3);
		}
		else if (percent == 100)
		{
			if (state != 0)
				SDL_DrawImage(RENDERER, battery_full_charging, x, 3);
			else
				SDL_DrawImage(RENDERER, battery_full, x, 3);
		}

		snprintf(buf, 5, "%d%%", percent);
		TTF_SizeText(Roboto, buf, &width, NULL);
		SDL_DrawText(RENDERER, Roboto, (x - width - 10), y, WHITE, buf);
	}
	else
	{
		snprintf(buf, 5, "%d%%", percent);
		TTF_SizeText(Roboto, buf, &width, NULL);
		SDL_DrawText(RENDERER, Roboto, (x - width - 10), y, WHITE, buf);
		SDL_DrawImage(RENDERER, battery_unknown, x, 1);
	}
}

void StatusBar_DisplayTime(void)
{
	int width = 0, height = 0;
	TTF_SizeText(Roboto, Clock_GetCurrentTime(), &width, &height);

	StatusBar_GetBatteryStatus(1260 - width - 44, (40 - height) / 2);
	SDL_DrawText(RENDERER, Roboto, 1260 - width, (40 - height) / 2, WHITE, Clock_GetCurrentTime());
}