#ifndef NX_SHELL_WINDOWS_H
#define NX_SHELL_WINDOWS_H

#include <switch.h>
#include <vector>

#include "imgui.h"

typedef struct {
    std::vector<FsDirectoryEntry> entries;
    std::vector<bool> checked;
    std::vector<bool> checked_copy;
    int checked_count = 0;
    s64 used_storage = 0;
    s64 total_storage = 0;
} WindowData;

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
    
    void MainWindow(WindowData &data);
}

#endif
