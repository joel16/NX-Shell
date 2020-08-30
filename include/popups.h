#ifndef NX_SHELL_POPUPS_H
#define NX_SHELL_POPUPS_H

#include "imgui.h"

namespace Popups {
    inline void SetupPopup(const char *id) {
        ImGui::OpenPopup(id);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
		ImGui::SetNextWindowPos(ImVec2(640.0f, 360.0f), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
    };

    inline void ExitPopup(void) {
        ImGui::EndPopup();
        ImGui::PopStyleVar();
    }

    void ArchivePopup(void);
    void DeletePopup(void);
    void FilePropertiesPopup(void);
    void ImageProperties(bool *state);
    void OptionsPopup(void);
    void ProgressPopup(float offset, float size, const std::string &title, const std::string &text);
}

#endif
