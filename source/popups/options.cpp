#include <algorithm>
#include <cstring>

#include "config.h"
#include "fs.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "keyboard.h"
#include "language.h"
#include "popups.h"

namespace Popups {
    static bool copy = false, move = false;
    static void HandleMultipleCopy(WindowData &data, Result (*func)()) {
        Result ret = 0;
        std::vector<FsDirectoryEntry> entries;
        
        if (R_FAILED(ret = FS::GetDirList(data.checkbox_data.cwd, entries)))
            return;

        std::sort(entries.begin(), entries.end(), FileBrowser::Sort);
        
        for (long unsigned int i = 0; i < data.checkbox_data.checked_copy.size(); i++) {
            if (std::strncmp(entries[i].name, "..", 2) == 0)
                continue;
            
            if (data.checkbox_data.checked_copy.at(i)) {
                FS::Copy(entries[i], data.checkbox_data.cwd);

                if (R_FAILED((*func)())) {
                    FS::GetDirList(cfg.cwd, data.entries);
                    Windows::ResetCheckbox(data);
                    break;
                }
            }
        }

        FS::GetDirList(cfg.cwd, data.entries);
        Windows::ResetCheckbox(data);
        sort = -1;
        entries.clear();
    }

    void OptionsPopup(WindowData &data) {
        Popups::SetupPopup(strings[cfg.lang][Lang::OptionsTitle]);

        if (ImGui::BeginPopupModal(strings[cfg.lang][Lang::OptionsTitle], nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (ImGui::Button(strings[cfg.lang][Lang::OptionsSelectAll], ImVec2(200, 50))) {
                if ((std::strlen(data.checkbox_data.cwd) != 0) && (strcasecmp(data.checkbox_data.cwd, cfg.cwd) != 0))
                    Windows::ResetCheckbox(data);
                
                std::strcpy(data.checkbox_data.cwd, cfg.cwd);
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
                    FS::GetDirList(cfg.cwd, data.entries);
                    sort = -1;
                }
                
                ImGui::CloseCurrentPopup();
                data.state = WINDOW_STATE_FILEBROWSER;
            }
            
            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
            
            if (ImGui::Button(strings[cfg.lang][Lang::OptionsNewFolder], ImVec2(200, 50))) {
                std::string name = Keyboard::GetText(strings[cfg.lang][Lang::OptionsFolderPrompt], strings[cfg.lang][Lang::OptionsNewFolder]);
                char path[FS_MAX_PATH];
                std::snprintf(path, FS_MAX_PATH, "%s/%s", cfg.cwd, name.c_str());

                if (R_SUCCEEDED(fsFsCreateDirectory(fs, path))) {
                    FS::GetDirList(cfg.cwd, data.entries);
                    Windows::ResetCheckbox(data);
                    sort = -1;
                }
                
                ImGui::CloseCurrentPopup();
                data.state = WINDOW_STATE_FILEBROWSER;
            }
            
            ImGui::SameLine(0.0f, 15.0f);
            
            if (ImGui::Button(strings[cfg.lang][Lang::OptionsNewFile], ImVec2(200, 50))) {
                std::string name = Keyboard::GetText(strings[cfg.lang][Lang::OptionsFilePrompt], strings[cfg.lang][Lang::OptionsNewFile]);
                char path[FS_MAX_PATH];
                std::snprintf(path, FS_MAX_PATH, "%s/%s", cfg.cwd, name.c_str());
                
                if (R_SUCCEEDED(fsFsCreateFile(fs, path, 0, 0))) {
                    FS::GetDirList(cfg.cwd, data.entries);
                    Windows::ResetCheckbox(data);
                    sort = -1;
                }
                
                ImGui::CloseCurrentPopup();
                data.state = WINDOW_STATE_FILEBROWSER;
            }
            
            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
            
            if (ImGui::Button(!copy? strings[cfg.lang][Lang::OptionsCopy] : strings[cfg.lang][Lang::OptionsPaste], ImVec2(200, 50))) {
                if (!copy) {
                    if ((data.checkbox_data.count >= 1) && (strcasecmp(data.checkbox_data.cwd, cfg.cwd) != 0))
                        Windows::ResetCheckbox(data);
                    if (data.checkbox_data.count <= 1)
                        FS::Copy(data.entries[data.selected], cfg.cwd);
                        
                    copy = !copy;
                    data.state = WINDOW_STATE_FILEBROWSER;
                }
                else {
                    // ImGui::EndPopup();
                    // ImGui::PopStyleVar();
                    // ImGui::Render();

                    if ((data.checkbox_data.count > 1) && (strcasecmp(data.checkbox_data.cwd, cfg.cwd) != 0))
                        Popups::HandleMultipleCopy(data, &FS::Paste);
                    else {
                        if (R_SUCCEEDED(FS::Paste())) {
                            FS::GetDirList(cfg.cwd, data.entries);
                            Windows::ResetCheckbox(data);
                            sort = -1;
                        }
                    }

                    copy = !copy;
                    ImGui::CloseCurrentPopup();
                    data.state = WINDOW_STATE_FILEBROWSER;
                }
            }
            
            ImGui::SameLine(0.0f, 15.0f);
            
            if (ImGui::Button(!move? strings[cfg.lang][Lang::OptionsMove] : strings[cfg.lang][Lang::OptionsPaste], ImVec2(200, 50))) {
                if (!move) {
                    if ((data.checkbox_data.count >= 1) && (strcasecmp(data.checkbox_data.cwd, cfg.cwd) != 0))
                        Windows::ResetCheckbox(data);
                    if (data.checkbox_data.count <= 1)
                        FS::Copy(data.entries[data.selected], cfg.cwd);
                }
                else {
                    if ((data.checkbox_data.count > 1) && (strcasecmp(data.checkbox_data.cwd, cfg.cwd) != 0))
                        Popups::HandleMultipleCopy(data, &FS::Move);
                    else {
                        if (R_SUCCEEDED(FS::Move())) {
                            FS::GetDirList(cfg.cwd, data.entries);
                            Windows::ResetCheckbox(data);
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
                if (R_SUCCEEDED(FS::SetArchiveBit(data.entries[data.selected]))) {
                    FS::GetDirList(cfg.cwd, data.entries);
                    Windows::ResetCheckbox(data);
                    sort = -1;
                }
                    
                ImGui::CloseCurrentPopup();
                data.state = WINDOW_STATE_FILEBROWSER;
            }
        }
        
        Popups::ExitPopup();
    }
}
