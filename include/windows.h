#ifndef NX_SHELL_WINDOWS_H
#define NX_SHELL_WINDOWS_H

#include "imgui.h"
#include "textures.h"

namespace Windows {
    inline void SetupWindow(void) {
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(1280.0f, 720.0f), ImGuiCond_Once);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    };

    inline void ExitWindow(void) {
        ImGui::End();
        ImGui::PopStyleVar();
    };

    void FileBrowserWindow(bool *focus, bool *first_item);
    void ImageWindow(void);
    void SettingsWindow(void);
    void TextReaderWindow(void);
}

#endif
