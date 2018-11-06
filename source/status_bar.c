#include <time.h>

#include "common.h"
#include "SDL_helper.h"
#include "status_bar.h"
#include "textures.h"

static char *Clock_GetCurrentTime(void) {
	static char buffer[27];

	time_t rawtime;
	struct tm* timeStruct;

	time(&rawtime);
	timeStruct = localtime(&rawtime);

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
		snprintf(buffer, 27, "%2i:%02i %s", hours, minutes, amOrPm ? "AM" : "PM");
	else
		snprintf(buffer, 27, "%2i:%02i %s", hours, minutes, amOrPm ? "AM" : "PM");

	return buffer;
}

static void StatusBar_GetBatteryStatus(int x, int y) {
	u32 percent = 0;
	ChargerType state;
	u32 width = 0;
	char buf[6];

	if (R_FAILED(psmGetChargerType(&state)))
		state = 0;
	
	if (R_SUCCEEDED(psmGetBatteryChargePercentage(&percent))) {
		if (percent < 20)
			SDL_DrawImage(battery_low, x, 3);
		else if ((percent >= 20) && (percent < 30)) {
			if (state != 0)
				SDL_DrawImage(battery_20_charging, x, 3);
			else
				SDL_DrawImage(battery_20, x, 3);
		}
		else if ((percent >= 30) && (percent < 50)) {
			if (state != 0)
				SDL_DrawImage(battery_50_charging, x, 3);
			else
				SDL_DrawImage(battery_50, x, 3);
		}
		else if ((percent >= 50) && (percent < 60)) {
			if (state != 0)
				SDL_DrawImage(battery_50_charging, x, 3);
			else
				SDL_DrawImage(battery_50, x, 3);
		}
		else if ((percent >= 60) && (percent < 80)) {
			if (state != 0)
				SDL_DrawImage(battery_60_charging, x, 3);
			else
				SDL_DrawImage(battery_60, x, 3);
		}
		else if ((percent >= 80) && (percent < 90)) {
			if (state != 0)
				SDL_DrawImage(battery_80_charging, x, 3);
			else
				SDL_DrawImage(battery_80, x, 3);
		}
		else if ((percent >= 90) && (percent < 100)) {
			if (state != 0)
				SDL_DrawImage(battery_90_charging, x, 3);
			else
				SDL_DrawImage(battery_90, x, 3);
		}
		else if (percent == 100) {
			if (state != 0)
				SDL_DrawImage(battery_full_charging, x, 3);
			else
				SDL_DrawImage(battery_full, x, 3);
		}

		snprintf(buf, 4, "%d", percent);
		strcat(buf, "%%");
		SDL_GetTextDimensions(25, buf, &width, NULL);
		SDL_DrawText((x - width - 10), y, 25, WHITE, buf);
	}
	else {
		snprintf(buf, 4, "%d", percent);
		strcat(buf, "%%");
		SDL_GetTextDimensions(25, buf, &width, NULL);
		SDL_DrawText((x - width - 10), y, 25, WHITE, buf);
		SDL_DrawImage(battery_unknown, x, 1);
	}
}

void StatusBar_DisplayTime(void) {
	u32 width = 0, height = 0;
	SDL_GetTextDimensions(25, Clock_GetCurrentTime(), &width, &height);

	StatusBar_GetBatteryStatus(1260 - width - 44, (40 - height) / 2);
	SDL_DrawText(1260 - width, (40 - height) / 2, 25, WHITE, Clock_GetCurrentTime());
}
