#include "config.h"
#include "gui.h"
#include "imgui.h"
#include "windows.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

namespace Windows {
    void ImageWindow(Tex *texture) {
        Windows::SetupWindow();
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGuiWindowFlags_ filename_flag = !config.image_filename? ImGuiWindowFlags_NoTitleBar : ImGuiWindowFlags_None; 
        
        if (ImGui::Begin(item.selected_filename.c_str(), nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | filename_flag)) {
            ImGui::SetCursorPos((ImGui::GetWindowSize() - ImVec2(texture->width, texture->height)) * 0.5f);
            ImGui::Image(reinterpret_cast<ImTextureID>(texture->id), ImVec2(texture->width, texture->height));
        }
        
        Windows::ExitWindow();
        ImGui::PopStyleVar();
    }
}
