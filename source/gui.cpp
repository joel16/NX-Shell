#include <algorithm>
#include <cstdio>
#include <iostream>
#include <vector>

#include "config.h"
#include "fs.h"
#include "gui.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include "keyboard.h"
#include "textures.h"
#include "utils.h"

namespace GUI {
	enum MENU_STATES {
		MENU_STATE_HOME = 0,
		MENU_STATE_OPTIONS = 1,
		MENU_STATE_DELETE = 2,
		MENU_STATE_PROPERTIES = 3,
		MENU_STATE_SETTINGS = 4,
		MENU_STATE_IMAGEVIEWER = 5
	};
	
	typedef struct {
		int state = 0;
		bool copy = false;
		bool move = false;
		int selected = 0;
		int file_count = 0;
		std::string selected_filename;
		FsDirectoryEntry *entries = nullptr;
		std::vector<bool> checked;
		std::vector<bool> checked_copy;
		std::string checked_cwd;
		int checked_count = 0;
	} MenuItem;

	static void ResetCheckbox(MenuItem *item) {
		item->checked.clear();
		item->checked_copy.clear();
		item->checked.resize(item->file_count);
		item->checked.assign(item->checked.size(), false);
		item->checked_cwd.clear();
		item->checked_count = 0;
	}

	static void HandleMultipleCopy(MenuItem *item, Result (*func)()) {
		s64 entry_count = 0;
		FsDirectoryEntry *entries = nullptr;
		
		entry_count = FS::GetDirList(item->checked_cwd.data(), &entries);
		for (long unsigned int i = 0; i < item->checked_copy.size(); i++) {
			if (item->checked_copy.at(i)) {
				FS::Copy(&entries[i], item->checked_cwd);
				if (R_FAILED((*func)())) {
					item->file_count = FS::RefreshEntries(&item->entries, item->file_count);
					GUI::ResetCheckbox(item);
					break;
				}
			}
		}
		
		item->file_count = FS::RefreshEntries(&item->entries, item->file_count);
		GUI::ResetCheckbox(item);
		FS::FreeDirEntries(&entries, entry_count);
	}

