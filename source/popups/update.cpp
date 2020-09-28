#include "fs.h"
#include "gui.h"
#include "imgui.h"
#include "log.h"
#include "net.h"
#include "popups.h"
#include "utils.h"
#include "windows.h"

namespace Popups {
    static bool done = false;

    void UpdatePopup(bool *state, bool *connection_status, bool *available, const std::string &tag) {
        Popups::SetupPopup("Update");
        
        if (ImGui::BeginPopupModal("Update", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (!*connection_status)
                ImGui::Text("Could not connect to network.");
            else if ((*connection_status) && (*available) && (!tag.empty()) && (!done)) {
                ImGui::Text("An update is available.");
                std::string text = "Do you wish to download and install NX-Shell v" + tag + "?";
                ImGui::Text(text.c_str());
            }
            else if (done) {
                ImGui::Text("Update was successful.");
                ImGui::Text("Please exit and rerun the application.");
            }
            else
                ImGui::Text("You are on the latest version.");

            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
            
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                if (!done) {
                    Net::GetLatestReleaseNRO(tag);
                    
                    Result ret = 0;
                    if (R_FAILED(ret = fsFsDeleteFile(fs, __application_path)))
                        Log::Error("fsFsDeleteFile(%s) failed: 0x%x\n", __application_path, ret);
                    
                    if (R_FAILED(ret = fsFsRenameFile(fs, "/switch/NX-Shell/NX-Shell_UPDATE.nro", __application_path)))
                        Log::Error("fsFsRenameFile(update) failed: 0x%x\n", ret);
                        
                    done = true;
                }
                else {
                    ImGui::CloseCurrentPopup();
                    *state = false;
                }
            }

            if ((*connection_status) && (*available) && (!done)) {
                ImGui::SameLine(0.0f, 15.0f);
                
                if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                    *state = false;
                }
            }
        }
        
        Popups::ExitPopup();
    }
}
