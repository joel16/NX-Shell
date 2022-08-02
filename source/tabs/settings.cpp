#include "config.hpp"
#include "fs.hpp"
#include "gui.hpp"
#include "imgui.h"
#include "language.hpp"
#include "net.hpp"
#include "popups.hpp"
#include "tabs.hpp"

namespace Tabs {
    static bool update_popup = false, network_status = false, update_available = false;
    static std::string tag_name = std::string();

    void Separator(void) {
        ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
    }
    
    void Settings(WindowData &data) {
        if (ImGui::BeginTabItem("Settings")) {
            // Disable language settings for now (At least until it;s complete)
            // if (ImGui::TreeNode(strings[cfg.lang][Lang::SettingsLanguageTitle])) {
            //     const char *languages[] = {
            //         " Japanese",
            //         " English",
            //         " French",
            //         " German",
            //         " Italian",
            //         " Spanish",
            //         " Simplified Chinese",
            //         " Korean",
            //         " Dutch",
            //         " Portuguese",
            //         " Russian",
            //         " Traditional Chinese"
            //     };

            //     ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
                
            //     const int max_lang = 12;
            //     for (int i = 0; i < max_lang; i++) {
            //         if (ImGui::RadioButton(languages[i], &cfg.lang, i))
            //             Config::Save(cfg);
                    
            //         if (i != (max_lang - 1))
            //             ImGui::Dummy(ImVec2(0.0f, 15.0f)); // Spacing
            //     }

            //     ImGui::TreePop();
            // }

            // ImGui::Separator();
            
            if (ImGui::TreeNode(strings[cfg.lang][Lang::SettingsImageViewTitle])) {
                ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing

                if (ImGui::Checkbox(strings[cfg.lang][Lang::SettingsImageViewFilenameToggle], std::addressof(cfg.image_filename)))
                    Config::Save(cfg);
                
                ImGui::TreePop();
            }
            
            ImGui::Separator();
            
            if (ImGui::TreeNode(strings[cfg.lang][Lang::SettingsDevOptsTitle])) {
                ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing

                if (ImGui::Checkbox(strings[cfg.lang][Lang::SettingsDevOptsLogsToggle], std::addressof(cfg.dev_options)))
                    Config::Save(cfg);
                
                ImGui::TreePop();
            }
            
            ImGui::Separator();
            
            if (ImGui::TreeNode(strings[cfg.lang][Lang::SettingsAboutTitle])) {
                ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
                ImGui::Text("NX-Shell %s: v%d.%d.%d", strings[cfg.lang][Lang::SettingsAboutVersion], VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO);
                ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
                ImGui::Text("ImGui %s: %s", strings[cfg.lang][Lang::SettingsAboutVersion], ImGui::GetVersion());
                ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
                ImGui::Text("%s: Joel16", strings[cfg.lang][Lang::SettingsAboutAuthor]);
                ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
                ImGui::Text("%s: Preetisketch", strings[cfg.lang][Lang::SettingsAboutBanner]);
                ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
                
                if (ImGui::Button(strings[cfg.lang][Lang::SettingsCheckForUpdates], ImVec2(250, 50))) {
                    tag_name = Net::GetLatestReleaseJSON();
                    network_status = Net::GetNetworkStatus();
                    update_available = Net::GetAvailableUpdate(tag_name);
                    update_popup = true;
                }

                ImGui::TreePop();
            }

            ImGui::EndTabItem();
        }
        
        if (update_popup)
            Popups::UpdatePopup(update_popup, network_status, update_available, tag_name);
    }
}
