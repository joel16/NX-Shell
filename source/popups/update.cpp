#include "config.hpp"
#include "fs.hpp"
#include "gui.hpp"
#include "imgui.h"
#include "language.hpp"
#include "log.hpp"
#include "net.hpp"
#include "popups.hpp"
#include "utils.hpp"
#include "windows.hpp"

namespace Popups {
    static bool done = false;

    void UpdatePopup(bool &state, bool &connection_status, bool &available, const std::string &tag) {
        Popups::SetupPopup(strings[cfg.lang][Lang::UpdateTitle]);
        
        if (ImGui::BeginPopupModal(strings[cfg.lang][Lang::UpdateTitle], nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (!connection_status)
                ImGui::Text(strings[cfg.lang][Lang::UpdateNetworkError]);
            else if ((connection_status) && (available) && (!tag.empty()) && (!done)) {
                ImGui::Text(strings[cfg.lang][Lang::UpdateAvailable]);
                std::string text = strings[cfg.lang][Lang::UpdatePrompt] + tag + "?";
                ImGui::Text(text.c_str());
            }
            else if (done) {
                ImGui::Text(strings[cfg.lang][Lang::UpdateSuccess]);
                ImGui::Text(strings[cfg.lang][Lang::UpdateRestart]);
            }
            else
                ImGui::Text(strings[cfg.lang][Lang::UpdateNotAvailable]);

            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
            
            if (ImGui::Button(strings[cfg.lang][Lang::ButtonOK], ImVec2(120, 0))) {
                if ((connection_status) && (available) && (!tag.empty()) && (!done)) {
                    Net::GetLatestReleaseNRO(tag);
                    
                    Result ret = 0;
                    if (R_FAILED(ret = fsFsDeleteFile(std::addressof(devices[FileSystemSDMC]), __application_path)))
                        Log::Error("fsFsDeleteFile(%s) failed: 0x%x\n", __application_path, ret);
                    
                    if (R_FAILED(ret = fsFsRenameFile(std::addressof(devices[FileSystemSDMC]), "/switch/NX-Shell/NX-Shell_UPDATE.nro", __application_path)))
                        Log::Error("fsFsRenameFile(update) failed: 0x%x\n", ret);
                        
                    done = true;
                }
                else {
                    ImGui::CloseCurrentPopup();
                    state = false;
                }
            }

            if ((connection_status) && (available) && (!done)) {
                ImGui::SameLine(0.0f, 15.0f);
                
                if (ImGui::Button(strings[cfg.lang][Lang::ButtonCancel], ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                    state = false;
                }
            }
        }
        
        Popups::ExitPopup();
    }
}