	static void RenderOptionsMenu(MenuItem *item) {
		ImGui::OpenPopup("Options");
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
		ImGui::SetNextWindowPos(ImVec2(640.0f, 360.0f), ImGuiCond_Once, ImVec2(0.5f, 0.5f));

		if (ImGui::BeginPopupModal("Options", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			if (ImGui::Button("Properties", ImVec2(200, 50))) {
				ImGui::CloseCurrentPopup();
				item->state = MENU_STATE_PROPERTIES;
			}

			ImGui::SameLine(0.0f, 15.0f);

			if (ImGui::Button("Rename", ImVec2(200, 50))) {
				std::string path = Keyboard::GetText("Enter name", item->entries[item->selected].name);
				if (R_SUCCEEDED(FS::Rename(&item->entries[item->selected], path.c_str())))
					item->file_count = FS::RefreshEntries(&item->entries, item->file_count);
				
				ImGui::CloseCurrentPopup();
				item->state = MENU_STATE_HOME;
			}
			
			ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
			
			if (ImGui::Button("New Folder", ImVec2(200, 50))) {
				std::string path = config.cwd;
				path.append("/");
				std::string name = Keyboard::GetText("Enter folder name", "New folder");
				path.append(name);
				
				if (R_SUCCEEDED(fsFsCreateDirectory(fs, path.c_str()))) {
					item->file_count = FS::RefreshEntries(&item->entries, item->file_count);
					GUI::ResetCheckbox(item);
				}
				
				ImGui::CloseCurrentPopup();
				item->state = MENU_STATE_HOME;
			}
			
			ImGui::SameLine(0.0f, 15.0f);
			
			if (ImGui::Button("New File", ImVec2(200, 50))) {
				std::string path = config.cwd;
				path.append("/");
				std::string name = Keyboard::GetText("Enter file name", "New file");
				path.append(name);
				
				if (R_SUCCEEDED(fsFsCreateFile(fs, path.c_str(), 0, 0))) {
					item->file_count = FS::RefreshEntries(&item->entries, item->file_count);
					GUI::ResetCheckbox(item);
				}
				
				ImGui::CloseCurrentPopup();
				item->state = MENU_STATE_HOME;
			}
			
			ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
			
			if (ImGui::Button(!item->copy? "Copy" : "Paste", ImVec2(200, 50))) {
				if (!item->copy) {
					if (item->checked_count <= 1)
						FS::Copy(&item->entries[item->selected], config.cwd);
				}
				else {
					if ((item->checked_count > 1) && (item->checked_cwd.compare(config.cwd) != 0))
						GUI::HandleMultipleCopy(item, &FS::Paste);
					else if (R_SUCCEEDED(FS::Paste())) {
						item->file_count = FS::RefreshEntries(&item->entries, item->file_count);
						GUI::ResetCheckbox(item);
					}
				}
				
				item->copy = !item->copy;
				ImGui::CloseCurrentPopup();
				item->state = MENU_STATE_HOME;
			}
			
			ImGui::SameLine(0.0f, 15.0f);
			
			if (ImGui::Button(!item->move? "Move" : "Paste", ImVec2(200, 50))) {
				if (!item->move) {
					if (item->checked_count <= 1)
						FS::Copy(&item->entries[item->selected], config.cwd);
				}
				else {
					if ((item->checked_count > 1) && (item->checked_cwd.compare(config.cwd) != 0))
						GUI::HandleMultipleCopy(item, &FS::Move);
					else if (R_SUCCEEDED(FS::Move())) {
						item->file_count = FS::RefreshEntries(&item->entries, item->file_count);
						GUI::ResetCheckbox(item);
					}
				}
				
				item->move = !item->move;
				ImGui::CloseCurrentPopup();
				item->state = MENU_STATE_HOME;
			}
			
			ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
			
			if (ImGui::Button("Delete", ImVec2(200, 50))) {
				ImGui::CloseCurrentPopup();
				item->state = MENU_STATE_DELETE;
			}
			
			ImGui::SameLine(0.0f, 15.0f);
			
			if (ImGui::Button("Set archive bit", ImVec2(200, 50))) {
				if (R_SUCCEEDED(FS::SetArchiveBit(&item->entries[item->selected]))) {
					item->file_count = FS::RefreshEntries(&item->entries, item->file_count);
					GUI::ResetCheckbox(item);
				}
					
				ImGui::CloseCurrentPopup();
				item->state = MENU_STATE_HOME;
			}
			
			ImGui::EndPopup();
		}

		ImGui::PopStyleVar();
	}

	static char *FormatDate(char *string, time_t timestamp) {
		strftime(string, 36, "%Y/%m/%d %H:%M:%S", localtime(&timestamp));
		return string;
	}

	#define IM_CLAMP(V, MN, MX)     ((V) < (MN) ? (MN) : (V) > (MX) ? (MX) : (V))

	static void RenderPropertiesMenu(MenuItem *item) {
		ImGui::OpenPopup("Properties");
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
		ImGui::SetNextWindowPos(ImVec2(640.0f, 360.0f), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
		
		if (ImGui::BeginPopupModal("Properties", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			std::string name_text = "Name: " + item->selected_filename;
			ImGui::Text(name_text.c_str());
			
			if (item->entries[item->selected].type == FsDirEntryType_File) {
				char size[16];
				Utils::GetSizeString(size, item->entries[item->selected].file_size);
				std::string size_text = "Size: ";
				size_text.append(size);
				ImGui::Text(size_text.c_str());
			}
			
			FsTimeStampRaw timestamp;
			if (R_SUCCEEDED(FS::GetTimeStamp(&item->entries[item->selected], &timestamp))) {
				if (timestamp.is_valid == 1) { // Confirm valid timestamp
					char date[3][36];
					
					std::string created_time = "Created: ";
					created_time.append(GUI::FormatDate(date[0], timestamp.created));
					ImGui::Text(created_time.c_str());
					
					std::string modified_time = "Modified: ";
					modified_time.append(GUI::FormatDate(date[1], timestamp.modified));
					ImGui::Text(modified_time.c_str());
					
					std::string accessed_time = "Accessed: ";
					accessed_time.append(GUI::FormatDate(date[2], timestamp.accessed));
					ImGui::Text(accessed_time.c_str());
				}
			}
			
			ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
			
			if (ImGui::Button("OK", ImVec2(120, 0))) {
				ImGui::CloseCurrentPopup();
				item->state = MENU_STATE_OPTIONS;
			}

			ImGui::EndPopup();
		}

		ImGui::PopStyleVar();
	}

	static void RenderDeleteMenu(MenuItem *item) {
		ImGui::OpenPopup("Delete");
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
		ImGui::SetNextWindowPos(ImVec2(640.0f, 360.0f), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
		
		if (ImGui::BeginPopupModal("Delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("This action cannot be undone");
			if ((item->checked_count > 1) && (!item->checked_cwd.compare(config.cwd))) {
				ImGui::Text("Do you wish to delete the following:");
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				ImGui::BeginChild("Scrolling", ImVec2(0, 100));
				for (long unsigned int i = 0; i < item->checked.size(); i++) {
					if (item->checked.at(i))
						ImGui::Text(item->entries[i].name);
				}
				ImGui::EndChild();
			}
			else {
				std::string text = "Do you wish to delete " + item->selected_filename + "?";
				ImGui::Text(text.c_str());
			}
			
			ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
			
			if (ImGui::Button("OK", ImVec2(120, 0))) {
				Result ret = 0;
				if ((item->checked_count > 1) && (!item->checked_cwd.compare(config.cwd))) {
					for (long unsigned int i = 0; i < item->checked.size(); i++) {
						if (item->checked.at(i)) {
							if (R_FAILED(ret = FS::Delete(&item->entries[i]))) {
								item->file_count = FS::RefreshEntries(&item->entries, item->file_count);
								GUI::ResetCheckbox(item);
								break;
							}
						}
					}
				}
				else
					ret = FS::Delete(&item->entries[item->selected]);
				
				if (R_SUCCEEDED(ret)) {
					item->file_count = FS::RefreshEntries(&item->entries, item->file_count);
					GUI::ResetCheckbox(item);
				}
				
				ImGui::CloseCurrentPopup();
				item->state = MENU_STATE_HOME;
			}
			
			ImGui::SetItemDefaultFocus();
			ImGui::SameLine(0.0f, 15.0f);
			
			if (ImGui::Button("Cancel", ImVec2(120, 0))) {
				ImGui::CloseCurrentPopup();
				item->state = MENU_STATE_OPTIONS;
			}
			
			ImGui::EndPopup();
		}
		
		ImGui::PopStyleVar();
	}
	
	static void RenderSettingsMenu(MenuItem *item) {
		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(1280.0f, 720.0f), ImGuiCond_Once);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		
		if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
			if (ImGui::TreeNode("Sort Settings")) {
				ImGui::RadioButton(" By name (ascending)", &config.sort, 0);
				ImGui::RadioButton(" By name (descending)", &config.sort, 1);
				ImGui::Dummy(ImVec2(0.0f, 10.0f)); // Spacing
				ImGui::RadioButton(" By size (largest first)", &config.sort, 2);
				ImGui::Dummy(ImVec2(0.0f, 10.0f)); // Spacing
				ImGui::RadioButton(" By size (smallest first)", &config.sort, 3);
				ImGui::TreePop();
			}
			
			ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
			
			if (ImGui::TreeNode("About")) {
				ImGui::Text("NX-Shell Version: v%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO);
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				ImGui::Text("ImGui Version: %s",  ImGui::GetVersion());
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				ImGui::Text("Author: Joel16");
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				ImGui::Text("Banner: Preetisketch");
				ImGui::TreePop();
			}
		}
		
		ImGui::End();
		ImGui::PopStyleVar();
		Config::Save(config);
		item->file_count = FS::RefreshEntries(&item->entries, item->file_count);
	}

	static void RenderImageViewer(MenuItem *item, Tex *texture) {
		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(1280.0f, 720.0f), ImGuiCond_Once);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

		if (ImGui::Begin(item->selected_filename.c_str(), nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
			ImGui::Image((void *)(intptr_t)texture->id, ImVec2(texture->width, texture->height));

		ImGui::End();
		ImGui::PopStyleVar();
	}

	int RenderMainMenu(SDL_Window *window) {
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		// Used for displaying images
		Tex texture;
		
		MenuItem item;
		item.state = MENU_STATE_HOME;
		item.selected = 0;
		item.file_count = FS::GetDirList(config.cwd, &item.entries);
		if (item.file_count < 0)
			return -1;

		item.checked.resize(item.file_count);
			
		// Main loop
		bool done = false, set_focus = false, first = true;
		while (!done) {
			// Start the Dear ImGui frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplSDL2_NewFrame(window);
			ImGui::NewFrame();
			
			ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Once);
			ImGui::SetNextWindowSize(ImVec2(1280.0f, 720.0f), ImGuiCond_Once);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			
			if (ImGui::Begin("NX-Shell", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
				// Initially set default focus to next window (FS::DirList)
				if (!set_focus) {
					ImGui::SetNextWindowFocus();
					set_focus = true;
				}
				
				// Display current working directory
				ImGui::TextColored(ImVec4(1.00f, 1.00f, 1.00f, 1.00f), config.cwd);
				ImGui::BeginChild("##FS::DirList");
				
				if (item.file_count != 0) {
					for (s64 i = 0; i < item.file_count; i++) {
						std::string filename = item.entries[i].name;

						if ((item.checked.at(i)) && (!item.checked_cwd.compare(config.cwd)))
							ImGui::Image((void *)(intptr_t)check_icon.id, ImVec2(check_icon.width, check_icon.height));
						else
							ImGui::Image((void *)(intptr_t)uncheck_icon.id, ImVec2(uncheck_icon.width, uncheck_icon.height));
						ImGui::SameLine();

						if (item.entries[i].type == FsDirEntryType_Dir)
							ImGui::Image((void *)(intptr_t)folder_icon.id, ImVec2(folder_icon.width, folder_icon.height));
						else {
							FileType file_type = FS::GetFileType(filename);
							switch(file_type) {
								case FileTypeArchive:
									ImGui::Image((void *)(intptr_t)archive_icon.id, ImVec2(archive_icon.width, archive_icon.height));
									break;

								case FileTypeAudio:
									ImGui::Image((void *)(intptr_t)audio_icon.id, ImVec2(audio_icon.width, audio_icon.height));
									break;

								case FileTypeImage:
									ImGui::Image((void *)(intptr_t)image_icon.id, ImVec2(image_icon.width, image_icon.height));
									break;

								case FileTypeText:
									ImGui::Image((void *)(intptr_t)text_icon.id, ImVec2(text_icon.width, text_icon.height));
									break;

								default:
									ImGui::Image((void *)(intptr_t)file_icon.id, ImVec2(file_icon.width, file_icon.height));
									break;
							}
						}
						
						ImGui::SameLine();
						ImGui::Selectable(filename.c_str());
						
						if (first) {
							ImGui::SetFocusID(ImGui::GetID((item.entries[0].name)), ImGui::GetCurrentWindow());
							ImGuiContext& g = *ImGui::GetCurrentContext();
							g.NavDisableHighlight = false;
							first = false;
						}
						
						if (!ImGui::IsAnyItemFocused())
							GImGui->NavId = GImGui->CurrentWindow->DC.LastItemId;
							
						if (ImGui::IsItemHovered()) {
							item.selected = i;
							item.selected_filename = item.entries[item.selected].name;
						}
					}
				}
				else
					ImGui::Text("No file entries");
				
				ImGui::EndChild();
			}
			ImGui::End();
			ImGui::PopStyleVar();

			SDL_Event event;
			while (SDL_PollEvent(&event)) {
				ImGui_ImplSDL2_ProcessEvent(&event);
				if (event.type == SDL_JOYBUTTONDOWN) {
					if (event.jbutton.which == 0) {
						if (event.jbutton.button == 0) {
							if (item.state == MENU_STATE_HOME) {
								if (item.entries[item.selected].type == FsDirEntryType_Dir) {
									if (item.file_count != 0) {
										s64 value = FS::ChangeDirNext(item.entries[item.selected].name, &item.entries, item.file_count);
										if (value >= 0) {
											item.file_count = value;

											// Make a copy before resizing our vector.
											if (item.checked_count > 1)
												item.checked_copy = item.checked;
											
											item.checked.resize(item.file_count);
											GImGui->NavId = 0;
										}
									}
								}
								else {
									FileType file_type = FS::GetFileType(item.selected_filename);
									if (file_type == FileTypeImage) {
										std::string path = std::string();
										int length = FS::ConstructPath(&item.entries[item.selected], path.data(), nullptr);
										bool image_ret = Textures::LoadImageFile(path, &texture);
										IM_ASSERT(image_ret);
										item.state = MENU_STATE_IMAGEVIEWER;
									}
								}
							}
						}
						else if (event.jbutton.button == 1) {
							if (item.state == MENU_STATE_HOME) {
								s64 value = FS::ChangeDirPrev(&item.entries, item.file_count);
								if (value >= 0) {
									item.file_count = value;

									// Make a copy before resizing our vector.
									if (item.checked_count > 1)
										item.checked_copy = item.checked;
									
									item.checked.resize(item.file_count);
									GImGui->NavId = 0;
								}
							}
							else if ((item.state == MENU_STATE_PROPERTIES) || (item.state == MENU_STATE_DELETE))
								item.state = MENU_STATE_OPTIONS;
							else if (item.state == MENU_STATE_IMAGEVIEWER) {
								Textures::Free(&texture);
								item.state = MENU_STATE_HOME;
							}
							else
								item.state = MENU_STATE_HOME;
						}
						else if (event.jbutton.button == 2) {
							if (item.state == MENU_STATE_HOME)
								item.state = MENU_STATE_OPTIONS;
						}
						else if (event.jbutton.button == 3) {
							if (item.state == MENU_STATE_HOME) {
								item.checked_cwd = config.cwd;
								item.checked.at(item.selected) = !item.checked.at(item.selected);
								item.checked_count = std::count(item.checked.begin(), item.checked.end(), true);
							}
						}
						else if (event.jbutton.button == 11)
							item.state = MENU_STATE_SETTINGS;
						else if (event.jbutton.button == 10)
							done = true;
					}
				}
				
				if (event.type == SDL_QUIT)
					done = true;
				
				if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
					done = true;
			}

			switch (item.state) {
				case MENU_STATE_OPTIONS:
					GUI::RenderOptionsMenu(&item);
					break;
				
				case MENU_STATE_PROPERTIES:
					GUI::RenderPropertiesMenu(&item);
					break;
				
				case MENU_STATE_DELETE:
					GUI::RenderDeleteMenu(&item);
					break;
					
				case MENU_STATE_SETTINGS:
					GUI::RenderSettingsMenu(&item);
					break;
					
				case MENU_STATE_IMAGEVIEWER:
					GUI::RenderImageViewer(&item, &texture);
					break;
				
				default:
					break;
			}
			
			// Rendering
			ImGui::Render();
			glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
			glClearColor(0.00f, 0.00f, 0.00f, 1.00f);
			glClear(GL_COLOR_BUFFER_BIT);
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			SDL_GL_SwapWindow(window);
		}

		FS::FreeDirEntries(&item.entries, item.file_count);
		return 0;
	}
}
