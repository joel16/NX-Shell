#include <time.h>
#include <switch.h>

#include "common.h"
#include "SDL_helper.h"
#include "status_bar.h"

static char *Clock_GetCurrentTime(bool _12hour)
{
	static char buffer[10];

    u64 current_time;
	timeGetCurrentTime(TimeType_UserSystemClock, &current_time);
	struct tm* time_struct = gmtime((const time_t *)&current_time);
	int hours = time_struct->tm_hour;
	int minutes = time_struct->tm_min;
	int amOrPm = 0;

	if (_12hour)
	{
		if (hours < 12)
			amOrPm = 1;
		if (hours == 0)
			hours = 12;
		else if (hours > 12)
			hours = hours - 12;

		if ((hours >= 1) && (hours < 10))
			snprintf(buffer, 10, "%2i:%02i %s", hours, minutes, amOrPm ? "AM" : "PM");
		else
			snprintf(buffer, 10, "%2i:%02i %s", hours, minutes, amOrPm ? "AM" : "PM");
	}

	return buffer;
}

void StatusBar_DisplayTime(void)
{
	int width = 0, height = 0;
	TTF_SizeText(Roboto, Clock_GetCurrentTime(true), &width, &height);

	SDL_DrawText(RENDERER, Roboto, 1260 - width, (40 - height)/2, WHITE, Clock_GetCurrentTime(true));
}