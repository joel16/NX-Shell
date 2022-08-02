#include "config.hpp"
#include "fs.hpp"
#include "gui.hpp"
#include "imgui.h"
#include "language.hpp"
#include "popups.hpp"
#include "utils.hpp"

namespace Popups {
    static char *FormatDate(char *string, time_t timestamp) {
        strftime(string, 36, "%Y/%m/%d %H:%M:%S", localtime(std::addressof(timestamp)));
        return string;
    }

    void FilePropertiesPopup(WindowData &data) {
        Popups::SetupPopup(strings[cfg.lang][Lang::OptionsProperties]);
        
        if (ImGui::BeginPopupModal(strings[cfg.lang][Lang::OptionsProperties], nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            std::string name_text = strings[cfg.lang][Lang::PropertiesName] + std::string(data.entries[data.selected].name);
            ImGui::Text(name_text.c_str());
            
            if (data.entries[data.selected].type == FsDirEntryType_File) {
                char size[16];
                Utils::GetSizeString(size, static_cast<double>(data.entries[data.selected].file_size));
                std::string size_text = strings[cfg.lang][Lang::PropertiesSize];
                size_text.append(size);
                ImGui::Text(size_text.c_str());
            }
            
            FsTimeStampRaw timestamp;
            if (R_SUCCEEDED(FS::GetTimeStamp(data.entries[data.selected], timestamp))) {
                if (timestamp.is_valid == 1) { // Confirm valid timestamp
                    char date[3][36];
                    
                    std::string created_time = strings[cfg.lang][Lang::PropertiesCreated];
                    created_time.append(Popups::FormatDate(date[0], timestamp.created));
                    ImGui::Text(created_time.c_str());
                    
                    std::string modified_time = strings[cfg.lang][Lang::PropertiesModified];
                    modified_time.append(Popups::FormatDate(date[1], timestamp.modified));
                    ImGui::Text(modified_time.c_str());
                    
                    std::string accessed_time = strings[cfg.lang][Lang::PropertiesAccessed];
                    accessed_time.append(Popups::FormatDate(date[2], timestamp.accessed));
                    ImGui::Text(accessed_time.c_str());
                }
            }
            
            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
            
            if (ImGui::Button(strings[cfg.lang][Lang::ButtonOK], ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
                data.state = WINDOW_STATE_OPTIONS;
            }
        }
        
        Popups::ExitPopup();
    }

    void ImageProperties(bool &state, Tex &texture) {
        Popups::SetupPopup(strings[cfg.lang][Lang::OptionsProperties]);

        std::string new_width, new_height;
        if (ImGui::BeginPopupModal("Properties", std::addressof(state), ImGuiWindowFlags_AlwaysAutoResize)) {
            std::string parent_text = "Parent: ";
            parent_text.append(cwd);
            ImGui::Text(parent_text.c_str());

            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing

            std::string name_text = "Name: ";
            name_text.append(data.entries[data.selected].name);
            ImGui::Text(name_text.c_str());

            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing

            char size[16];
            Utils::GetSizeString(size, static_cast<double>(data.entries[data.selected].file_size));
            std::string size_text = "Size: ";
            size_text.append(size);
            ImGui::Text(size_text.c_str());

            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing

            std::string width_text = "Width: ";
            width_text.append(std::to_string(texture.width));
            width_text.append("px");
            ImGui::Text(width_text.c_str());
            /*ImGui::SameLine(0.0f, 10.0f);
            if (ImGui::Button("Edit width"))
                new_width = Keyboard::GetText("Enter width", std::to_string(texture.width));*/

            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing

            std::string height_text = "Height: ";
            height_text.append(std::to_string(texture.height));
            height_text.append("px");
            ImGui::Text(height_text.c_str());

            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
            
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
                state = false;
            }
        }
        
        Popups::ExitPopup();
    }
}
