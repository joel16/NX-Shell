#include "config.h"
#include "fs.h"
#include "gui.h"
#include "imgui.h"
#include "popups.h"

namespace Popups {
    void DeletePopup(void) {
		Popups::SetupPopup(u8"删除");
		
		if (ImGui::BeginPopupModal(u8"删除", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text(u8"此操作无法撤消");
			if ((item.checked_count > 1) && (!item.checked_cwd.compare(config.cwd))) {
				ImGui::Text(u8"是否要删除以下内容:");
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				ImGui::BeginChild("Scrolling", ImVec2(0, 100));
				for (long unsigned int i = 0; i < item.checked.size(); i++) {
					if (item.checked.at(i))
						ImGui::Text(item.entries[i].name);
				}
				ImGui::EndChild();
			}
			else {
				std::string text = u8"是否要删除 " + item.selected_filename + "?";
				ImGui::Text(text.c_str());
			}
			
			ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
			
			if (ImGui::Button(u8"确定", ImVec2(120, 0))) {
				Result ret = 0;
				if ((item.checked_count > 1) && (!item.checked_cwd.compare(config.cwd))) {
					for (long unsigned int i = 0; i < item.checked.size(); i++) {
						if (item.checked.at(i)) {
							if (R_FAILED(ret = FS::Delete(&item.entries[i]))) {
								item.file_count = FS::RefreshEntries(&item.entries, item.file_count);
								GUI::ResetCheckbox();
								break;
							}
						}
					}
				}
				else
					ret = FS::Delete(&item.entries[item.selected]);
				
				if (R_SUCCEEDED(ret)) {
					item.file_count = FS::RefreshEntries(&item.entries, item.file_count);
					GUI::ResetCheckbox();
				}
				
				ImGui::CloseCurrentPopup();
				item.state = MENU_STATE_HOME;
			}
			
			ImGui::SetItemDefaultFocus();
			ImGui::SameLine(0.0f, 15.0f);
			
			if (ImGui::Button(u8"取消", ImVec2(120, 0))) {
				ImGui::CloseCurrentPopup();
				item.state = MENU_STATE_OPTIONS;
			}
			
			ImGui::EndPopup();
		}
		
		ImGui::PopStyleVar();
	}
}
