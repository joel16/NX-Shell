#include "imgui.h"
#include "windows.h"

namespace Windows {
    void ImageWindow(MenuItem *item, Tex *texture) {
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(1280.0f, 720.0f), ImGuiCond_Once);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        
        if (ImGui::Begin(item->selected_filename.c_str(), nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
            ImGui::Image(reinterpret_cast<ImTextureID>(texture->id), ImVec2(texture->width, texture->height));
            
        ImGui::End();
        ImGui::PopStyleVar();
	}
}
