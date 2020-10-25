#include "config.h"
#include "fs.h"
#include "gui.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "keyboard.h"
#include "language.h"
#include "popups.h"

namespace Popups {
	static bool copy = false, move = false;
    static void HandleMultipleCopy(Result (*func)()) {
		Result ret = 0;
		std::vector<FsDirectoryEntry> entries;
		
		if (R_FAILED(ret = FS::GetDirList(item.checked_cwd.data(), entries)))
			return;
		
		for (long unsigned int i = 0; i < item.checked_copy.size(); i++) {
			if (item.checked_copy.at(i)) {
				FS::Copy(&entries[i], item.checked_cwd);
				if (R_FAILED((*func)())) {
					FS::GetDirList(cfg.cwd, item.entries);
					GUI::ResetCheckbox();
					break;
				}
			}
		}

		FS::GetDirList(cfg.cwd, item.entries);
		GUI::ResetCheckbox();
		entries.clear();
	}

	void OptionsPopup(void) {
		Popups::SetupPopup(strings[cfg.lang][Lang::OptionsTitle]);

		if (ImGui::BeginPopupModal(strings[cfg.lang][Lang::OptionsTitle], nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			if (ImGui::Button(strings[cfg.lang][Lang::OptionsSelectAll], ImVec2(200, 50))) {
				if ((!item.checked_cwd.empty()) && (item.checked_cwd.compare(cfg.cwd) != 0))
					GUI::ResetCheckbox();

				item.checked_cwd = cfg.cwd;
				std::fill(item.checked.begin(), item.checked.end(), true);
				item.checked_count = item.checked.size();
			}

			ImGui::SameLine(0.0f, 15.0f);
			
			if (ImGui::Button(strings[cfg.lang][Lang::OptionsClearAll], ImVec2(200, 50))) {
				GUI::ResetCheckbox();
				copy = false;
				move = false;
			}

			ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing

			if (ImGui::Button(strings[cfg.lang][Lang::OptionsProperties], ImVec2(200, 50))) {
				ImGui::CloseCurrentPopup();
				item.state = MENU_STATE_PROPERTIES;
			}

			ImGui::SameLine(0.0f, 15.0f);

			if (ImGui::Button(strings[cfg.lang][Lang::OptionsRename], ImVec2(200, 50))) {
				std::string path = Keyboard::GetText(strings[cfg.lang][Lang::OptionsRenamePrompt], item.entries[item.selected].name);
				if (R_SUCCEEDED(FS::Rename(&item.entries[item.selected], path.c_str())))
					FS::GetDirList(cfg.cwd, item.entries);
				
				ImGui::CloseCurrentPopup();
				item.state = MENU_STATE_FILEBROWSER;
			}
			
			ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
			
			if (ImGui::Button(strings[cfg.lang][Lang::OptionsNewFolder], ImVec2(200, 50))) {
				std::string path = cfg.cwd;
				path.append("/");
				std::string name = Keyboard::GetText(strings[cfg.lang][Lang::OptionsFolderPrompt], strings[cfg.lang][Lang::OptionsNewFolder]);
				path.append(name);
				
				if (R_SUCCEEDED(fsFsCreateDirectory(fs, path.c_str()))) {
					FS::GetDirList(cfg.cwd, item.entries);
					GUI::ResetCheckbox();
				}
				
				ImGui::CloseCurrentPopup();
				item.state = MENU_STATE_FILEBROWSER;
			}
			
			ImGui::SameLine(0.0f, 15.0f);
			
			if (ImGui::Button(strings[cfg.lang][Lang::OptionsNewFile], ImVec2(200, 50))) {
				std::string path = cfg.cwd;
				path.append("/");
				std::string name = Keyboard::GetText(strings[cfg.lang][Lang::OptionsFilePrompt], strings[cfg.lang][Lang::OptionsNewFile]);
				path.append(name);
				
				if (R_SUCCEEDED(fsFsCreateFile(fs, path.c_str(), 0, 0))) {
					FS::GetDirList(cfg.cwd, item.entries);
					GUI::ResetCheckbox();
				}
				
				ImGui::CloseCurrentPopup();
				item.state = MENU_STATE_FILEBROWSER;
			}
			
			ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
			
			if (ImGui::Button(!copy? strings[cfg.lang][Lang::OptionsCopy] : strings[cfg.lang][Lang::OptionsPaste], ImVec2(200, 50))) {
				if (!copy) {
					if ((item.checked_count >= 1) && (item.checked_cwd.compare(cfg.cwd) != 0))
						GUI::ResetCheckbox();
					if (item.checked_count <= 1)
						FS::Copy(&item.entries[item.selected], cfg.cwd);
						
					copy = !copy;
					item.state = MENU_STATE_FILEBROWSER;
				}
				else {
					ImGui::EndPopup();
					ImGui::PopStyleVar();
					ImGui::Render();

					if ((item.checked_count > 1) && (item.checked_cwd.compare(cfg.cwd) != 0))
						Popups::HandleMultipleCopy(&FS::Paste);
					else {
						if (R_SUCCEEDED(FS::Paste())) {
							FS::GetDirList(cfg.cwd, item.entries);
							GUI::ResetCheckbox();
						}
					}

					copy = !copy;
					item.state = MENU_STATE_FILEBROWSER;
					return;
				}
			}
			
			ImGui::SameLine(0.0f, 15.0f);
			
			if (ImGui::Button(!move? strings[cfg.lang][Lang::OptionsMove] : strings[cfg.lang][Lang::OptionsPaste], ImVec2(200, 50))) {
				if (!move) {
					if ((item.checked_count >= 1) && (item.checked_cwd.compare(cfg.cwd) != 0))
						GUI::ResetCheckbox();
					if (item.checked_count <= 1)
						FS::Copy(&item.entries[item.selected], cfg.cwd);
				}
				else {
					if ((item.checked_count > 1) && (item.checked_cwd.compare(cfg.cwd) != 0))
						Popups::HandleMultipleCopy(&FS::Move);
					else if (R_SUCCEEDED(FS::Move())) {
						FS::GetDirList(cfg.cwd, item.entries);
						GUI::ResetCheckbox();
					}
				}
				
				move = !move;
				ImGui::CloseCurrentPopup();
				item.state = MENU_STATE_FILEBROWSER;
			}
			
			ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
			
			if (ImGui::Button(strings[cfg.lang][Lang::OptionsDelete], ImVec2(200, 50))) {
				ImGui::CloseCurrentPopup();
				item.state = MENU_STATE_DELETE;
			}
			
			ImGui::SameLine(0.0f, 15.0f);
			
			if (ImGui::Button(strings[cfg.lang][Lang::OptionsSetArchiveBit], ImVec2(200, 50))) {
				if (R_SUCCEEDED(FS::SetArchiveBit(&item.entries[item.selected]))) {
					FS::GetDirList(cfg.cwd, item.entries);
					GUI::ResetCheckbox();
				}
					
				ImGui::CloseCurrentPopup();
				item.state = MENU_STATE_FILEBROWSER;
			}
		}
		
		Popups::ExitPopup();
	}
}
