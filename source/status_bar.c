#include <time.h>
#include <switch.h>

#include "common.h"
#include "SDL_helper.h"
#include "status_bar.h"

static char *Clock_GetCurrentTime(void)
{
	static char buffer[10];

    time_t unixTime = time(NULL);
	struct tm* timeStruct = gmtime((const time_t *)&unixTime);
	int hours = ((timeStruct->tm_hour) - 5);
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

void StatusBar_DisplayTime(void)
{
	int width = 0, height = 0;
	TTF_SizeText(Roboto, Clock_GetCurrentTime(), &width, &height);

	SDL_DrawText(RENDERER, Roboto, 1260 - width, (40 - height)/2, WHITE, Clock_GetCurrentTime());
}