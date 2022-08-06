#include <algorithm>
#include <cstring>
#include <glad/glad.h>
#include <sys/stat.h>

#include "config.hpp"
#include "fs.hpp"
#include "gui.hpp"
#include "imgui_impl_switch.hpp"
#include "imgui_internal.h"
#include "keyboard.hpp"
#include "language.hpp"
#include "popups.hpp"

namespace Options {
    static void RefreshEntries(bool reset_checkbox_data) {
        FS::GetDirList(device, cwd, data.entries);

        if (reset_checkbox_data)
            Windows::ResetCheckbox(data);
    }

    static void HandleMultipleCopy(WindowData &data, bool (*func)()) {
        std::vector<FsDirectoryEntry> entries;
        if (!FS::GetDirList(data.checkbox_data.device, data.checkbox_data.cwd, entries))
            return;

        std::sort(entries.begin(), entries.end(), FileBrowser::Sort);
        
        for (std::size_t i = 0; i < data.checkbox_data.checked_copy.size(); i++) {
            if (std::strncmp(entries[i].name, "..", 2) == 0)
                continue;
            
            if (data.checkbox_data.checked_copy[i]) {
                std::string path = data.checkbox_data.device + data.checkbox_data.cwd;
                FS::Copy(entries[i], path);

                if (!(*func)()) {
                    Options::RefreshEntries(true);
                    break;
                }
            }
        }
        
        Options::RefreshEntries(true);
        sort = -1;
        entries.clear();
    }
}

namespace Popups {
    static bool copy = false, move = false;

