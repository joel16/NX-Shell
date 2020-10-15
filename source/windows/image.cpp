#include "config.h"
#include "gui.h"
#include "imgui.h"
#include "windows.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

namespace Windows {
    void ImageWindow(void) {
        Windows::SetupWindow();
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGuiWindowFlags_ filename_flag = !config.image_filename? ImGuiWindowFlags_NoTitleBar : ImGuiWindowFlags_None;
        
        if (ImGui::Begin(item.entries[item.selected].name, nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | filename_flag)) {
            ImGui::SetCursorPos((ImGui::GetWindowSize() - ImVec2(item.texture.width, item.texture.height)) * 0.5f);
            ImGui::Image(reinterpret_cast<ImTextureID>(item.texture.id), ImVec2(item.texture.width, item.texture.height));
        }
        
        Windows::ExitWindow();
        ImGui::PopStyleVar();
    }
}
