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
#include "windows.h"

// Global var used across windows/popups
MenuItem item;

namespace GUI {
	enum SDL_KEYS {
		SDL_KEY_A, SDL_KEY_B, SDL_KEY_X, SDL_KEY_Y,
		SDL_KEY_LSTICK, SDL_KEY_RSTICK,
		SDL_KEY_L, SDL_KEY_R,
		SDL_KEY_ZL, SDL_KEY_ZR,
		SDL_KEY_PLUS, SDL_KEY_MINUS,
		SDL_KEY_DLEFT, SDL_KEY_DUP, SDL_KEY_DRIGHT, SDL_KEY_DDOWN,
		SDL_KEY_LSTICK_LEFT, SDL_KEY_LSTICK_UP, SDL_KEY_LSTICK_RIGHT, SDL_KEY_LSTICK_DOWN,
		SDL_KEY_RSTICK_LEFT, SDL_KEY_RSTICK_UP, SDL_KEY_RSTICK_RIGHT, SDL_KEY_RSTICK_DOWN,
		SDL_KEY_SL_LEFT, SDL_KEY_SR_LEFT, SDL_KEY_SL_RIGHT, SDL_KEY_SR_RIGHT
	};

	int RenderLoop(void) {
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		item.state = MENU_STATE_FILEBROWSER;
		item.selected = 0;
		
		Result ret = 0;
		if (R_FAILED(ret = FS::GetDirList(cfg.cwd, item.entries)))
			return ret;

		item.checked.resize(item.entries.size());
		FS::GetUsedStorageSpace(&item.used_storage);
		FS::GetTotalStorageSpace(&item.total_storage);

		// Main loop
		bool done = false, focus = false, first_item = true, tex_properties = false;

		while (!done) {
			// Start the Dear ImGui frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplSDL2_NewFrame(window);
			ImGui::NewFrame();
			
			Windows::FileBrowserWindow(&focus, &first_item);

			SDL_Event event;
			while (SDL_PollEvent(&event)) {
				ImGui_ImplSDL2_ProcessEvent(&event);
				if (event.type == SDL_JOYBUTTONDOWN) {
					Uint8 button = event.jbutton.button;

					if (button == SDL_KEY_A) {
						if (item.state == MENU_STATE_FILEBROWSER) {
							if (item.entries[item.selected].type == FsDirEntryType_Dir) {
								if (item.entries.size() != 0) {
									if (R_SUCCEEDED(FS::ChangeDirNext(item.entries[item.selected].name, item.entries))) {
										// Make a copy before resizing our vector.
										if ((item.checked_count > 1) && (item.checked_copy.empty()))
											item.checked_copy = item.checked;
										
										item.checked.resize(item.entries.size());
										GImGui->NavId = 0;
									}
								}
							}
						}
					}
					else if (button == SDL_KEY_B) {
						if (item.state == MENU_STATE_FILEBROWSER) {
							if (R_SUCCEEDED(FS::ChangeDirPrev(item.entries))) {
								// Make a copy before resizing our vector.
								if (item.checked_count > 1)
									item.checked_copy = item.checked;
									
								item.checked.resize(item.entries.size());
								GImGui->NavId = 0;
							}
						}
						else if ((item.state == MENU_STATE_PROPERTIES) || (item.state == MENU_STATE_DELETE))
							item.state = MENU_STATE_OPTIONS;
						else if (item.state == MENU_STATE_IMAGEVIEWER) {
							if (tex_properties)
								tex_properties = false;
							else {
								for (long unsigned int i = 0; i < item.textures.size(); i++)
									Textures::Free(&item.textures[i]);
									
								item.textures.clear();
								item.frame_count = 0;
								item.zoom_factor = 1.0f;
								item.state = MENU_STATE_FILEBROWSER;
							}
						}
						else if (item.state == MENU_STATE_TEXTREADER) {
							text_reader.buf_size = 0;
							delete[] text_reader.buf;
							item.state = MENU_STATE_FILEBROWSER;
						}
						else if (item.state == MENU_STATE_SETTINGS)
							item.state = MENU_STATE_FILEBROWSER;
						else
							item.state = MENU_STATE_FILEBROWSER;
					}
					else if (button == SDL_KEY_X) {
						if (item.state == MENU_STATE_FILEBROWSER)
							item.state = MENU_STATE_OPTIONS;
						else if (item.state == MENU_STATE_IMAGEVIEWER)
							tex_properties = true;
					}
					else if (button == SDL_KEY_Y) {
						if (item.state == MENU_STATE_FILEBROWSER) {
							if ((!item.checked_cwd.empty()) && (item.checked_cwd.compare(cfg.cwd) != 0))
								GUI::ResetCheckbox();
								
							item.checked_cwd = cfg.cwd;
							item.checked.at(item.selected) = !item.checked.at(item.selected);
							item.checked_count = std::count(item.checked.begin(), item.checked.end(), true);
						}
					}
					else if (button == SDL_KEY_DLEFT) {
						if (item.state == MENU_STATE_FILEBROWSER) {
							ImGui::SetItemDefaultFocus();
						}
					}
					else if (button == SDL_KEY_DRIGHT) {
						if (item.state == MENU_STATE_FILEBROWSER) {
							ImGui::SetScrollHereY(1.0f);
						}
					}
					else if (button == SDL_KEY_MINUS)
						item.state = MENU_STATE_SETTINGS;
					else if (button == SDL_KEY_PLUS)
						done = true;
					// TODO fix this so that it's continous or just scrap SDL events
					else if (button == SDL_KEY_LSTICK_DOWN) {
						if (item.state == MENU_STATE_IMAGEVIEWER) {
							item.zoom_factor -= 0.5f * ImGui::GetIO().DeltaTime;
							
							if (item.zoom_factor < 0.1f)
								item.zoom_factor = 0.1f;
						}
					}
					else if (button == SDL_KEY_LSTICK_UP) {
						if (item.state == MENU_STATE_IMAGEVIEWER) {
							item.zoom_factor += 0.5f * ImGui::GetIO().DeltaTime;
							
							if (item.zoom_factor > 5.0f)
								item.zoom_factor = 5.0f;
						}
					}
				}
				
				if (event.type == SDL_QUIT)
					done = true;
				
				if ((event.type == SDL_WINDOWEVENT) && (event.window.event == SDL_WINDOWEVENT_CLOSE) && (event.window.windowID == SDL_GetWindowID(window)))
					done = true;
			}

			switch (item.state) {
				// Popups
				case MENU_STATE_ARCHIVEEXTRACT:
					Popups::ArchivePopup();
					break;

				case MENU_STATE_DELETE:
					Popups::DeletePopup();
					break;
				
				case MENU_STATE_OPTIONS:
					Popups::OptionsPopup();
					break;
				
				case MENU_STATE_PROPERTIES:
					Popups::FilePropertiesPopup();
					break;

				// Windows
				case MENU_STATE_IMAGEVIEWER:
					Windows::ImageWindow();
					if (tex_properties)
						Popups::ImageProperties(&tex_properties);
					
					break;
					
				case MENU_STATE_SETTINGS:
					Windows::SettingsWindow();
					break;

				case MENU_STATE_TEXTREADER:
					Windows::TextReaderWindow();
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

		item.entries.clear();
		return 0;
	}
}
