#include "fs.h"
#include "gui.h"
#include "imgui.h"
#include "popups.h"
#include "utils.h"
#include "lang.hpp"
using namespace lang::literals;
namespace Popups {
    static char *FormatDate(char *string, time_t timestamp) {
		strftime(string, 36, "%Y/%m/%d %H:%M:%S", localtime(&timestamp));
		return string;
	}

	void FilePropertiesPopup(void) {
		Popups::SetupPopup("Properties_properties"_lang.c_str());
		
		if (ImGui::BeginPopupModal("Properties_properties"_lang.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			std::string name_text = "Name"_lang+ item.selected_filename;
			ImGui::Text(name_text.c_str());
			
			if (item.entries[item.selected].type == FsDirEntryType_File) {
				char size[16];
				Utils::GetSizeString(size, static_cast<double>(item.entries[item.selected].file_size));
				std::string size_text = "Size"_lang;
				size_text.append(size);
				ImGui::Text(size_text.c_str());
			}
			
			FsTimeStampRaw timestamp;
			if (R_SUCCEEDED(FS::GetTimeStamp(&item.entries[item.selected], &timestamp))) {
				if (timestamp.is_valid == 1) { // Confirm valid timestamp
					char date[3][36];
					
					std::string created_time = "Created"_lang;
					created_time.append(Popups::FormatDate(date[0], timestamp.created));
					ImGui::Text(created_time.c_str());
					
					std::string modified_time = "Modified"_lang;
					modified_time.append(Popups::FormatDate(date[1], timestamp.modified));
					ImGui::Text(modified_time.c_str());
					
					std::string accessed_time = "Accessed"_lang;
					accessed_time.append(Popups::FormatDate(date[2], timestamp.accessed));
					ImGui::Text(accessed_time.c_str());
				}
			}
			
			ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
			
			if (ImGui::Button("OK_properties"_lang.c_str(), ImVec2(120, 0))) {
				ImGui::CloseCurrentPopup();
				item.state = MENU_STATE_OPTIONS;
			}
		}
		
		Popups::ExitPopup();
	}
	
	void ImageProperties(bool *state) {
		Popups::SetupPopup("Properties_properties_2"_lang.c_str());
		
		if (ImGui::BeginPopupModal("Properties_properties_2"_lang.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			std::string name_text = "Name_properties"_lang+ item.selected_filename;
			ImGui::Text(name_text.c_str());
			
			std::string width_text = "Width"_lang;
			width_text.append(std::to_string(item.texture.width));
			width_text.append("px");
			ImGui::Text(width_text.c_str());
			
			std::string height_text = "Height"_lang;
			height_text.append(std::to_string(item.texture.height));
			height_text.append("px");
			ImGui::Text(height_text.c_str());
			
			ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
			
			if (ImGui::Button("OK_properties_2"_lang.c_str(), ImVec2(120, 0))) {
				ImGui::CloseCurrentPopup();
				*state = false;
			}
		}
		
		Popups::ExitPopup();
	}
}
