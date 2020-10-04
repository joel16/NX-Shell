#include "config.h"
#include "fs.h"
#include "gui.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "keyboard.h"
#include "popups.h"
#include "lang.hpp"
using namespace lang::literals;
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
		Popups::SetupPopup("Options"_lang.c_str());

		if (ImGui::BeginPopupModal("Options"_lang.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			if (ImGui::Button("Select_all"_lang.c_str() , ImVec2(200, 50))) { //Select all
				if ((!item.checked_cwd.empty()) && (item.checked_cwd.compare(config.cwd) != 0))
					GUI::ResetCheckbox();

				item.checked_cwd = config.cwd;
				std::fill(item.checked.begin(), item.checked.end(), true);
				item.checked_count = item.checked.size();
			}

			ImGui::SameLine(0.0f, 15.0f);
			
			if (ImGui::Button("Clear_all"_lang.c_str() , ImVec2(200, 50))) {
				GUI::ResetCheckbox();
				item.copy = false;
				item.move = false;
			}

			ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing

			if (ImGui::Button("Properties"_lang.c_str(), ImVec2(200, 50))) {
				ImGui::CloseCurrentPopup();
				item.state = MENU_STATE_PROPERTIES;
			}

			ImGui::SameLine(0.0f, 15.0f);

			if (ImGui::Button("Rename"_lang.c_str(), ImVec2(200, 50))) {
				std::string path = Keyboard::GetText("Enter_name"_lang.c_str(), item.entries[item.selected].name);
				if (R_SUCCEEDED(FS::Rename(&item.entries[item.selected], path.c_str())))
					item.file_count = FS::RefreshEntries(&item.entries, item.file_count);
				
				ImGui::CloseCurrentPopup();
				item.state = MENU_STATE_HOME;
			}
			
			ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
			
			if (ImGui::Button("New_Folder"_lang.c_str(), ImVec2(200, 50))) {
				std::string path = config.cwd;
				path.append("/");
				std::string name = Keyboard::GetText("Enter_folder_name"_lang.c_str(), "New_folder"_lang.c_str());
				path.append(name);
				
				if (R_SUCCEEDED(fsFsCreateDirectory(fs, path.c_str()))) {
					item.file_count = FS::RefreshEntries(&item.entries, item.file_count);
					GUI::ResetCheckbox();
				}
				
				ImGui::CloseCurrentPopup();
				item.state = MENU_STATE_HOME;
			}
			
			ImGui::SameLine(0.0f, 15.0f);
			
			if (ImGui::Button("New_File"_lang.c_str(), ImVec2(200, 50))) {
				std::string path = config.cwd;
				path.append("/");
				std::string name = Keyboard::GetText("Enter_file_name"_lang.c_str(), "New_file"_lang.c_str());
				path.append(name);
				
				if (R_SUCCEEDED(fsFsCreateFile(fs, path.c_str(), 0, 0))) {
					item.file_count = FS::RefreshEntries(&item.entries, item.file_count);
					GUI::ResetCheckbox();
				}
				
				ImGui::CloseCurrentPopup();
				item.state = MENU_STATE_HOME;
			}
			
			ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
			
			if (ImGui::Button(!item.copy? "Copy"_lang.c_str() : "Paste_1"_lang.c_str(), ImVec2(200, 50))) {
				if (!item.copy) {
					if ((item.checked_count >= 1) && (item.checked_cwd.compare(config.cwd) != 0))
						GUI::ResetCheckbox();
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
			
			if (ImGui::Button(!item.move? "Move"_lang.c_str() : "Paste_2"_lang.c_str(), ImVec2(200, 50))) {
				if (!item.move) {
					if ((item.checked_count >= 1) && (item.checked_cwd.compare(config.cwd) != 0))
						GUI::ResetCheckbox();
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
			
			if (ImGui::Button("Delete"_lang.c_str(), ImVec2(200, 50))) {
				ImGui::CloseCurrentPopup();
				item.state = MENU_STATE_DELETE;
			}
			
			ImGui::SameLine(0.0f, 15.0f);
			
			if (ImGui::Button("Set_archive_bit"_lang.c_str(), ImVec2(200, 50))) {
				if (R_SUCCEEDED(FS::SetArchiveBit(&item.entries[item.selected]))) {
					item.file_count = FS::RefreshEntries(&item.entries, item.file_count);
					GUI::ResetCheckbox();
				}
					
				ImGui::CloseCurrentPopup();
				item.state = MENU_STATE_HOME;
			}
		}
		
		Popups::ExitPopup();
	}
}
