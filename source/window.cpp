#include "imgui.h"
#include "tabs.h"
#include "windows.h"

namespace Windows {
    void MainWindow(WindowData &data) {
        Windows::SetupWindow();
        if (ImGui::Begin("NX-Shell", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
            if (ImGui::BeginTabBar("NX-Shell-tabs")) {
                Tabs::FileBrowser(data);
                Tabs::Settings(data);
                ImGui::EndTabBar();
            }
        }
        Windows::ExitWindow();
    }
}
