#include "config.hpp"
#include "imgui.h"
#include "language.hpp"
#include "popups.hpp"
#include "usb.hpp"

namespace Popups {
    static bool done = false;

    void USBPopup(bool &state) {
        Popups::SetupPopup(strings[cfg.lang][Lang::SettingsUSBTitle]);

        if (ImGui::BeginPopupModal(strings[cfg.lang][Lang::SettingsUSBTitle], nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (!done)
                ImGui::Text(strings[cfg.lang][Lang::USBUnmountPrompt]);
            else
                ImGui::Text(strings[cfg.lang][Lang::USBUnmountSuccess]);
            
            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
            
            if (ImGui::Button(strings[cfg.lang][Lang::ButtonOK], ImVec2(120, 0))) {
                if (!done) {
                    USB::Unmount();
                    ImGui::CloseCurrentPopup();
                    done = true;
                }
                else {
                    ImGui::CloseCurrentPopup();
                    done = false;
                    state = false;
                }
            }
            
            ImGui::SameLine(0.0f, 15.0f);
            
            if (!done) {
                if (ImGui::Button(strings[cfg.lang][Lang::ButtonCancel], ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                    state = false;
                }
            }
        }

        Popups::ExitPopup();
    }
}
