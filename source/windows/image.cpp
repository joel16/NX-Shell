#include "gui.h"
#include "imgui.h"
#include "windows.h"

namespace Windows {
    void ImageWindow(Tex *texture) {
        Windows::SetupWindow();
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        if (ImGui::Begin(item.selected_filename.c_str(), nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar))
            ImGui::Image(reinterpret_cast<ImTextureID>(texture->id), ImVec2(texture->width, texture->height));
        
        Windows::ExitWindow();
        ImGui::PopStyleVar();
    }
}
