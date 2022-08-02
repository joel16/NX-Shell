#include <string>

#include "config.hpp"
#include "gui.hpp"
#include "imgui.h"
#include "popups.hpp"
#include "windows.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

namespace ImageViewer {
    void ClearTextures(void) {
        data.textures.clear();
        data.frame_count = 0;
    }

    bool HandleScroll(int index) {
        if (data.entries[index].type == FsDirEntryType_Dir)
            return false;
        else {
            data.selected = index;
            
            char fs_path[FS_MAX_PATH + 1];
            if ((std::snprintf(fs_path, FS_MAX_PATH, "%s/%s", cwd, data.entries[index].name)) > 0) {
                bool ret = Textures::LoadImageFile(fs_path, data.textures);
                IM_ASSERT(ret);
                return ret;
            }
        }

        return false;
    }

    bool HandlePrev(void) {
        bool ret = false;

        for (int i = data.selected - 1; i > 0; i--) {
            std::string filename = data.entries[i].name;
            if (filename.empty())
                continue;
                
            if (!(ret = ImageViewer::HandleScroll(i)))
                continue;
            else
                break;
        }

        return ret;
    }

    bool HandleNext(void) {
        bool ret = false;

        if (data.selected == data.entries.size())
            return ret;
        
        for (unsigned int i = data.selected + 1; i < data.entries.size(); i++) {
            if (!(ret = ImageViewer::HandleScroll(i)))
                continue;
            else
                break;
        }

        return ret;
    }

    void HandleControls(u64 &key, bool &properties) {
        if (key & HidNpadButton_X)
            properties = !properties;
        
        if (key & HidNpadButton_StickLDown) {
            data.zoom_factor -= 0.5f * ImGui::GetIO().DeltaTime;
            
            if (data.zoom_factor < 0.1f)
                data.zoom_factor = 0.1f;
        }
        else if (key & HidNpadButton_StickLUp) {
            data.zoom_factor += 0.5f * ImGui::GetIO().DeltaTime;
            
            if (data.zoom_factor > 5.0f)
                data.zoom_factor = 5.0f;
        }
        
        if (!properties) {
            if (key & HidNpadButton_B) {
                ImageViewer::ClearTextures();
                data.zoom_factor = 1.0f;
                data.state = WINDOW_STATE_FILEBROWSER;
            }
            
            if (key & HidNpadButton_L) {
                ImageViewer::ClearTextures();
                
                if (!ImageViewer::HandlePrev())
                    data.state = WINDOW_STATE_FILEBROWSER;
            }
            else if (key & HidNpadButton_R) {
                ImageViewer::ClearTextures();
                
                if (!ImageViewer::HandleNext())
                    data.state = WINDOW_STATE_FILEBROWSER;
            }
        }
    }
}

namespace Windows {
    void ImageViewer(bool &properties) {
        Windows::SetupWindow();
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGuiWindowFlags_ filename_flag = !cfg.image_filename? ImGuiWindowFlags_NoTitleBar : ImGuiWindowFlags_None;
        
        if (ImGui::Begin(data.entries[data.selected].name, nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar | filename_flag)) {
            if (((data.textures[0].width * data.zoom_factor) <= 1280) && ((data.textures[0].height * data.zoom_factor) <= 720))
                ImGui::SetCursorPos((ImGui::GetWindowSize() - ImVec2((data.textures[0].width * data.zoom_factor), (data.textures[0].height * data.zoom_factor))) * 0.5f);
                
            if (data.textures.size() > 1) {
                svcSleepThread(data.textures[data.frame_count].delay);
                ImGui::Image(reinterpret_cast<ImTextureID>(data.textures[data.frame_count].id), (ImVec2((data.textures[data.frame_count].width * data.zoom_factor), 
                    (data.textures[data.frame_count].height * data.zoom_factor))));
                data.frame_count++;
                
                // Reset frame counter
                if (data.frame_count == data.textures.size() - 1)
                    data.frame_count = 0;
            }
            else
                ImGui::Image(reinterpret_cast<ImTextureID>(data.textures[0].id), ImVec2((data.textures[0].width * data.zoom_factor), (data.textures[0].height * data.zoom_factor)));
        }

        if (properties)
            Popups::ImageProperties(properties, data.textures[0]);
        
        Windows::ExitWindow();
        ImGui::PopStyleVar();
    }
}
