#include <glad/glad.h>
#include <switch.h>

#include "gui.hpp"
#include "imgui_impl_switch.hpp"
#include "log.hpp"
#include "popups.hpp"
#include "windows.hpp"

// Todo maybe use a thread to run this?
namespace Popups {
    
    void ProgressBar(float offset, float size, const std::string &title, const std::string &text) {
        u64 key = ImGui_ImplSwitch_NewFrame();
        ImGui::NewFrame();
        
        Windows::MainWindow(data, key, true);
        Popups::SetupPopup(title.c_str());
        
        if (ImGui::BeginPopupModal(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text(text.c_str());
            ImGui::ProgressBar(offset/size, ImVec2(0.0f, 0.0f));
        }
        
        Popups::ExitPopup();
        
        ImGui::Render();
        glViewport(0, 0, static_cast<int>(ImGui::GetIO().DisplaySize.x), static_cast<int>(ImGui::GetIO().DisplaySize.y));
        glClearColor(0.00f, 0.00f, 0.00f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplSwitch_RenderDrawData(ImGui::GetDrawData());
        GUI::SwapBuffers();
    }
}
