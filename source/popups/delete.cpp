#include "config.h"
#include "fs.h"
#include "gui.h"
#include "imgui.h"
#include "language.h"
#include "log.h"
#include "popups.h"

namespace Popups {
    void DeletePopup(void) {
		Popups::SetupPopup(strings[cfg.lang][Lang::OptionsDelete]);
		
		if (ImGui::BeginPopupModal(strings[cfg.lang][Lang::OptionsDelete], nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text(strings[cfg.lang][Lang::DeleteMessage]);
			if ((item.checked_count > 1) && (!item.checked_cwd.compare(cfg.cwd))) {
				ImGui::Text(strings[cfg.lang][Lang::DeleteMultiplePrompt]);
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				ImGui::BeginChild("Scrolling", ImVec2(0, 100));
				for (long unsigned int i = 0; i < item.checked.size(); i++) {
					if (item.checked.at(i))
						ImGui::Text(item.entries[i].name);
				}
				ImGui::EndChild();
			}
			else {
				std::string text = strings[cfg.lang][Lang::DeletePrompt] + std::string(item.entries[item.selected].name) + "?";
				ImGui::Text(text.c_str());
			}
			
			ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
			
			if (ImGui::Button(strings[cfg.lang][Lang::ButtonOK], ImVec2(120, 0))) {
				Result ret = 0;
				Log::Exit();

				if ((item.checked_count > 1) && (!item.checked_cwd.compare(cfg.cwd))) {
					for (long unsigned int i = 0; i < item.checked.size(); i++) {
						if (item.checked.at(i)) {
							if (R_FAILED(ret = FS::Delete(&item.entries[i]))) {
								FS::GetDirList(cfg.cwd, item.entries);
								GUI::ResetCheckbox();
								break;
							}
						}
					}
				}
				else
					ret = FS::Delete(&item.entries[item.selected]);
				
				if (R_SUCCEEDED(ret)) {
					FS::GetDirList(cfg.cwd, item.entries);
					GUI::ResetCheckbox();
				}

				Log::Exit();
				ImGui::CloseCurrentPopup();
				item.state = MENU_STATE_FILEBROWSER;
			}
			
			ImGui::SameLine(0.0f, 15.0f);
			
			if (ImGui::Button(strings[cfg.lang][Lang::ButtonCancel], ImVec2(120, 0))) {
				ImGui::CloseCurrentPopup();
				item.state = MENU_STATE_OPTIONS;
			}
		}
		
		Popups::ExitPopup();
	}
}
