#include <cstring>
#include <switch.h>

#include "keyboard.h"

namespace Keyboard {
	// Empty strings are invalid.
	SwkbdTextCheckResult ValidateText(char *string, size_t size) {
		if (std::strcmp(string, "") == 0) {
			std::strncpy(string, u8"名称不能为空", size); 
			return SwkbdTextCheckResult_Bad;
		}
		
		return SwkbdTextCheckResult_OK;
	}
	
	std::string GetText(const std::string &guide_text, const std::string &initial_text) {
		Result ret = 0;
		SwkbdConfig swkbd;
		static char input_string[256];
		
		if (R_FAILED(ret = swkbdCreate(&swkbd, 0))) {
			swkbdClose(&swkbd);
			return std::string();
		}
		
		swkbdConfigMakePresetDefault(&swkbd);
		if (!guide_text.empty())
			swkbdConfigSetGuideText(&swkbd, guide_text.c_str());
			
		if (!initial_text.empty())
			swkbdConfigSetInitialText(&swkbd, initial_text.c_str());
		
		swkbdConfigSetTextCheckCallback(&swkbd, Keyboard::ValidateText);
		if (R_FAILED(ret = swkbdShow(&swkbd, input_string, sizeof(input_string)))) {
			swkbdClose(&swkbd);
			return std::string();
		}
		
		swkbdClose(&swkbd);
		return input_string;
	}
}
