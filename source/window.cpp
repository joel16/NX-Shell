#include <algorithm>
#include <cstring>

#include "config.hpp"
#include "imgui.h"
#include "popups.hpp"
#include "tabs.hpp"
#include "windows.hpp"

WindowData data;

namespace Windows {
    static bool image_properties = false;

    void SetupWindow(void) {
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(1280.0f, 720.0f), ImGuiCond_Once);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    };
    
    void ExitWindow(void) {
        ImGui::End();
        ImGui::PopStyleVar();
    };
    
    void ResetCheckbox(WindowData &data) {
        data.checkbox_data.checked.clear();
        data.checkbox_data.checked_copy.clear();
        data.checkbox_data.checked.resize(data.entries.size());
        data.checkbox_data.checked.assign(data.checkbox_data.checked.size(), false);
        data.checkbox_data.cwd[0] = '\0';
        data.checkbox_data.count = 0;
    };

    void MainWindow(WindowData &data, u64 &key, bool progress) {
        Windows::SetupWindow();
        if (ImGui::Begin("NX-Shell", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
            if (ImGui::BeginTabBar("NX-Shell-tabs")) {
                Tabs::FileBrowser(data);
                Tabs::Settings(data);
                ImGui::EndTabBar();
            }
        }
        Windows::ExitWindow();

        if (progress)
            return;

        switch (data.state) {
            case WINDOW_STATE_OPTIONS:
                Popups::OptionsPopup(data);
                break;

            case WINDOW_STATE_PROPERTIES:
                Popups::FilePropertiesPopup(data);
                break;
            
            case WINDOW_STATE_DELETE:
                Popups::DeletePopup(data);
                break;

            case WINDOW_STATE_IMAGEVIEWER:
                Windows::ImageViewer(image_properties);
                ImageViewer::HandleControls(key, image_properties);
                break;

            default:
                break;
        }

        if ((key & HidNpadButton_X) && (data.state == WINDOW_STATE_FILEBROWSER))
            data.state = WINDOW_STATE_OPTIONS;

        if (key & HidNpadButton_Y) {
            if ((std::strlen(data.checkbox_data.cwd) != 0) && (strcasecmp(data.checkbox_data.cwd, cwd) != 0))
                Windows::ResetCheckbox(data);
            
            if ((std::strncmp(data.entries[data.selected].name, "..", 2)) != 0) {
                std::strcpy(data.checkbox_data.cwd, cwd);
                data.checkbox_data.checked.at(data.selected) = !data.checkbox_data.checked.at(data.selected);
                data.checkbox_data.count = std::count(data.checkbox_data.checked.begin(), data.checkbox_data.checked.end(), true);
            }
        }

        if (key & HidNpadButton_B) {
            switch(data.state) {
                case WINDOW_STATE_OPTIONS:
                    data.state = WINDOW_STATE_FILEBROWSER;
                    break;

                case WINDOW_STATE_PROPERTIES:
                case WINDOW_STATE_DELETE:
                    data.state = WINDOW_STATE_OPTIONS;
                    break;

                case WINDOW_STATE_IMAGEVIEWER:
                    if (image_properties)
                        image_properties = false;
                    else {
                        ImageViewer::ClearTextures();
                        data.state = WINDOW_STATE_FILEBROWSER;
                    }
                    
                    break;

                default:
                    break;
            }
        }
    }
}
