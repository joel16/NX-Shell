#include <algorithm>
#include <iostream>

#include "config.h"
#include "fs.h"
#include "gui.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include "popups.h"
#include "textures.h"
#include "windows.h"

namespace GUI {
	void ResetCheckbox(MenuItem *item) {
		item->checked.clear();
		item->checked_copy.clear();
		item->checked.resize(item->file_count);
		item->checked.assign(item->checked.size(), false);
		item->checked_cwd.clear();
		item->checked_count = 0;
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
		FS::GetUsedStorageSpace(&item.used_storage);
		FS::GetTotalStorageSpace(&item.total_storage);

		// Main loop
		bool done = false, focus = false, first_item = true;
		while (!done) {
			// Start the Dear ImGui frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplSDL2_NewFrame(window);
			ImGui::NewFrame();
			
			Windows::FileBrowserWindow(&item, &focus, &first_item);

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
										ImageType type = Textures::GetImageType(item.selected_filename);
										std::string path = std::string();
										int length = FS::ConstructPath(&item.entries[item.selected], path.data(), nullptr);
										if (length > 0) {
											bool image_ret = Textures::LoadImageFile(path, type, &texture);
											IM_ASSERT(image_ret);
											item.state = MENU_STATE_IMAGEVIEWER;
										}
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
								if ((!item.checked_cwd.empty()) && (item.checked_cwd.compare(config.cwd) != 0))
									GUI::ResetCheckbox(&item);
								
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
					Popups::OptionsPopup(&item);
					break;
				
				case MENU_STATE_PROPERTIES:
					Popups::PropertiesPopup(&item);
					break;
				
				case MENU_STATE_DELETE:
					Popups::DeletePopup(&item);
					break;
					
				case MENU_STATE_SETTINGS:
					Windows::SettingsWindow(&item);
					break;
					
				case MENU_STATE_IMAGEVIEWER:
					Windows::ImageWindow(&item, &texture);
					break;
				
				default:
					break;
			}
			
			// Rendering
			ImGui::Render();
			glViewport(0, 0, static_cast<int>(io.DisplaySize.x), static_cast<int>(io.DisplaySize.y));
			glClearColor(0.00f, 0.00f, 0.00f, 1.00f);
			glClear(GL_COLOR_BUFFER_BIT);
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			SDL_GL_SwapWindow(window);
		}

		FS::FreeDirEntries(&item.entries, item.file_count);
		return 0;
	}
}
