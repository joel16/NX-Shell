#pragma once

#include <string>

#include "imgui.h"
#include "windows.hpp"

namespace Popups {
    inline void SetupPopup(const char *id) {
        ImGui::OpenPopup(id);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
        ImGui::SetNextWindowPos(ImVec2(640.0f, 360.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    };
    
    inline void ExitPopup(void) {
        ImGui::EndPopup();
        ImGui::PopStyleVar();
    };

    void ArchivePopup(void);
    void DeletePopup(WindowData &data);
    void FilePropertiesPopup(WindowData &data);
    void ImageProperties(bool &state, Tex &texture);
    void OptionsPopup(WindowData &data);
    void ProgressPopup(float offset, float size, const std::string &title, const std::string &text);
    void UpdatePopup(bool &state, bool &connection_status, bool &available, const std::string &tag);
    void ProgressBar(float offset, float size, const std::string &title, const std::string &text);
}
