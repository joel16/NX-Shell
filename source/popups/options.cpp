#include "config.h"
#include "fs.h"
#include "gui.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "keyboard.h"
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
					FS::GetDirList(config.cwd, item.entries);
					GUI::ResetCheckbox();
					break;
				}
			}
		}

		FS::GetDirList(config.cwd, item.entries);
		GUI::ResetCheckbox();
		entries.clear();
	}

	void OptionsPopup(void) {
		Popups::SetupPopup("Options");

		if (ImGui::BeginPopupModal("Options", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			if (ImGui::Button("Select all", ImVec2(200, 50))) {
				if ((!item.checked_cwd.empty()) && (item.checked_cwd.compare(config.cwd) != 0))
					GUI::ResetCheckbox();

				item.checked_cwd = config.cwd;
				std::fill(item.checked.begin(), item.checked.end(), true);
				item.checked_count = item.checked.size();
			}

			ImGui::SameLine(0.0f, 15.0f);
			
			if (ImGui::Button("Clear all", ImVec2(200, 50))) {
				GUI::ResetCheckbox();
				copy = false;
				move = false;
			}

			ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing

			if (ImGui::Button("Properties", ImVec2(200, 50))) {
				ImGui::CloseCurrentPopup();
				item.state = MENU_STATE_PROPERTIES;
			}

			ImGui::SameLine(0.0f, 15.0f);

			if (ImGui::Button("Rename", ImVec2(200, 50))) {
				std::string path = Keyboard::GetText("Enter name", item.entries[item.selected].name);
				if (R_SUCCEEDED(FS::Rename(&item.entries[item.selected], path.c_str())))
					FS::GetDirList(config.cwd, item.entries);
				
				ImGui::CloseCurrentPopup();
				item.state = MENU_STATE_HOME;
			}
			
			ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
			
			if (ImGui::Button("New Folder", ImVec2(200, 50))) {
				std::string path = config.cwd;
				path.append("/");
				std::string name = Keyboard::GetText("Enter folder name", "New folder");
				path.append(name);
				
				if (R_SUCCEEDED(fsFsCreateDirectory(fs, path.c_str()))) {
					FS::GetDirList(config.cwd, item.entries);
					GUI::ResetCheckbox();
				}
				
				ImGui::CloseCurrentPopup();
				item.state = MENU_STATE_HOME;
			}
			
			ImGui::SameLine(0.0f, 15.0f);
			
			if (ImGui::Button("New File", ImVec2(200, 50))) {
				std::string path = config.cwd;
				path.append("/");
				std::string name = Keyboard::GetText("Enter file name", "New file");
				path.append(name);
				
				if (R_SUCCEEDED(fsFsCreateFile(fs, path.c_str(), 0, 0))) {
					FS::GetDirList(config.cwd, item.entries);
					GUI::ResetCheckbox();
				}
				
				ImGui::CloseCurrentPopup();
				item.state = MENU_STATE_HOME;
			}
			
			ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
			
			if (ImGui::Button(!copy? "Copy" : "Paste", ImVec2(200, 50))) {
				if (!copy) {
					if ((item.checked_count >= 1) && (item.checked_cwd.compare(config.cwd) != 0))
						GUI::ResetCheckbox();
					if (item.checked_count <= 1)
						FS::Copy(&item.entries[item.selected], config.cwd);
						
					copy = !copy;
					item.state = MENU_STATE_HOME;
				}
				else {
					ImGui::EndPopup();
					ImGui::PopStyleVar();
					ImGui::Render();

					if ((item.checked_count > 1) && (item.checked_cwd.compare(config.cwd) != 0))
						Popups::HandleMultipleCopy(&FS::Paste);
					else {
						if (R_SUCCEEDED(FS::Paste())) {
							FS::GetDirList(config.cwd, item.entries);
							GUI::ResetCheckbox();
						}
					}

					copy = !copy;
					item.state = MENU_STATE_HOME;
					return;
				}
			}
			
			ImGui::SameLine(0.0f, 15.0f);
			
			if (ImGui::Button(!move? "Move" : "Paste", ImVec2(200, 50))) {
				if (!move) {
					if ((item.checked_count >= 1) && (item.checked_cwd.compare(config.cwd) != 0))
						GUI::ResetCheckbox();
					if (item.checked_count <= 1)
						FS::Copy(&item.entries[item.selected], config.cwd);
				}
				else {
					if ((item.checked_count > 1) && (item.checked_cwd.compare(config.cwd) != 0))
						Popups::HandleMultipleCopy(&FS::Move);
					else if (R_SUCCEEDED(FS::Move())) {
						FS::GetDirList(config.cwd, item.entries);
						GUI::ResetCheckbox();
					}
				}
				
				move = !move;
				ImGui::CloseCurrentPopup();
				item.state = MENU_STATE_HOME;
			}
			
			ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
			
			if (ImGui::Button("Delete", ImVec2(200, 50))) {
				ImGui::CloseCurrentPopup();
				item.state = MENU_STATE_DELETE;
			}
			
			ImGui::SameLine(0.0f, 15.0f);
			
			if (ImGui::Button("Set archive bit", ImVec2(200, 50))) {
				if (R_SUCCEEDED(FS::SetArchiveBit(&item.entries[item.selected]))) {
					FS::GetDirList(config.cwd, item.entries);
					GUI::ResetCheckbox();
				}
					
				ImGui::CloseCurrentPopup();
				item.state = MENU_STATE_HOME;
			}
		}
		
		Popups::ExitPopup();
	}
}
