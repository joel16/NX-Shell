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
        ImGuiWindowFlags_ filename_flag = !cfg.image_filename? ImGuiWindowFlags_NoTitleBar : ImGuiWindowFlags_None;
        
        if (ImGui::Begin(item.entries[item.selected].name, nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | filename_flag)) {
            if (((item.textures[0].width * item.zoom_factor) <= 1280) && ((item.textures[0].height * item.zoom_factor) <= 720))
                ImGui::SetCursorPos((ImGui::GetWindowSize() - ImVec2((item.textures[0].width * item.zoom_factor), (item.textures[0].height * item.zoom_factor))) * 0.5f);
                
            if (item.textures.size() > 1) {
                svcSleepThread(item.textures[item.frame_count].delay * 10000000);
                ImGui::Image(reinterpret_cast<ImTextureID>(item.textures[item.frame_count].id), (ImVec2((item.textures[item.frame_count].width * item.zoom_factor), 
                    (item.textures[item.frame_count].height * item.zoom_factor))));
                item.frame_count++;
                
                // Reset frame counter
                if (item.frame_count == item.textures.size() - 1)
                    item.frame_count = 0;
            }
            else
                ImGui::Image(reinterpret_cast<ImTextureID>(item.textures[0].id), ImVec2((item.textures[0].width * item.zoom_factor), (item.textures[0].height * item.zoom_factor)));
        }
        
        Windows::ExitWindow();
        ImGui::PopStyleVar();
    }
}
