#include "config.h"
#include "fs.h"
#include "gui.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "keyboard.h"
#include "popups.h"

namespace Popups {
    static void HandleMultipleCopy(Result (*func)()) {
		s64 entry_count = 0;
		FsDirectoryEntry *entries = nullptr;
		
		entry_count = FS::GetDirList(item.checked_cwd.data(), &entries);
		for (long unsigned int i = 0; i < item.checked_copy.size(); i++) {
			if (item.checked_copy.at(i)) {
				FS::Copy(&entries[i], item.checked_cwd);
				if (R_FAILED((*func)())) {
					item.file_count = FS::RefreshEntries(&item.entries, item.file_count);
					GUI::ResetCheckbox();
					break;
				}
			}
		}
		
		item.file_count = FS::RefreshEntries(&item.entries, item.file_count);
		GUI::ResetCheckbox();
		FS::FreeDirEntries(&entries, entry_count);
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

			if (!item.checked_count) {
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
				ImGui::Button("Clear all", ImVec2(200, 50));
				ImGui::PopItemFlag();
				ImGui::PopStyleVar();
			}
			else {
				if (ImGui::Button("Clear all", ImVec2(200, 50)))
					GUI::ResetCheckbox();
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
					item.file_count = FS::RefreshEntries(&item.entries, item.file_count);
				
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
					item.file_count = FS::RefreshEntries(&item.entries, item.file_count);
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
					item.file_count = FS::RefreshEntries(&item.entries, item.file_count);
					GUI::ResetCheckbox();
				}
				
				ImGui::CloseCurrentPopup();
				item.state = MENU_STATE_HOME;
			}
			
			ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
			
			if (ImGui::Button(!item.copy? "Copy" : "Paste", ImVec2(200, 50))) {
				if (!item.copy) {
					if (item.checked_count <= 1)
						FS::Copy(&item.entries[item.selected], config.cwd);
						
					item.copy = !item.copy;
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
							item.file_count = FS::RefreshEntries(&item.entries, item.file_count);
							GUI::ResetCheckbox();
						}
					}

					item.copy = !item.copy;
					item.state = MENU_STATE_HOME;
					return;
				}
			}
			
			ImGui::SameLine(0.0f, 15.0f);
			
			if (ImGui::Button(!item.move? "Move" : "Paste", ImVec2(200, 50))) {
				if (!item.move) {
					if (item.checked_count <= 1)
						FS::Copy(&item.entries[item.selected], config.cwd);
				}
				else {
					if ((item.checked_count > 1) && (item.checked_cwd.compare(config.cwd) != 0))
						Popups::HandleMultipleCopy(&FS::Move);
					else if (R_SUCCEEDED(FS::Move())) {
						item.file_count = FS::RefreshEntries(&item.entries, item.file_count);
						GUI::ResetCheckbox();
					}
				}
				
				item.move = !item.move;
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
					item.file_count = FS::RefreshEntries(&item.entries, item.file_count);
					GUI::ResetCheckbox();
				}
					
				ImGui::CloseCurrentPopup();
				item.state = MENU_STATE_HOME;
			}

			ImGui::Dummy(ImVec2(0.0f, 10.0f)); // Spacing
			
			if (ImGui::Button("OK", ImVec2(120, 0))) {
				ImGui::CloseCurrentPopup();
				item.state = MENU_STATE_HOME;
			}

			if (item.copy || item.move) {
				ImGui::SameLine(0.0f, 15.0f);
				
				if (ImGui::Button("Clear clipboard", ImVec2(280, 0))) {
					item.copy = false;
					item.move = false;
				}
			}

			ImGui::EndPopup();
		}

		ImGui::PopStyleVar();
	}
}
