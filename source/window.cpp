#include "imgui.h"
#include "popups.h"
#include "tabs.h"
#include "windows.h"

namespace Windows {
    static void SetupWindow(void) {
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(1280.0f, 720.0f), ImGuiCond_Once);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    };
    
    static void ExitWindow(void) {
        ImGui::End();
        ImGui::PopStyleVar();
    };
    
    void ResetCheckbox(WindowData &data) {
        data.checkbox_data.checked.clear();
        data.checkbox_data.checked_copy.clear();
        data.checkbox_data.checked.resize(data.entries.size());
        data.checkbox_data.checked.assign(data.checkbox_data.checked.size(), false);
        data.checkbox_data.cwd.clear();
        data.checkbox_data.count = 0;
    };

    void MainWindow(WindowData &data, u64 &key) {
        Windows::SetupWindow();
        if (ImGui::Begin("NX-Shell", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
            if (ImGui::BeginTabBar("NX-Shell-tabs")) {
                Tabs::FileBrowser(data);
                Tabs::Settings(data);
                ImGui::EndTabBar();
            }
        }
        Windows::ExitWindow();

        switch (data.state) {
            case WINDOW_STATE_OPTIONS:
                Popups::OptionsPopup(data);
                break;

            default:
                break;
        }

        if ((key & HidNpadButton_X) && (data.state == WINDOW_STATE_FILEBROWSER))
            data.state = WINDOW_STATE_OPTIONS;

        if (key & HidNpadButton_B) {
            if (data.state == WINDOW_STATE_OPTIONS)
                data.state = WINDOW_STATE_FILEBROWSER;
        }
    }
}