    void OptionsPopup(WindowData &data) {
        Popups::SetupPopup(strings[cfg.lang][Lang::OptionsTitle]);

        if (ImGui::BeginPopupModal(strings[cfg.lang][Lang::OptionsTitle], nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (ImGui::Button(strings[cfg.lang][Lang::OptionsSelectAll], ImVec2(200, 50))) {
                if ((data.checkbox_data.cwd.length() != 0) && (data.checkbox_data.cwd != cwd))
                    Windows::ResetCheckbox(data);
                
                data.checkbox_data.cwd = cwd;
                data.checkbox_data.device = device;
                std::fill(data.checkbox_data.checked.begin() + 1, data.checkbox_data.checked.end(), true);
                data.checkbox_data.count = data.checkbox_data.checked.size();
            }

            ImGui::SameLine(0.0f, 15.0f);
            
            if (ImGui::Button(strings[cfg.lang][Lang::OptionsClearAll], ImVec2(200, 50))) {
                Windows::ResetCheckbox(data);
                copy = false;
                move = false;
            }

            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing

            if (ImGui::Button(strings[cfg.lang][Lang::OptionsProperties], ImVec2(200, 50))) {
                ImGui::CloseCurrentPopup();
                data.state = WINDOW_STATE_PROPERTIES;
            }

            ImGui::SameLine(0.0f, 15.0f);

            if (ImGui::Button(strings[cfg.lang][Lang::OptionsRename], ImVec2(200, 50))) {
                std::string path = Keyboard::GetText(strings[cfg.lang][Lang::OptionsRenamePrompt], data.entries[data.selected].name);
                if (R_SUCCEEDED(FS::Rename(data.entries[data.selected], path.c_str()))) {
                    Options::RefreshEntries(false);
                    sort = -1;
                }
                
                ImGui::CloseCurrentPopup();
                data.state = WINDOW_STATE_FILEBROWSER;
            }
            
            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
            
            if (ImGui::Button(strings[cfg.lang][Lang::OptionsNewFolder], ImVec2(200, 50))) {
                std::string name = Keyboard::GetText(strings[cfg.lang][Lang::OptionsFolderPrompt], strings[cfg.lang][Lang::OptionsNewFolder]);
                std::string path = FS::BuildPath(name, true);

                if (R_SUCCEEDED(mkdir(path.c_str(), 0700))) {
                    Options::RefreshEntries(true);
                    sort = -1;
                }
                
                ImGui::CloseCurrentPopup();
                data.state = WINDOW_STATE_FILEBROWSER;
            }
            
            ImGui::SameLine(0.0f, 15.0f);
            
            if (ImGui::Button(strings[cfg.lang][Lang::OptionsNewFile], ImVec2(200, 50))) {
                std::string name = Keyboard::GetText(strings[cfg.lang][Lang::OptionsFilePrompt], strings[cfg.lang][Lang::OptionsNewFile]);
                std::string path = FS::BuildPath(name, true);
                
                FILE *file = fopen(path.c_str(), "w");
                fclose(file);
                
                if (FS::FileExists(path)) {
                    Options::RefreshEntries(true);
                    sort = -1;
                }
                
                ImGui::CloseCurrentPopup();
                data.state = WINDOW_STATE_FILEBROWSER;
            }
            
            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
            
            if (ImGui::Button(!copy? strings[cfg.lang][Lang::OptionsCopy] : strings[cfg.lang][Lang::OptionsPaste], ImVec2(200, 50))) {
                if (!copy) {
                    if ((data.checkbox_data.count >= 1) && (data.checkbox_data.cwd != cwd))
                        Windows::ResetCheckbox(data);
                    if (data.checkbox_data.count <= 1) {
                        std::string path = device + cwd;
                        FS::Copy(data.entries[data.selected], path);
                    }
                        
                    copy = !copy;
                    data.state = WINDOW_STATE_FILEBROWSER;
                }
                else {
                    ImGui::EndPopup();
                    ImGui::PopStyleVar();
                    ImGui::Render();

                    if ((data.checkbox_data.count > 1) && (data.checkbox_data.cwd != cwd))
                        Options::HandleMultipleCopy(data, std::addressof(FS::Paste));
                    else {
                        if (FS::Paste()) {
                            Options::RefreshEntries(true);
                            sort = -1;
                        }
                    }

                    copy = !copy;
                    //ImGui::CloseCurrentPopup();
                    data.state = WINDOW_STATE_FILEBROWSER;
                    return;
                }
            }
            
            ImGui::SameLine(0.0f, 15.0f);
            
            if (ImGui::Button(!move? strings[cfg.lang][Lang::OptionsMove] : strings[cfg.lang][Lang::OptionsPaste], ImVec2(200, 50))) {
                if (!move) {
                    if ((data.checkbox_data.count >= 1) && (data.checkbox_data.cwd != cwd))
                        Windows::ResetCheckbox(data);
                    if (data.checkbox_data.count <= 1) {
                        std::string path = device + cwd;
                        FS::Copy(data.entries[data.selected], path);
                    }
                }
                else {
                    if ((data.checkbox_data.count > 1) && (data.checkbox_data.cwd != cwd))
                        Options::HandleMultipleCopy(data, std::addressof(FS::Move));
                    else {
                        if (FS::Move()) {
                            Options::RefreshEntries(true);
                            sort = -1;
                        }
                    }
                }
                
                move = !move;
                ImGui::CloseCurrentPopup();
                data.state = WINDOW_STATE_FILEBROWSER;
            }
            
            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
            
            if (ImGui::Button(strings[cfg.lang][Lang::OptionsDelete], ImVec2(200, 50))) {
                ImGui::CloseCurrentPopup();
                data.state = WINDOW_STATE_DELETE;
            }
            
            ImGui::SameLine(0.0f, 15.0f);
            
            if (ImGui::Button(strings[cfg.lang][Lang::OptionsSetArchiveBit], ImVec2(200, 50))) {
                std::string path = FS::BuildPath(data.entries[data.selected]);
                
                if (FS::SetArchiveBit(path)) {
                    Options::RefreshEntries(true);
                    sort = -1;
                }
                    
                ImGui::CloseCurrentPopup();
                data.state = WINDOW_STATE_FILEBROWSER;
            }
        }
        
        Popups::ExitPopup();
    }
}
