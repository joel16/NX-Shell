#include <string.h>
#include <switch.h>

#include "osk.h"

const char *OSK_GetString(const char *guide_text, const char *initial_text) {
	SwkbdConfig swkbd;
	Result ret = 0;
	static char input_string[256];

	if (R_SUCCEEDED(ret = swkbdCreate(&swkbd, 0))) {
		swkbdConfigMakePresetDefault(&swkbd);

		if (strlen(guide_text) != 0)
			swkbdConfigSetGuideText(&swkbd, guide_text);

		if (strlen(initial_text) != 0)
			swkbdConfigSetInitialText(&swkbd, initial_text);

		if (R_FAILED(ret = swkbdShow(&swkbd, input_string, sizeof(input_string))))
			return NULL;
	}

	swkbdClose(&swkbd);
	return input_string;
}
