#include "fs.h"
#include "gui.h"
#include "imgui.h"
#include "log.h"
#include "net.h"
#include "popups.h"
#include "utils.h"
#include "windows.h"
#include "lang.hpp"
using namespace lang::literals;
namespace Popups {
    static bool done = false;

    void UpdatePopup(bool *state, bool *connection_status, bool *available, const std::string &tag) {
        Popups::SetupPopup("Update"_lang.c_str());
        
        if (ImGui::BeginPopupModal("Update"_lang.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (!*connection_status)
                ImGui::Text("Text_Could"_lang.c_str());
            else if ((*connection_status) && (*available) && (!tag.empty()) && (!done)) {
                ImGui::Text("Text_An"_lang.c_str());
                std::string text = "Text_Do_you_wish_to_download"_lang+ tag + "?";
                ImGui::Text(text.c_str());
            }
            else if (done) {
                ImGui::Text("Text_update"_lang.c_str());
                ImGui::Text("Text_Please"_lang.c_str());
            }
            else
                ImGui::Text("Text_You"_lang.c_str());

            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
            
            if (ImGui::Button("OK_update"_lang.c_str(), ImVec2(120, 0))) {
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
                
                if (ImGui::Button("Cancel_update"_lang.c_str(), ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                    *state = false;
                }
            }
        }
        
        Popups::ExitPopup();
    }
}
