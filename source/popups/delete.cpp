#include <algorithm>
#include <cstring>

#include "config.hpp"
#include "fs.hpp"
#include "imgui.h"
#include "language.hpp"
#include "log.hpp"
#include "popups.hpp"

namespace Popups {
    void DeletePopup(WindowData &data) {
        Popups::SetupPopup(strings[cfg.lang][Lang::OptionsDelete]);
        
        if (ImGui::BeginPopupModal(strings[cfg.lang][Lang::OptionsDelete], nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text(strings[cfg.lang][Lang::DeleteMessage]);
            if ((data.checkbox_data.count > 1) && (data.checkbox_data.cwd == cwd)) {
                ImGui::Text(strings[cfg.lang][Lang::DeleteMultiplePrompt]);
                ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
                ImGui::BeginChild("Scrolling", ImVec2(0, 100));
                for (std::size_t i = 0; i < data.checkbox_data.checked.size(); i++) {
                    if (data.checkbox_data.checked[i])
                        ImGui::Text(data.entries[i].name);
                }
                ImGui::EndChild();
            }
            else {
                std::string text = strings[cfg.lang][Lang::DeletePrompt] + std::string(data.entries[data.selected].name) + "?";
                ImGui::Text(text.c_str());
            }
            
            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
            
            if (ImGui::Button(strings[cfg.lang][Lang::ButtonOK], ImVec2(120, 0))) {
                bool ret = false;

                if ((data.checkbox_data.count > 1) && (data.checkbox_data.cwd == cwd)) {
                    std::vector<FsDirectoryEntry> entries;
                    if (!FS::GetDirList(data.checkbox_data.device, data.checkbox_data.cwd, entries))
                        return;
                        
                    std::sort(entries.begin(), entries.end(), FileBrowser::Sort);
                    Log::Exit();

                    for (std::size_t i = 0; i < data.checkbox_data.checked.size(); i++) {
                        if (std::strncmp(entries[i].name, "..", 2) == 0)
                            continue;
                        
                        if (data.checkbox_data.checked[i]) {
                            if (!(ret = FS::Delete(entries[i]))) {
                                FS::GetDirList(device, cwd, data.entries);
                                Windows::ResetCheckbox(data);
                                break;
                            }
                        }
                    }
                }
                else {
                    if ((std::strncmp(data.entries[data.selected].name, "..", 2)) != 0)
                        ret = FS::Delete(data.entries[data.selected]);
                }
                
                if (ret) {
                    FS::GetDirList(device, cwd, data.entries);
                    Windows::ResetCheckbox(data);
                }

                sort = -1;
                Log::Init();
                ImGui::CloseCurrentPopup();
                data.state = WINDOW_STATE_FILEBROWSER;
            }
            
            ImGui::SameLine(0.0f, 15.0f);
            
            if (ImGui::Button(strings[cfg.lang][Lang::ButtonCancel], ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
                data.state = WINDOW_STATE_OPTIONS;
            }
        }
        
        Popups::ExitPopup();
    }
}
