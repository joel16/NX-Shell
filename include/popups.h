#ifndef NX_SHELL_POPUPS_H
#define NX_SHELL_POPUPS_H

#include "gui.h"
#include "imgui.h"

namespace Popups {
    inline void SetupPopup(const char *id) {
        ImGui::OpenPopup(id);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
		ImGui::SetNextWindowPos(ImVec2(640.0f, 360.0f), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
    };

    void DeletePopup(MenuItem *item);
    void OptionsPopup(MenuItem *item);
    void ProgressPopup(float offset, float size, const std::string &title, const std::string &text);
    void PropertiesPopup(MenuItem *item);
}

#endif
